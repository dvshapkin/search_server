cmake_minimum_required(VERSION 3.5)

project(sprint_5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sprint_5
    main.cpp
    utils.h utils.cpp
    document.h document.cpp
    searchserver.h searchserver.cpp
    requestqueue.h requestqueue.cpp
    paginator.h
    log_duration.h)
