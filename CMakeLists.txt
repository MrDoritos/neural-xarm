cmake_minimum_required(VERSION 3.10)

project(neural-xarm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(NEURAL_XARM_NAME neural_xarm)
set(NEURAL_XARM_FILES shaders assets)
set(NEURAL_XARM_SOURCE_DIR src)
set(NEURAL_XARM_INCLUDE_DIR include)

set(THIRDPARTY_INCLUDE_DIR thirdparty)

add_subdirectory(${THIRDPARTY_INCLUDE_DIR}/tinyobjloader)

add_executable(${NEURAL_XARM_NAME}
    ${NEURAL_XARM_SOURCE_DIR}/main.cpp
)

find_path(GL_INCLUDE_DIR GL/gl.h)
find_path(GLES3_INCLUDE_DIR GLES3/gl3.h)
find_path(EGL_INCLUDE_DIR EGL/egl.h)
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)
find_path(GLM_INCLUDE_DIR glm/glm.hpp)
find_path(hidapi_INCLUDE_DIR hidapi/hidapi.h)
find_path(tinyobjloader_INCLUDE_DIR tiny_obj_loader.h)

find_library(GL_LIBRARY GL)
find_library(GLFW_LIBRARY glfw)
find_library(hidapi_LIBRARY hidapi-libusb)
find_library(tinyobjloader_LIBRARY tinyobjloader)

include_directories(
    ${NEURAL_XARM_INCLUDE_DIR}
    ${GL_INCLUDE_DIR}
    ${GLES3_INCLUDE_DIR}
    ${EGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${hidapi_INCLUDE_DIR}
    ${THIRDPARTY_INCLUDE_DIR}
)

target_link_libraries(${NEURAL_XARM_NAME}
    ${GL_LIBRARY}
    ${GLFW_LIBRARY}
    ${hidapi_LIBRARY}
    ${tinyobjloader_LIBRARY}
)

target_compile_options(${NEURAL_XARM_NAME} PRIVATE
    -g
)

file(COPY ${NEURAL_XARM_FILES} DESTINATION ${CMAKE_BINARY_DIR})