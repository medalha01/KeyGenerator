cmake_minimum_required(VERSION 3.10)
project(RNG_Benchmark CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define source files
set(SOURCE_FILES
    src/main.cpp
    src/pseudo_rng/mersenne_twister.cpp
    src/pseudo_rng/naor_reingold_prf.cpp
    src/primality_test/fermat_test.cpp
    src/primality_test/miller_rabin_test.cpp
    src/key_generator.cpp


)

# Add the executable
add_executable(rng_benchmark ${SOURCE_FILES})

# Specify include directories
target_include_directories(rng_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/pseudo_rng
)

# Link necessary libraries (often not needed for basic C++ features like chrono)
# target_link_libraries(rng_benchmark PRIVATE)
