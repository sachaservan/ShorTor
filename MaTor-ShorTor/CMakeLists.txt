cmake_minimum_required(VERSION 3.0)
project(mator_project)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(OperatingSystem "Linux")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(OperatingSystem "MacOSX")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set(RELEASE_FOLDER Release)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${RELEASE_FOLDER}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${RELEASE_FOLDER}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${RELEASE_FOLDER}/lib)

configure_file(data/config.json "${CMAKE_BINARY_DIR}/config.json" COPYONLY)

add_definitions("-std=c++11 -mfpmath=sse -Ofast -fPIC -march=native -funroll-loops -funsafe-math-optimizations -I /usr/local/include")

add_subdirectory(src)

set(MAKE_TESTS true)
set(MAKE_PYTHON true)

if(MAKE_TESTS)
        # Setup CMake to run tests
        enable_testing()
        
        # Prepare ourselves for compiling boost
        find_package(Boost COMPONENTS unit_test_framework REQUIRED)
        include_directories (${Boost_INCLUDE_DIRS} src)
        add_definitions("-DSINGLE_TEST_EXECUTABLES=1")
        
        # Get test sources
        file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
        
        # Run through each source
        foreach(testSrc ${TEST_SRCS})
                # Extract the filename without an extension (NAME_WE)
                get_filename_component(testName ${testSrc} NAME_WE)
        
                # Add compile target
                add_executable(${testName} ${testSrc})
        
                # Link to Boost libraries AND MATor (TODO: add MATor)
                target_link_libraries(${testName} mator ${Boost_LIBRARIES})
        
                # Keep test binaries in Testing/Bin
                set_target_properties(${testName} PROPERTIES 
                    RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/Testing/Bin)
        
                # Finally add it to test execution
                add_test(NAME ${testName} 
                         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Testing/Bin 
                         COMMAND ${CMAKE_BINARY_DIR}/Testing/Bin/${testName})
        endforeach(testSrc)
        
        # Copy data to Realse folder 
        file(COPY data DESTINATION ${CMAKE_BINARY_DIR}/${RELEASE_FOLDER})

        # Copy necessary data files for tests
        file(COPY test/Data DESTINATION ${CMAKE_BINARY_DIR}/Testing/)
endif()



if(MAKE_PYTHON)
    # Add a CMake parameter for choosing a desired Python version
    set(EXAMPLE_PYTHON_VERSION  CACHE STRING "Python version to use for compiling the mator_project library")
    include(CheckCXXCompilerFlag)
    add_definitions("-DUSE_PYTHON=1")

    # Try to autodetect Python (can be overridden manually if needed)
    set(Python_ADDITIONAL_VERSIONS 3.4 3.5 3.6 3.7)
    if (NOT ${EXAMPLE_PYTHON_VERSION} STREQUAL "")
      find_package(PythonLibs ${EXAMPLE_PYTHON_VERSION} EXACT)
      if (NOT PythonLibs_FOUND)
        find_package(PythonLibs ${EXAMPLE_PYTHON_VERSION} REQUIRED)
      endif()
    else()
      find_package(PythonLibs REQUIRED)
    endif()

    # Include path for Python header files
    include_directories(${PYTHON_INCLUDE_DIR})
    # Include path for pybind11 header files -- this may need to be changed depending on your setup
    include_directories(${PROJECT_SOURCE_DIR}/pybind11/include)

    add_library(pymator SHARED
        src/mator.cpp
        src/ip.cpp
        src/utils.cpp
        src/relay.cpp
        src/db_connection.cpp
        src/consensus.cpp
        src/types/work_manager.cpp
        src/relationship_manager.cpp
        src/path_selection_standard.cpp
        src/tor_like.cpp
        src/ps_tor.cpp
	src/ps_uniform.cpp
	src/ps_selektor.cpp
        src/ps_distributor.cpp
        src/ps_lastor.cpp
        src/scenario.cpp
        src/costmap.cpp
        src/pcf.cpp
        src/pcf_parser.cpp
        src/asmap.cpp
        src/generic_worst_case_anonymity.cpp
        src/generic_precise_anonymity.cpp
        src/pythonbinding.cpp
    )
    find_package(SQLITE3 REQUIRED)
    include_directories( ${SQLITE3_INCLUDE_DIRS} )
    add_definitions(-DSOURCE_DIRECTORY_=${CMAKE_SOURCE_DIR}/src)
    target_link_libraries ( pymator
        simple_json
        ${SQLITE3_LIBRARIES}
    )
    # Don't add a 'lib' prefix to the shared library
    set_target_properties(pymator PROPERTIES PREFIX "")

    if (WIN32)
      if (MSVC)
        # Enforce size-based optimization and link time code generation
        # on MSVC (~30% smaller binaries in experiments). /bigobj is needed
        # for bigger binding projects due to the limit to 64k addressable sections
        # /MP enables multithreaded builds (relevant when there are many files).
        set_target_properties(pymator PROPERTIES COMPILE_FLAGS "/Os /GL /MP /bigobj")
        set_target_properties(pymator PROPERTIES LINK_FLAGS "/LTCG")
      endif()

      # .PYD file extension on Windows
      set_target_properties(pymator PROPERTIES SUFFIX ".pyd")

      # Link against the Python shared library
      target_link_libraries(pymator ${PYTHON_LIBRARY})
    elseif (UNIX)
      # It's quite common to have multiple copies of the same Python version
      # installed on one's system. E.g.: one copy from the OS and another copy
      # that's statically linked into an application like Blender or Maya.
      # If we link our plugin library against the OS Python here and import it
      # into Blender or Maya later on, this will cause segfaults when multiple
      # conflicting Python instances are active at the same time (even when they
      # are of the same version).

      # Windows is not affected by this issue since it handles DLL imports
      # differently. The solution for Linux and Mac OS is simple: we just don't
      # link against the Python library. The resulting shared library will have
      # missing symbols, but that's perfectly fine -- they will be resolved at
      # import time.

      # .SO file extension on Linux/Mac OS
      set_target_properties(pymator PROPERTIES SUFFIX ".so")

      # Strip unnecessary sections of the binary on Linux/Mac OS
      if(APPLE)
        set_target_properties(pymator PROPERTIES MACOSX_RPATH ".")
        set_target_properties(pymator PROPERTIES LINK_FLAGS "-undefined dynamic_lookup ")
        if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
          add_custom_command(TARGET pymator POST_BUILD COMMAND strip -u -r ${PROJECT_BINARY_DIR}/example.so)
        endif()
      else()
        if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
          add_custom_command(TARGET pymator POST_BUILD COMMAND strip ${PROJECT_BINARY_DIR}/example.so)
        endif()
      endif()
    endif()
endif()
