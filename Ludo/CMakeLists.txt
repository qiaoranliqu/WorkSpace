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
        farmhash/farmhash.cc
        utils/ClientSock.cpp
        utils/hashutil.cc
        input/input_types.cpp
        )

set(HEADER_FILES
        common.h
        Base.h
        hash.h
        farmhash/farmhash.h
        utils/hashutil.h
        utils/ClientSock.h
        utils/json.hpp
        utils/debugbreak.h
        utils/common.h
        lfsr64.h
        disjointset.h
        control_plane.h
        Cuckoo/cuckoo.h
        input/input_types.h
        network/graph.h
        SetSep/bucket_map_to_group.h
        SetSep/setsep.h
        SetSep/DataPlaneSetSep.h
        Ludo/cuckoo_ht.h
        Ludo/minimal_perfect_cuckoo.h
        VacuumFilter/better_table.h
        VacuumFilter/bitsutil.h
        VacuumFilter/cuckoo_filter.h
        VacuumFilter/debug.h
        VacuumFilter/packedtable.h
        VacuumFilter/permencoding.h
        VacuumFilter/printutil.h
        VacuumFilter/simd-block.h
        VacuumFilter/singletable.h
        VacuumFilter/vacuum_filter.h
        SingleLudo.h
        MultiLudo.h
        MyDesign.h
        )

add_executable(testCuckoo
        ${HEADER_FILES}
        ${COMMON_SOURCE_FILES}
        testCuckoo.cpp)

add_executable(bl
        ${HEADER_FILES}
        ${COMMON_SOURCE_FILES}
        bl.cpp)

add_executable(mkdata
        ${HEADER_FILES}
        ${COMMON_SOURCE_FILES}
        mkdata.cpp)

IF (APPLE)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ELSE ()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(testCuckoo Threads::Threads)
    target_link_libraries(bl Threads::Threads)
    target_link_libraries(mkdata Threads::Threads)
ENDIF ()


set(Gperftools_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake/")
find_package(Gperftools REQUIRED)
target_link_libraries(testCuckoo ${GPERFTOOLS_PROFILER})
target_link_libraries(bl ${GPERFTOOLS_PROFILER})
target_link_libraries(mkdata ${GPERFTOOLS_PROFILER})

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)
target_link_libraries(testCuckoo ${OPENSSL_LIBRARIES})
target_link_libraries(bl ${OPENSSL_LIBRARIES})
target_link_libraries(mkdata ${OPENSSL_LIBRARIES})

#TARGET_LINK_LIBRARIES(testCuckoo LINK_PUBLIC ${Boost_LIBRARIES})
#TARGET_LINK_LIBRARIES(mkdata LINK_PUBLIC ${Boost_LIBRARIES})
#TARGET_LINK_LIBRARIES(bl LINK_PUBLIC ${Boost_LIBRARIES})

#TARGET_LINK_LIBRARIES(testCuckoo cmph)
#TARGET_LINK_LIBRARIES(mkdata cmph)
#TARGET_LINK_LIBRARIES(bl cmph)
