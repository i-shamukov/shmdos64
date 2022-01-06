/*
   tests.cpp
   Kernel unit tests
   SHM DOS64
   Copyright (c) 2022, Ilya Shamukov, ilya.shamukov@gmail.com
   
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option) 
   any later version.
   
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
   more details.
   
   You should have received a copy of the GNU General Public License along with 
   this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
   Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <atomic>
#include <cmath>
#include <conout.h>
#include <kalgorithm.h>
#include <cpu.h>
#include <VirtualMemoryManager.h>
#include <kvector.h>
#include <klist.h>
#include <kthread.h>
#include <kevent.h>
#include <kmutex.h>
#include <kcondition_variable.h>
#include <kunordered_map.h>
#include <ThreadPool.h>
#include <AbstractDevice.h>
#include "phmem.h"
#include "common_lib.h"
#include "Heap.h"
#include "AbstractTimer.h"
#include "Semaphore.h"

#include "tests.h"

const wchar_t* g_testOk = L"OK";
const wchar_t* g_testFail = L"FAIL";
const wchar_t* g_assertStr = L"Assert at line ";

#define DEF_TEST(name)\
    static void _##name(bool&);\
    static void name()\
    {\
		print(#name, L"... ");\
		bool result = true;\
		_##name(result);\
		println(result ? g_testOk : g_testFail);\
    }\
    static void _##name(bool& __test_result)

#define ASSERT(ex)\
    if (!(ex)) \
    {\
		__test_result = false;\
		println(g_assertStr, __LINE__, L": ", #ex);\
		return;\
    }

#define EXPECT(ex)\
    __test_result = __test_result && (ex);

DEF_TEST(ramPagesTest)
{
	const int max = 512;
	uintptr_t pages[max] = {};
	RamAllocator& allocator = RamAllocator::getInstance();
	for (int i = 0; i < max; ++i)
	{
		const uintptr_t newPage = allocator.allocPage(false);
		ASSERT(newPage != 0);
		for (int j = 0; j < i; ++j)
			ASSERT(pages[j] != newPage);
		pages[i] = newPage;
	}
	for (int i = 0; i < max; i += 2)
	{
		allocator.freePage(pages[i]);
		pages[i] = 0;
	}
	for (int i = 0; i < max; i += 2)
	{
		const uintptr_t newPage = allocator.allocPage(false);
		ASSERT(newPage != 0);
		for (int j = 0; j < max; ++j)
			ASSERT(pages[j] != newPage);
		pages[i] = newPage;
	}
	for (int i = 0; i < max; ++i)
	{
		if (pages[i] != 0)
			allocator.freePage(pages[i]);
	}
}

DEF_TEST(virtualMemorySimpleTest)
{
	void* p = VirtualMemoryManager::system().alloc(PAGE_SIZE, VMM_READWRITE);
	kmemset(p, 0xAA, PAGE_SIZE);
	VirtualMemoryManager::system().free(p);
}

DEF_TEST(virtualMemoryTest)
{
	VirtualMemoryManager& vmm = VirtualMemoryManager::system();
	const int max = 256;
	int* p[max] = {};
	int ref[max] = {};
	size_t s[max] = {};
	int tag = 1;
	int* maxAddr = nullptr;
	for (int i = 0; i < max; ++i)
	{
		s[i] = PAGE_SIZE * ((i % 10) + 1);
		p[i] = static_cast<int*>(vmm.alloc(s[i], VMM_READWRITE));
		s[i] /= sizeof (int);
		if (p[i] > maxAddr)
			maxAddr = p[i];
		kfill(p[i], p[i] + s[i], tag);
		ref[i] = tag;
		tag++;
	}
	for (int i = 0; i < max; i += 2)
	{
		vmm.free(p[i]);
		p[i] = nullptr;
	}
	for (int i = 0; i < max; i += 4)
	{
		s[i] = PAGE_SIZE;
		p[i] = static_cast<int*>(vmm.alloc(s[i], VMM_READWRITE));
		s[i] /= sizeof (int);
		ASSERT(p[i] <= maxAddr);
		kfill(p[i], p[i] + s[i], tag);
		ref[i] = tag;
		tag++;
	}
	for (int i = 0; i < max; ++i)
	{
		if (p[i] != nullptr)
		{
			for (size_t j = 0; j < s[i]; ++j)
				ASSERT(p[i][j] == ref[i]);
			vmm.free(p[i]);
		}
	}
}

DEF_TEST(heapTest)
{
	const size_t maxMemory = 16 << 20;
	struct Reg
	{
		int* m_p;
		int m_tag;
		int m_regSize;
	};
	const size_t startVmSize = Heap::system().virtualMemorySize();
	kvector<Reg> regs;
	size_t memCnt = 0;
	int tag = 0;
	while (memCnt < maxMemory)
	{
		const int regSize = krand() % 10000;
		Reg reg{new int[regSize], ++tag, regSize};
		kfill(reg.m_p, reg.m_p + regSize, reg.m_tag);
		regs.push_back(reg);
		memCnt += regSize;
	}
	for (Reg& reg : regs)
	{
		if ((krand() % 2) == 0)
		{
			delete[] reg.m_p;
			reg.m_p = nullptr;
		}
	}
	for (Reg& reg : regs)
	{
		if (reg.m_p == nullptr)
		{
			const int regSize = krand() % 10000;
			reg.m_p = new int[regSize];
			reg.m_tag = ++tag;
			reg.m_regSize = regSize;
			kfill(reg.m_p, reg.m_p + regSize, reg.m_tag);
		}
	}
	for (Reg& reg : regs)
	{
		for (int i = 0; i < reg.m_regSize; ++i)
			ASSERT(reg.m_p[i] == reg.m_tag);
		delete[] reg.m_p;
	}
	regs.clear_and_free();
	const size_t endVmSize = Heap::system().virtualMemorySize();
	ASSERT(startVmSize == endVmSize);
}

DEF_TEST(klistTest)
{
	klist<int> lst;
	int sum = 0;
	for (int i = 0; i < 100; i++)
	{
		lst.push_back(i);
		sum += i;
	}
	for (auto it = lst.begin(); it != lst.end();)
	{
		if ((*it % 2) == 0)
		{
			sum -= *it;
			it = lst.erase(it);
		}
		else
			++it;
	}
	int testSum = 0;
	for (int val : lst)
		testSum += val;
	ASSERT(testSum == sum);

	struct A
	{

		A(const A& a)
		{
			m_cnt = a.m_cnt;
			(*m_cnt)++;
		}

		A(int* cnt)
			: m_cnt(cnt)
		{
			(*m_cnt)++;
		}

		~A()
		{
			(*m_cnt)--;
		}
		int* m_cnt = nullptr;
	};
	int cnt = 0;
	{
		klist<A> lst;
		for (int i = 0; i < 10; i++)
			lst.push_back(A(&cnt));
		ASSERT(cnt == 10);
		auto it = lst.begin();
		for (int i = 0; i < 5; i++)
			it = lst.erase(it);
		ASSERT(cnt == 5);
	}
	ASSERT(cnt == 0);
}

DEF_TEST(threadSimpleTest)
{
	bool run = false;
	kthread th([&run] {
		run = true;
	});
	th.join();
	ASSERT(run);
}

DEF_TEST(threadSleepTest)
{
	const TimePoint desiredDelayMs = 10;
	AbstractTimer* timer = AbstractTimer::system();
	for (int i = 0; i < 10; ++i)
	{
		const TimePoint beginTime = timer->fastTimepoint();
		sleepMs(desiredDelayMs);
		const TimePoint actualDelayMs = timer->toMilliseconds(timer->fastTimepoint() - beginTime);
		ASSERT(actualDelayMs >= desiredDelayMs);
	}
}

DEF_TEST(threadMultipleTest)
{
	const int numThreads = 10;
	const int numIterations = 1000000;
	const int expectedResult = numIterations * numThreads;
	std::atomic<int> sum{0};
	kvector<kthread> threads;
	for (int i = 0; i < numThreads; ++i)
	{
		threads.emplace_back([&sum] {
			for (int i = 0; i < numIterations; ++i)
				++sum;
		});
	}
	for (kthread& thread : threads)
		thread.join();
	ASSERT(sum == expectedResult);
}

DEF_TEST(threadFpuTest)
{
	double result = 0.;
	static const double sinArg = 3.14159265 / 4;
	kthread thread([&result] {
		result = std::sin(sinArg);
	});
	thread.join();
	ASSERT(std::sin(sinArg) == result);
}

DEF_TEST(mutexTest)
{
	static const int incIterations = 100000;
	static const int lockIterations = 10;
	std::atomic<bool> result{ true};
	std::atomic<uint64_t> value{ 0};
	kmutex mutex;
	auto proc = [&value, &result, &mutex](uint64_t add) {
		for (int lockIdx = 0; lockIdx < lockIterations; lockIdx++)
		{
			klock_guard lock(mutex);
			for (int i = 0; i < incIterations; ++i)
			{
				value += add;
				if ((value % add) != 0)
				{
					result = false;
					return;
				}
			}
		}
	};
	kthread th1(std::bind(proc, 1));
	kthread th2(std::bind(proc, 100));
	kthread th3(std::bind(proc, 10000));
	th1.join();
	th2.join();
	th3.join();
	ASSERT(result);
}

DEF_TEST(threadEventsWaitTest)
{
	static const int numEvents = 3;
	static const int singleSignaledEvent = numEvents - 1;
	kevent events[numEvents];
	kevent * eventPtrs[numEvents];
	for (int i = 0; i < numEvents; ++i)
		eventPtrs[i] = &events[i];
	kthread thread([&events] {
		sleepMs(10);
		events[singleSignaledEvent].set();
	});
	const int returnedSignaledEvent = kevent::waitMultiple(eventPtrs, std::size(eventPtrs), false, kevent::WaitInfinite);
	thread.join();
	ASSERT(returnedSignaledEvent == singleSignaledEvent);
}

DEF_TEST(threadEventsWaitAllTest)
{
	static const int numEvents = 3;
	kevent events[numEvents];
	kevent * eventPtrs[numEvents];
	kvector<kthread> threads;
	std::atomic<int> setCount{0};
	for (int i = 0; i < numEvents; ++i)
	{
		kevent& event = events[i];
		eventPtrs[i] = &event;
		threads.emplace_back([&event, &setCount] {
			sleepMs(10);
			event.set();
			++setCount;
		});
	}
	kevent::waitMultiple(eventPtrs, std::size(eventPtrs), true, kevent::WaitInfinite);
	EXPECT(setCount == numEvents);
	for (kthread& thread : threads)
		thread.join();
}

DEF_TEST(semaphoreTest)
{
	Semaphore sem(2, 2);
	sem.wait(2, Semaphore::WaitInfinite);
	std::atomic<bool> signaled{false};
	kthread th([&sem, &signaled] {
		sleepMs(10);
		sem.signal(2, nullptr);
		signaled = true;
	});
	sem.wait(1, Semaphore::WaitInfinite);
	EXPECT(signaled);
	sem.signal(1, nullptr);
	th.join();
}

DEF_TEST(conditionVariableTest)
{
	const int numProducers = 2;
	const int numConsumers = 2;
	static const int dataSize = 100000;
	static_assert(dataSize % numProducers == 0);
	static const int dataPerProducer = dataSize / numProducers;
	kcondition_variable cv;
	kmutex mutex;
	kvector<int> data;
	kvector<kthread> threads;
	bool exit = false;
	int popCount = 0;
	kevent ev;

	for (int i = 0; i < numConsumers; ++i)
	{
		threads.emplace_back([&exit, &cv, &mutex, &data, &popCount, &ev] {
			for (;;)
			{
				kunique_lock lock(mutex);
				while (data.empty() && !exit)
					cv.wait(lock);
				if (exit)
					return;

				data.pop_back();
				++popCount;
				if (popCount == dataSize)
					ev.set();
			}
		});
	}

	for (int i = 0; i < numProducers; ++i)
	{
		threads.emplace_back([&cv, &mutex, &data] {
			for (int i = 0; i < dataPerProducer; i++)
			{
				{
					klock_guard lock(mutex);
					data.push_back(0);
				}
				cv.notify_one();
			}
		});
	}
	ev.wait();
	{
		klock_guard lock(mutex);
		exit = true;
	}
	cv.notify_all();
	for (kthread& thread : threads)
		thread.join();

	ASSERT(popCount == dataSize);
	ASSERT(data.empty());
}

DEF_TEST(threadPoolTest)
{
	kevent ev;
	bool result = false;
	ThreadPool::system().run([&result, &ev] {
		result = true;
		ev.set();
	});
	ev.wait();
	ASSERT(result);
}

DEF_TEST(interruptMessageTest)
{
	class TestDevice : public AbstractDevice
	{
	public:
		TestDevice() 
			: AbstractDevice(DeviceClass::System, L"Test", AbstractDevice::root())
		{
			
		}
			
		bool test()
		{
			postInterruptMessage(m_refValue, 0, nullptr);
			m_ev.wait();
			return (m_testValue == m_refValue);
		}
		
		void onInterruptMessage(int arg1, int, void*)
		{
			m_testValue = m_refValue;
			m_ev.set();
		}

	private:
		kevent m_ev;
		int m_testValue = 0;
		const int m_refValue = 12345;
	} dev;
	
	ASSERT(dev.test());
}

DEF_TEST(smpTest)
{
	kvector<kthread> threads;
	const unsigned int numCpu = cpuLogicalCount();
	kmutex mutex;
	kvector<unsigned int> ids;
	ids.reserve(numCpu);
	std::atomic<unsigned int> cpuCnt{1};
	for (unsigned int cnt = 0; cnt < (numCpu - 1); cnt++)
	{
		threads.emplace_back([&ids, &mutex, &cpuCnt, numCpu] {
			unsigned int curCpuId = cpuCurrentId();
			{
				klock_guard lock(mutex);
				for (const unsigned int cpuId : ids)
				{
					if (cpuId == curCpuId)
						break;
				}
				ids.push_back(cpuCurrentId());
			}
			++cpuCnt;
			while (cpuCnt < numCpu)
				cpuPause();
		});
	}
	
	for (kthread& thread : threads)
		thread.join();
	ASSERT(cpuCnt == numCpu);
}

DEF_TEST(kunorderedMapTest)
{
	kunordered_map<kstring, kstring> m = { 
		{"a", "A"}, 
		{"b", "B"}, 
		{"c", "c"},
		{"d", "D"},
		{"e", "E"},
		{"e", "E"},
		{"f", "F"},
	};
	m.erase("d");
	m.emplace("c", "C");
	ASSERT(m.find("c")->second == "C");
	ASSERT(m.find("d") == m.end());
	size_t sz = 0;
	for (auto it = m.begin(); it != m.end(); ++it)
		++sz;
	ASSERT(sz == m.size());
}

void runTests()
{
	println(L"Start tests:");
	ramPagesTest();
	virtualMemorySimpleTest();
	virtualMemoryTest();
	heapTest();
	klistTest();
	threadSimpleTest();
	threadSleepTest();
	threadMultipleTest();
	threadFpuTest();
	mutexTest();
	threadEventsWaitTest();
	threadEventsWaitAllTest();
	semaphoreTest();
	conditionVariableTest();
	threadPoolTest();
	interruptMessageTest();
	smpTest();
	kunorderedMapTest();
	println(L"Tests completed ");
}
