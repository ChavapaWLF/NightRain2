cmake_minimum_required(VERSION 3.10)
project(GLFWDependencyTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenGL
find_package(OpenGL REQUIRED)

# 设置GLEW路径
set(GLEW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32.a")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/glew32.a")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32.lib")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/glew32.lib")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/libglew32.a")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libglew32.a")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32s.lib")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/glew32s.lib")
else()
    message(FATAL_ERROR "找不到GLEW库文件。请确保glew32.a, glew32.lib, libglew32.a或glew32s.lib中的一个存在于'${CMAKE_SOURCE_DIR}/lib'目录中。")
endif()

# 设置GLFW路径
set(GLFW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
set(GLFW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libglfw3.a")

# 设置SDL2路径
set(SDL2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include/SDL2")

# 查找SDL2库文件
if(WIN32)
    # MinGW使用.a文件
    if(MINGW)
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2.a")
            set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libSDL2.a")
            if(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2main.a")
                set(SDL2_LIBRARIES ${SDL2_LIBRARIES} "${CMAKE_SOURCE_DIR}/lib/libSDL2main.a")
            endif()
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2.dll.a")
            set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libSDL2.dll.a")
            if(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2main.a")
                set(SDL2_LIBRARIES ${SDL2_LIBRARIES} "${CMAKE_SOURCE_DIR}/lib/libSDL2main.a")
            endif()
        else()
            message(FATAL_ERROR "找不到SDL2库文件")
        endif()
        
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.a")
            set(SDL2_MIXER_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.a")
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.dll.a")
            set(SDL2_MIXER_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.dll.a")
        else()
            message(FATAL_ERROR "找不到SDL2_mixer库文件")
        endif()
    else()
        # MSVC使用.lib文件
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/SDL2.lib")
            set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/SDL2.lib")
            if(EXISTS "${CMAKE_SOURCE_DIR}/lib/SDL2main.lib")
                set(SDL2_LIBRARIES ${SDL2_LIBRARIES} "${CMAKE_SOURCE_DIR}/lib/SDL2main.lib")
            endif()
        else()
            message(FATAL_ERROR "找不到SDL2库文件")
        endif()
        
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/SDL2_mixer.lib")
            set(SDL2_MIXER_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/SDL2_mixer.lib")
        else()
            message(FATAL_ERROR "找不到SDL2_mixer库文件")
        endif()
    endif()
else()
    # Unix系统可以使用pkg-config
    include(FindPkgConfig)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES})
    set(SDL2_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARIES})
endif()

# 包含目录
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

# 添加测试可执行文件
add_executable(glfw_test glfw_test.cpp)

# 设置Windows系统下的子系统
if(WIN32)
    set_target_properties(glfw_test PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# 链接库
target_link_libraries(glfw_test
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    -mconsole  # 使用控制台子系统
)

# 添加平台特定库
if(WIN32)
    target_link_libraries(glfw_test 
        winmm
        gdi32
        user32
        shell32
        imm32
        version
        setupapi
    )
elseif(APPLE)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(glfw_test
        ${CORE_FOUNDATION_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
    )
else()
    target_link_libraries(glfw_test
        dl
        pthread
    )
endif()

# 复制DLL文件
if(WIN32)
    # 对于GLEW
    if(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32.dll")
        add_custom_command(TARGET glfw_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/lib/glew32.dll"
                $<TARGET_FILE_DIR:glfw_test>)
    endif()
    
    # 对于SDL2
    foreach(SDL_DLL_PATH 
        "${CMAKE_SOURCE_DIR}/lib/SDL2.dll"
        "${CMAKE_SOURCE_DIR}/bin/SDL2.dll"
        "C:/msys64/mingw64/bin/SDL2.dll")
        if(EXISTS "${SDL_DLL_PATH}")
            add_custom_command(TARGET glfw_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SDL_DLL_PATH}"
                    $<TARGET_FILE_DIR:glfw_test>)
            break()
        endif()
    endforeach()
    
    # 对于SDL2_mixer
    foreach(SDL_MIXER_DLL_PATH 
        "${CMAKE_SOURCE_DIR}/lib/SDL2_mixer.dll"
        "${CMAKE_SOURCE_DIR}/bin/SDL2_mixer.dll"
        "C:/msys64/mingw64/bin/SDL2_mixer.dll")
        if(EXISTS "${SDL_MIXER_DLL_PATH}")
            add_custom_command(TARGET glfw_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SDL_MIXER_DLL_PATH}"
                    $<TARGET_FILE_DIR:glfw_test>)
            break()
        endif()
    endforeach()
endif()

# 显示项目状态
message(STATUS "GLEW库: ${GLEW_LIBRARIES}")
message(STATUS "GLFW库: ${GLFW_LIBRARIES}")
message(STATUS "OpenGL库: ${OPENGL_LIBRARIES}")
message(STATUS "SDL2库: ${SDL2_LIBRARIES}")
message(STATUS "SDL2_mixer库: ${SDL2_MIXER_LIBRARIES}")