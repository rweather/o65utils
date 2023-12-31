cmake_minimum_required(VERSION 3.5)
include(CheckIncludeFiles)
include(CheckLibraryExists)

# Set the project name and version number.
project(o65utils VERSION 0.1.0 LANGUAGES C)

# Set up the type of build and the compiler flags to use.
if(NOT CMAKE_BUILD_TYPE)
    if(COVERAGE)
        set(CMAKE_BUILD_TYPE Debug)
    else()
        set(CMAKE_BUILD_TYPE Release)
    endif()
endif()
set(CMAKE_C_FLAGS "-Wall -Wextra ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O2")

# Require the c99 standard to compile C code.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Need libelf to build elf2o65.
check_include_files(elf.h HAVE_ELF_H)
check_include_files(libelf.h HAVE_LIBELF_H)
check_library_exists(elf elf_begin "" HAVE_LIBELF)

# Set up the main include directory.
include_directories(include)

# Add the subdirectories.
add_subdirectory(lib)
add_subdirectory(dump)
add_subdirectory(reloc)
if(HAVE_ELF_H AND HAVE_LIBELF_H AND HAVE_LIBELF)
    add_subdirectory(elf2o65)
endif()
