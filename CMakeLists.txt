cmake_minimum_required(VERSION 3.14)
project(OS3)

set(CMAKE_CXX_STANDARD 17)

add_executable(OS3 kill_test.c kill_wrapper.h
        intercept.c)