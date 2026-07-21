cmake_minimum_required(VERSION 3.16)
project(lobV3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lobV3
    src/main.cpp
    src/OrderBook/OrderBook.cpp
    src/Order/Order.cpp
    src/Order/LimitOrder.cpp
    src/Order/PriceLevel.cpp
    src/OrderBook/PriceTimeOrderMatchingStrategy.cpp
)

target_include_directories(lobV3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(benchmark
    benchmark.cpp
    src/OrderBook/OrderBook.cpp
    src/Order/Order.cpp
    src/Order/LimitOrder.cpp
    src/Order/PriceLevel.cpp
    src/OrderBook/PriceTimeOrderMatchingStrategy.cpp
)

target_include_directories(benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Release-mode optimizations matter a lot for latency numbers.
# If you don't pass -DCMAKE_BUILD_TYPE=Release on the cmake command,
# default to a sane optimization level anyway.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()