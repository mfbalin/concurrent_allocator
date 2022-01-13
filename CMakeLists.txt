cmake_minimum_required(VERSION 3.16)

# ----------------------------------------------------------------------------
# Project
# ----------------------------------------------------------------------------
project (concurrent_allocator
         LANGUAGES CXX)

include (GNUInstallDirs)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -pedantic -DDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DDEBUG -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -O3")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------------------------------------------------------
# C/CXX Flags and Standards
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------
# Other flags
# -------------------------------------------------------------

# ----------------------------------------------------------------------------
# Depedencies
# ----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

# ----------------------------------------------------------------------------
# Src Files and Targets
# ----------------------------------------------------------------------------

include_directories(
        PUBLIC
        include
)

link_libraries(
        Threads::Threads
        stdc++fs
        OpenMP::OpenMP_CXX
)

add_executable(main src/main.cpp)

# ----------------------------------------------------------------------------
# Tests
# ----------------------------------------------------------------------------
# enable_testing ()
# 
# add_subdirectory (test)
