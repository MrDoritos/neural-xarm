cmake_minimum_required(VERSION 3.10)

project(neural-xarm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(NEURAL_XARM_NAME neural_xarm)
set(NEURAL_XARM_FILES assets)
set(NEURAL_XARM_SOURCE_DIR src)
set(NEURAL_XARM_WIDGETS_LIBRARY widgets)
set(NEURAL_XARM_RESOURCES_LIBRARY resources)

set(GUILIB_LIBRARY guilib)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(${NEURAL_XARM_SOURCE_DIR}/${GUILIB_LIBRARY})

add_executable(${NEURAL_XARM_NAME}
    ${NEURAL_XARM_SOURCE_DIR}/main.cpp
    ${NEURAL_XARM_SOURCE_DIR}/segment.cpp
    ${NEURAL_XARM_SOURCE_DIR}/robot_interface.cpp
    ${NEURAL_XARM_SOURCE_DIR}/joystick.cpp
    ${NEURAL_XARM_SOURCE_DIR}/kinematics.cpp
    ${NEURAL_XARM_SOURCE_DIR}/xarm_common.cpp
    ${NEURAL_XARM_SOURCE_DIR}/segment_loader.cpp
    ${NEURAL_XARM_SOURCE_DIR}/csv.cpp
    ${NEURAL_XARM_SOURCE_DIR}/robot.cpp
)

find_path(hidapi_INCLUDE_DIR hidapi/hidapi.h)

find_library(hidapi_LIBRARY hidapi-libusb)

target_include_directories(${NEURAL_XARM_NAME} PUBLIC 
    ${hidapi_INCLUDE_DIR}
    ${GUILIB_INCLUDE_DIR}
)

target_link_libraries(${NEURAL_XARM_NAME}
    ${hidapi_LIBRARY}
    ${GUILIB_LIBRARY}
)

target_compile_options(${NEURAL_XARM_NAME} PUBLIC
    -g #-O2
    #-v 
    #-Q
    #-ftime-report
)

add_custom_command(TARGET ${NEURAL_XARM_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/${NEURAL_XARM_FILES}/"
        "${CMAKE_BINARY_DIR}/${NEURAL_XARM_FILES}"
    COMMENT "Copying files"
    VERBATIM
)

add_custom_target(copyall
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/${NEURAL_XARM_FILES}/"
        "${CMAKE_BINARY_DIR}/${NEURAL_XARM_FILES}"
    COMMENT "Copying files"
    VERBATIM
)