# Minimum CMake version
cmake_minimum_required(VERSION 3.12)

# Project definition
project(RNG_Benchmark LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Build Type Handling ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

# --- Add the Executable Target ---
set(SOURCE_FILES
    src/main.cpp
    src/pseudo_rng/mersenne_twister.cpp
    src/pseudo_rng/naor_reingold_prf.cpp
    src/primality_test/fermat_test.cpp
    src/primality_test/miller_rabin_test.cpp
    src/key_generator.cpp
)
add_executable(rng_benchmark ${SOURCE_FILES})

# --- Target Properties: Include Directories ---
target_include_directories(rng_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pseudo_rng
    ${CMAKE_CURRENT_SOURCE_DIR}/src/primality_test
)

# --- Target Properties: Compiler Flags (REVISED) ---

# Common Warnings (Applied regardless of build type)
target_compile_options(rng_benchmark PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# --- Release/RelWithDebInfo specific flags ---
target_compile_options(rng_benchmark PRIVATE
    # GNU/Clang Release Optimizations
    $<$<AND:$<CONFIG:Release,RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang>>:-O3;-pipe;-DNDEBUG>
    # MSVC Release Optimizations
    $<$<AND:$<CONFIG:Release,RelWithDebInfo>,$<CXX_COMPILER_ID:MSVC>>:/O2;/DNDEBUG>
)

# --- Debug specific flags ---
target_compile_options(rng_benchmark PRIVATE
    # GNU/Clang Debug
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU,Clang>>:-O0;-g>
    # MSVC Debug
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/Od;/Zi>
)


# --- Optional Performance Features (REVISED) ---
option(ENABLE_NATIVE_TUNING "Enable CPU-specific native optimizations (-march=native or /arch:AVX2)" ON)
option(ENABLE_LTO "Enable Link-Time Optimization (-flto or /GL)" ON)

if(ENABLE_NATIVE_TUNING)
    message(STATUS "Native Tuning Enabled: ON")
    target_compile_options(rng_benchmark PRIVATE
        # Apply only for Release/RelWithDebInfo builds
        # GNU/Clang Native Tuning
        $<$<AND:$<CONFIG:Release,RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang>>:-march=native>
        # MSVC Native Tuning
        $<$<AND:$<CONFIG:Release,RelWithDebInfo>,$<CXX_COMPILER_ID:MSVC>>:/arch:AVX2>
    )
else()
    message(STATUS "Native Tuning Enabled: OFF")
endif()

# --- LTO Handling (Keep as before, the check seemed okay) ---
set(IPO_SUPPORTED_BY_CMAKE FALSE)
if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT error)
    if(result)
        message(STATUS "Link-Time Optimization (LTO/IPO) Enabled: ON (Supported)")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON) # Enable LTO for Release/RelWithDebInfo
        set(IPO_SUPPORTED_BY_CMAKE TRUE)
    else()
        message(WARNING "Link-Time Optimization (LTO/IPO) Enabled: ON (But check failed: ${error}) LTO might not work during build.")
    endif()
else()
     message(STATUS "Link-Time Optimization (LTO/IPO) Enabled: OFF")
endif()

# --- Find Dependencies ---
find_package(Boost 1.70 QUIET REQUIRED COMPONENTS system)
if(Boost_FOUND)
    message(STATUS "Found Boost version ${Boost_VERSION_STRING} in ${Boost_INCLUDE_DIRS}")
    target_link_libraries(rng_benchmark PRIVATE Boost::boost)
endif()

# --- Add other libraries if needed ---
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(Threads QUIET)
    if(Threads_FOUND)
      message(STATUS "Found Threads library, linking...")
      target_link_libraries(rng_benchmark PRIVATE Threads::Threads)
    else()
       message(WARNING "Threads library not found, std::thread might not link correctly.")
    endif()
endif()

# --- Installation ---
install(TARGETS rng_benchmark RUNTIME DESTINATION bin)

# --- Testing ---
enable_testing()
add_test(NAME MyBenchmarkTest COMMAND $<TARGET_FILE:rng_benchmark> --some-test-args)

# --- Build Output Summary ---
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Boost Include Dirs: ${Boost_INCLUDE_DIRS}")