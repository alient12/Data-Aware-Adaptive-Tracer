cmake_minimum_required(VERSION 3.13)
project(BPFNexus)

set(CMAKE_CXX_STANDARD 20)

# Find YAML-CPP
find_package(yaml-cpp REQUIRED)

# Add Clang libraries Manually
include_directories(/usr/lib/llvm-18/include)
link_directories(/usr/lib/llvm-18/lib)

include_directories(include/)

add_executable(main
    src/main.cpp
    src/yaml_reader.cpp
    src/script_writer.cpp
    src/tracer_logic.cpp
    src/command.cpp
    src/terminal.cpp
    src/bpflog_reader.cpp
    src/distribution.cpp
    src/function_parser.cpp
)
target_link_libraries(
    main
    PRIVATE
    yaml-cpp
    clang
)