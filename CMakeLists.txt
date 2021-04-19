cmake_minimum_required(VERSION 3.19)

project(search_server)

set(CMAKE_CXX_STANDARD 17)

add_executable(search_server
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
