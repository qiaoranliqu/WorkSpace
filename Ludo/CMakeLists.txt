cmake_minimum_required(VERSION 3.5)
project(KVSystem)

# sudo apt-mark hold *grub*
# sudo apt-get install google-perftools libgoogle-perftools-dev cmake build-essential gdb libssl-dev pkgconf tmux clang liblua5.3-dev
# cd ~/MinimalPerfectCuckoo ; mkdir release ; cd release ; cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -G "CodeBlocks - Unix Makefiles" .. ; make -j8

link_directories(/usr/local/lib)
#link_directories(cmph)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mavx -maes")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mavx -maes")

IF ((CMAKE_BUILD_TYPE MATCHES Debug) OR (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPROFILE ")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPROFILE ")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb ")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb ")
ENDIF ((CMAKE_BUILD_TYPE MATCHES Debug) OR (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo))

IF ((CMAKE_BUILD_TYPE MATCHES Debug))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFULL_DEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-no-as-needed -ggdb -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFULL_DEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-no-as-needed -ggdb -O0")
ENDIF ((CMAKE_BUILD_TYPE MATCHES Debug))

IF ((CMAKE_BUILD_TYPE MATCHES Release) OR (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -DNDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -DNDEBUG")
ENDIF ((CMAKE_BUILD_TYPE MATCHES Release) OR (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo))

#FIND_PACKAGE(Boost 1.65 COMPONENTS REQUIRED)

include_directories(
        #        ${Boost_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set(COMMON_SOURCE_FILES
        common.cpp
        Cuckoo/farmhash/farmhash.cc
        Cuckoo/utils/ClientSock.cpp
        Cuckoo/utils/hashutil.cc
        )

set(HEADER_FILES
        common.h
        Base.h
        Cuckoo/hash.h
        Cuckoo/farmhash/farmhash.h
        Cuckoo/utils/hashutil.h
        Cuckoo/lfsr64.h
        Cuckoo/disjointset.h
        Cuckoo/cuckoo.h
        MyDesign.h
        Cuckoo/utils/ClientSock.h
        Cuckoo/utils/json.hpp
        Cuckoo/utils/debugbreak.h
        )

add_executable(test
        ${HEADER_FILES}
        ${COMMON_SOURCE_FILES}
        testCuckoo.cpp)

IF (APPLE)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ELSE ()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(validity Threads::Threads)
    target_link_libraries(microbenchmarks Threads::Threads)
    target_link_libraries(sideExperiments Threads::Threads)
ENDIF ()


set(Gperftools_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake/")
find_package(Gperftools REQUIRED)
target_link_libraries(testCuckoo ${GPERFTOOLS_PROFILER})

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)
target_link_libraries(testCuckoo ${OPENSSL_LIBRARIES})

#TARGET_LINK_LIBRARIES(testCuckoo LINK_PUBLIC ${Boost_LIBRARIES})

#TARGET_LINK_LIBRARIES(testCuckoo cmph)
