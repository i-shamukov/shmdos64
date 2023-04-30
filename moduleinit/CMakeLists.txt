set(HEADERS
)

set(SOURCES
    moduleinit.cpp
)

add_library(moduleinit STATIC ${HEADERS} ${SOURCES})
kernel_static_lib(moduleinit)