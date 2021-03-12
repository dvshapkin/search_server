cmake_minimum_required(VERSION 3.5)

project(sprint_5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sprint_5
    main.cpp
    read_input_functions.h read_input_functions.cpp
    string_processing.h string_processing.cpp
    document.h document.cpp
    searchserver.h searchserver.cpp
    requestqueue.h requestqueue.cpp
    remove_duplicates.h remove_duplicates.cpp
    test_example_functions.h test_example_functions.cpp
    paginator.h
    log_duration.h)
