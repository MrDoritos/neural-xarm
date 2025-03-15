cmake_minimum_required(VERSION 3.10)

project(neural-xarm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(COMPAT)
    if(NOT DEFINED COMPAT_PATH)
        find_path(COMPAT_PATH g++-13)
    endif()
    set(CXX ${COMPAT_PATH}/g++-13)
    set(CC ${COMPAT_PATH}/gcc-13)
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,${COMPAT_PATH}/../lib64")
endif()

set(NEURAL_XARM_NAME neural_xarm)
set(NEURAL_XARM_FILES shaders assets)
set(NEURAL_XARM_SOURCE_DIR src)
set(NEURAL_XARM_INCLUDE_DIR include)
set(NEURAL_XARM_WIDGETS_LIBRARY widgets)
set(NEURAL_XARM_RESOURCES_LIBRARY resources)

set(THIRDPARTY_INCLUDE_DIR thirdparty)

add_subdirectory(${THIRDPARTY_INCLUDE_DIR}/tinyobjloader)

add_library(${NEURAL_XARM_WIDGETS_LIBRARY}
    ${NEURAL_XARM_SOURCE_DIR}/ui_element.cpp
    ${NEURAL_XARM_SOURCE_DIR}/ui_text.cpp
    ${NEURAL_XARM_SOURCE_DIR}/ui_slider.cpp
    ${NEURAL_XARM_SOURCE_DIR}/ui_toggle.cpp
)

add_library(${NEURAL_XARM_RESOURCES_LIBRARY}
    ${NEURAL_XARM_SOURCE_DIR}/text.cpp
    ${NEURAL_XARM_SOURCE_DIR}/camera.cpp
    ${NEURAL_XARM_SOURCE_DIR}/texture.cpp
    ${NEURAL_XARM_SOURCE_DIR}/shader.cpp
    ${NEURAL_XARM_SOURCE_DIR}/shader_program.cpp
    ${NEURAL_XARM_SOURCE_DIR}/mesh.cpp
)

add_executable(${NEURAL_XARM_NAME}
    ${NEURAL_XARM_SOURCE_DIR}/main.cpp
    ${NEURAL_XARM_SOURCE_DIR}/common.cpp
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

target_include_directories(${NEURAL_XARM_RESOURCES_LIBRARY} PUBLIC 
    ${NEURAL_XARM_INCLUDE_DIR}
    ${GL_INCLUDE_DIR}
    ${GLES3_INCLUDE_DIR}
    ${EGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${hidapi_INCLUDE_DIR}
    ${THIRDPARTY_INCLUDE_DIR}
)

target_link_libraries(${NEURAL_XARM_WIDGETS_LIBRARY}
    ${tinyobjloader_LIBRARY}
    ${NEURAL_XARM_RESOURCES_LIBRARY}
)

target_link_libraries(${NEURAL_XARM_NAME}
    ${GL_LIBRARY}
    ${GLFW_LIBRARY}
    ${hidapi_LIBRARY}
    ${tinyobjloader_LIBRARY}
    ${NEURAL_XARM_WIDGETS_LIBRARY}
    ${NEURAL_XARM_RESOURCES_LIBRARY}
)

target_compile_options(${NEURAL_XARM_NAME} PUBLIC
    -g -O2
)

target_compile_options(${NEURAL_XARM_RESOURCES_LIBRARY} PUBLIC
    -g -O2
)

target_compile_options(${NEURAL_XARM_WIDGETS_LIBRARY} PUBLIC
    -g -O2
)

file(COPY ${NEURAL_XARM_FILES} DESTINATION ${CMAKE_BINARY_DIR})