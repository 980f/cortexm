cmake_minimum_required(VERSION 3.30)
project(bitbandero)

#need c++20 for std::countl_zero<T>
set(CMAKE_CXX_STANDARD 17)

add_executable(bitbandero ../bitbandcalculator.cpp ../bitbanding.h)

target_include_directories(bitbandero PUBLIC .. ../../ezcpp)  #bitbasher.h
