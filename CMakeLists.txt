cmake_minimum_required(VERSION 3.13)
project(main)

set(CMAKE_CXX_STANDARD 20)

find_package(yaml-cpp REQUIRED)

include_directories(include/)

add_executable(main
    src/main.cpp
    src/yaml_reader.cpp
    src/script_writer.cpp
    src/tracer_logic.cpp
    src/command.cpp
    src/terminal.cpp
)
target_link_libraries(
    main
    yaml-cpp
)