cmake_minimum_required(VERSION 3.0)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

project(bunsan_bacs_system_single)

if(BunsanCMakeBuiltin)
    BunsanCMakeLoad()
else()
    find_package(BunsanCMake CONFIG REQUIRED)
endif()

bunsan_add_library(${PROJECT_NAME}
    src/check.cpp
    src/file.cpp
    src/main.cpp
    src/test/storage.cpp
    src/tester_util.cpp
    src/worker.cpp
)
bunsan_use_bunsan_package(${PROJECT_NAME} bunsan_bacs_problem_single
    bunsan_bacs_problem_single
)
bunsan_use_bunsan_package(${PROJECT_NAME} bunsan_bacs_system
    bunsan_bacs_system
)
bunsan_use_bunsan_package(${PROJECT_NAME} bunsan_protobuf
    bunsan_protobuf
)

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME})
bunsan_install_project()

bunsan_include_tests()
