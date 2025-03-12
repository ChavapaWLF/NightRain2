cmake_minimum_required(VERSION 3.10)
project(ColorfulRainSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenGL (通常在Windows上能正常工作)
find_package(OpenGL REQUIRED)

# 手动设置GLEW路径，而不依赖find_package
set(GLEW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
# 自动查找可用的GLEW库文件
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32.a")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/glew32.a")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32.lib")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/glew32.lib")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/libglew32.a")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libglew32.a")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32s.lib")
    set(GLEW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/glew32s.lib")
else()
    message(FATAL_ERROR "找不到GLEW库文件。请确保glew32.a, glew32.lib, libglew32.a或glew32s.lib中的任意一个存在于'${CMAKE_SOURCE_DIR}/lib'目录中。")
endif()

# 手动设置GLFW路径
set(GLFW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
set(GLFW_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libglfw3.a")

# 手动设置GLM路径
set(GLM_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")

# SDL2相关设置
set(SDL2_DIR "${CMAKE_SOURCE_DIR}/lib/cmake/SDL2" CACHE PATH "Path to SDL2 CMake config")
set(SDL2_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include/SDL2")

# 自动查找可用的SDL2库文件
if(WIN32)
    # MinGW使用的是.a文件
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
            # 如果找不到.a文件，尝试直接设置-lSDL2
            message(WARNING "找不到libSDL2.a或libSDL2.dll.a，将尝试使用-lSDL2链接")
            set(SDL2_LIBRARIES "-lSDL2" "-lSDL2main")
        endif()
        
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.a")
            set(SDL2_MIXER_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.a")
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.dll.a")
            set(SDL2_MIXER_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/libSDL2_mixer.dll.a")
        else()
            # 如果找不到.a文件，尝试直接设置-lSDL2_mixer
            message(WARNING "找不到libSDL2_mixer.a或libSDL2_mixer.dll.a，将尝试使用-lSDL2_mixer链接")
            set(SDL2_MIXER_LIBRARIES "-lSDL2_mixer")
        endif()
    else()
        # MSVC使用的是.lib文件
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/SDL2.lib")
            set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/SDL2.lib")
            if(EXISTS "${CMAKE_SOURCE_DIR}/lib/SDL2main.lib")
                set(SDL2_LIBRARIES ${SDL2_LIBRARIES} "${CMAKE_SOURCE_DIR}/lib/SDL2main.lib")
            endif()
        else()
            message(FATAL_ERROR "找不到SDL2.lib文件。请确保SDL2.lib存在于'${CMAKE_SOURCE_DIR}/lib'目录中。")
        endif()
        
        if(EXISTS "${CMAKE_SOURCE_DIR}/lib/SDL2_mixer.lib")
            set(SDL2_MIXER_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/SDL2_mixer.lib")
        else()
            message(FATAL_ERROR "找不到SDL2_mixer.lib文件。请确保SDL2_mixer.lib存在于'${CMAKE_SOURCE_DIR}/lib'目录中。")
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

# 下载STB库
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# 包含目录
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include   
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${stb_SOURCE_DIR}
)

# 添加ImGui源文件
set(IMGUI_SOURCES
    include/imgui/imgui.cpp
    include/imgui/imgui_demo.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp
)

# 添加可执行文件
add_executable(ColorfulRainSimulation 
    main.cpp
    ${IMGUI_SOURCES}
)

# 设置Windows系统下的子系统
if(WIN32)
    # 设置WIN32应用程序为控制台程序
    set_target_properties(ColorfulRainSimulation PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# 链接库
target_link_libraries(ColorfulRainSimulation
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    -mconsole  # 使用控制台子系统，解决WinMain问题
)

# 根据平台添加其他需要的库
if(WIN32)
    target_link_libraries(ColorfulRainSimulation 
        winmm # Windows多媒体库
        gdi32
        user32
        shell32
        imm32
        version
        setupapi
    )
elseif(APPLE)
    # macOS特定的库
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(ColorfulRainSimulation
        ${CORE_FOUNDATION_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
    )
else()
    # Linux平台需要的库
    target_link_libraries(ColorfulRainSimulation
        dl
        pthread
    )
endif()

# 创建目录结构
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/textures)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/audio)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/config)

# 下载和安装脚本
# 添加下载依赖文件的脚本
if(WIN32)
    # Windows版本
    file(WRITE ${CMAKE_BINARY_DIR}/download_dependencies.bat
        "@echo off\n"
        "echo 正在下载依赖文件...\n"
        
        "mkdir include\\stb 2>nul\n"
        "curl -o include\\stb\\stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h\n"
        "curl -o include\\stb\\stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h\n"
        
        "mkdir textures 2>nul\n"
        "echo 请手动下载纹理文件并放入textures文件夹\n"
        
        "mkdir audio 2>nul\n"
        "echo 请手动下载音频文件并放入audio文件夹\n"
        
        "echo 下载完成!\n"
    )
else()
    # Unix版本
    file(WRITE ${CMAKE_BINARY_DIR}/download_dependencies.sh
        "#!/bin/bash\n"
        "echo \"正在下载依赖文件...\"\n"
        
        "mkdir -p include/stb\n"
        "curl -o include/stb/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h\n"
        "curl -o include/stb/stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h\n"
        
        "mkdir -p textures\n"
        "echo \"请手动下载纹理文件并放入textures文件夹\"\n"
        
        "mkdir -p audio\n"
        "echo \"请手动下载音频文件并放入audio文件夹\"\n"
        
        "echo \"下载完成!\"\n"
    )
    # 添加执行权限
    execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/download_dependencies.sh)
endif()

# 写入着色器文件
function(write_shader_file filename content)
    file(WRITE ${CMAKE_BINARY_DIR}/shaders/${filename} ${content})
endfunction()

# 写入水面顶点着色器
write_shader_file("water.vert"
"#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // 简单的波浪效果
    vec3 pos = aPos;
    pos.y = 0.0 + sin(pos.x * 0.1 + time) * 0.1 + cos(pos.z * 0.1 + time) * 0.1;
    
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoords = aTexCoords;
    
    // 简单的法线计算 (水面向上)
    Normal = vec3(0.0, 1.0, 0.0);
}")

# 写入水面片段着色器
write_shader_file("water.frag"
"#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D normalMap;
uniform sampler2D dudvMap;
uniform sampler2D reflectionMap;
uniform vec3 viewPos;
uniform float time;

void main() {
    // 扰动纹理坐标
    vec2 distortedTexCoords = texture(dudvMap, vec2(TexCoords.x + time * 0.05, TexCoords.y)).rg * 0.1;
    distortedTexCoords = TexCoords + vec2(distortedTexCoords.x, distortedTexCoords.y);
    
    // 从法线贴图获取法线
    vec3 normal = texture(normalMap, distortedTexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    
    // 环境光
    vec3 ambient = vec3(0.1, 0.1, 0.3);
    
    // 漫反射
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.3, 0.5, 0.7);
    
    // 反射
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * vec3(0.5);
    
    // 反射贴图
    float ratio = 0.6 + 0.4 * clamp(1.0 - dot(Normal, viewDir), 0.0, 1.0);
    vec2 reflectionCoord = vec2(TexCoords.x + normal.x * 0.05, TexCoords.y + normal.z * 0.05);
    vec3 reflection = texture(reflectionMap, reflectionCoord).rgb;
    
    // 最终颜色
    vec3 result = ambient + diffuse + specular;
    result = mix(result, reflection, ratio * 0.5);
    result = mix(vec3(0.0, 0.1, 0.3), result, 0.8); // 混合深蓝色水底
    
    FragColor = vec4(result, 0.8); // 水面半透明
}")

# 写入雨滴顶点着色器
write_shader_file("raindrop.vert"
"#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float raindropSize;
uniform vec3 raindropColor;

out vec3 Color;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = raindropSize / gl_Position.w; // 根据距离调整大小
    Color = raindropColor;
}")

# 写入雨滴片段着色器
write_shader_file("raindrop.frag"
"#version 330 core
out vec4 FragColor;

in vec3 Color;
uniform sampler2D glowTexture;

void main() {
    // 创建圆形点
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    if (dot(circCoord, circCoord) > 1.0) {
        discard;
    }
    
    // 添加一些亮度变化
    float brightness = 0.7 + 0.3 * (1.0 - length(circCoord));
    
    // 使用光晕贴图（如果有）
    // 这里我们假设没有贴图，直接使用计算的亮度
    
    FragColor = vec4(Color * brightness, 0.7);
}")

# 写入水波顶点着色器
write_shader_file("ripple.vert"
"#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}")

# 写入水波片段着色器
write_shader_file("ripple.frag"
"#version 330 core
out vec4 FragColor;

uniform vec3 rippleColor;
uniform float opacity;

void main() {
    FragColor = vec4(rippleColor, opacity);
}")

# 复制DLL文件
if(WIN32)
    # 对于GLEW
    if(EXISTS "${CMAKE_SOURCE_DIR}/lib/glew32.dll")
        add_custom_command(TARGET ColorfulRainSimulation POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/lib/glew32.dll"
                $<TARGET_FILE_DIR:ColorfulRainSimulation>)
    endif()
    
    # 对于SDL2，检查多个可能的位置
    foreach(SDL_DLL_PATH 
        "${CMAKE_SOURCE_DIR}/lib/SDL2.dll"
        "${CMAKE_SOURCE_DIR}/bin/SDL2.dll"
        "C:/msys64/mingw64/bin/SDL2.dll")
        if(EXISTS "${SDL_DLL_PATH}")
            add_custom_command(TARGET ColorfulRainSimulation POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SDL_DLL_PATH}"
                    $<TARGET_FILE_DIR:ColorfulRainSimulation>)
            message(STATUS "找到SDL2.dll: ${SDL_DLL_PATH}")
            break()
        endif()
    endforeach()
    
    # 对于SDL2_mixer，也检查多个位置
    foreach(SDL_MIXER_DLL_PATH 
        "${CMAKE_SOURCE_DIR}/lib/SDL2_mixer.dll"
        "${CMAKE_SOURCE_DIR}/bin/SDL2_mixer.dll"
        "C:/msys64/mingw64/bin/SDL2_mixer.dll")
        if(EXISTS "${SDL_MIXER_DLL_PATH}")
            add_custom_command(TARGET ColorfulRainSimulation POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SDL_MIXER_DLL_PATH}"
                    $<TARGET_FILE_DIR:ColorfulRainSimulation>)
            message(STATUS "找到SDL2_mixer.dll: ${SDL_MIXER_DLL_PATH}")
            break()
        endif()
    endforeach()
    
    # 检查是否需要复制额外的DLL依赖
    foreach(EXTRA_DLL_NAME
        "libfreetype-6.dll"
        "libmpg123-0.dll"
        "libogg-0.dll"
        "libvorbis-0.dll"
        "libvorbisfile-3.dll"
        "zlib1.dll")
        foreach(EXTRA_DLL_PATH 
            "${CMAKE_SOURCE_DIR}/lib/${EXTRA_DLL_NAME}"
            "${CMAKE_SOURCE_DIR}/bin/${EXTRA_DLL_NAME}"
            "C:/msys64/mingw64/bin/${EXTRA_DLL_NAME}")
            if(EXISTS "${EXTRA_DLL_PATH}")
                add_custom_command(TARGET ColorfulRainSimulation POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${EXTRA_DLL_PATH}"
                        $<TARGET_FILE_DIR:ColorfulRainSimulation>)
                message(STATUS "找到依赖DLL: ${EXTRA_DLL_PATH}")
                break()
            endif()
        endforeach()
    endforeach()
endif()

# 显示项目构建完成后的信息
add_custom_command(TARGET ColorfulRainSimulation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "构建完成！请确保以下资源文件存在:"
    COMMAND ${CMAKE_COMMAND} -E echo "1. textures/waternormal.jpeg"
    COMMAND ${CMAKE_COMMAND} -E echo "2. textures/waterDuDv.jpg"
    COMMAND ${CMAKE_COMMAND} -E echo "3. textures/waterReflection.jpg"
    COMMAND ${CMAKE_COMMAND} -E echo "4. textures/raindrop_glow.png"
    COMMAND ${CMAKE_COMMAND} -E echo "5. textures/night_sky.jpg"
    COMMAND ${CMAKE_COMMAND} -E echo "6. audio/raindrop_splash.wav"
    COMMAND ${CMAKE_COMMAND} -E echo "7. audio/ambient_rain.mp3"
    COMMAND ${CMAKE_COMMAND} -E echo "8. audio/water_ripple.wav"
)

# 帮助诊断文件路径问题
message(STATUS "检查项目目录:")
message(STATUS "项目根目录: ${CMAKE_SOURCE_DIR}")
message(STATUS "GLEW库路径: ${GLEW_LIBRARIES}")
message(STATUS "GLFW库路径: ${GLFW_LIBRARIES}")
message(STATUS "OpenGL库: ${OPENGL_LIBRARIES}")
message(STATUS "SDL2库: ${SDL2_LIBRARIES}")
message(STATUS "SDL2_mixer库: ${SDL2_MIXER_LIBRARIES}")

# 检查关键文件是否存在
if(EXISTS "${CMAKE_SOURCE_DIR}/include/GL/glew.h")
    message(STATUS "GLEW头文件存在: ${CMAKE_SOURCE_DIR}/include/GL/glew.h")
else()
    message(WARNING "GLEW头文件不存在: ${CMAKE_SOURCE_DIR}/include/GL/glew.h")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/include/SDL2/SDL.h")
    message(STATUS "SDL2头文件存在: ${CMAKE_SOURCE_DIR}/include/SDL2/SDL.h")
else()
    message(WARNING "SDL2头文件不存在: ${CMAKE_SOURCE_DIR}/include/SDL2/SDL.h - 请将SDL.h复制到此目录")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/include/SDL2/SDL_mixer.h")
    message(STATUS "SDL2_mixer头文件存在: ${CMAKE_SOURCE_DIR}/include/SDL2/SDL_mixer.h")
else()
    message(WARNING "SDL2_mixer头文件不存在: ${CMAKE_SOURCE_DIR}/include/SDL2/SDL_mixer.h - 请将SDL_mixer.h复制到此目录")
endif()

if(EXISTS "${GLEW_LIBRARIES}")
    message(STATUS "GLEW库文件存在: ${GLEW_LIBRARIES}")
else()
    message(WARNING "找不到GLEW库文件: ${GLEW_LIBRARIES}")
endif()

if(EXISTS "${GLFW_LIBRARIES}")
    message(STATUS "GLFW库文件存在: ${GLFW_LIBRARIES}")
else()
    message(WARNING "找不到GLFW库文件: ${GLFW_LIBRARIES}")
endif()

if(WIN32)
    if(MINGW)
        foreach(SDL_LIB ${SDL2_LIBRARIES})
            if(EXISTS "${SDL_LIB}")
                message(STATUS "SDL2库文件存在: ${SDL_LIB}")
            else()
                message(WARNING "找不到SDL2库文件: ${SDL_LIB} - 请从SDL2-devel-mingw包中复制对应库文件")
            endif()
        endforeach()
        
        if(EXISTS "${SDL2_MIXER_LIBRARIES}")
            message(STATUS "SDL2_mixer库文件存在: ${SDL2_MIXER_LIBRARIES}")
        else()
            message(WARNING "找不到SDL2_mixer库文件: ${SDL2_MIXER_LIBRARIES} - 请从SDL2_mixer-devel-mingw包中复制对应库文件")
        endif()
    else()
        if(EXISTS "${SDL2_LIBRARIES}")
            message(STATUS "SDL2库文件存在: ${SDL2_LIBRARIES}")
        else()
            message(WARNING "找不到SDL2库文件: ${SDL2_LIBRARIES}")
        endif()
        
        if(EXISTS "${SDL2_MIXER_LIBRARIES}")
            message(STATUS "SDL2_mixer库文件存在: ${SDL2_MIXER_LIBRARIES}")
        else()
            message(WARNING "找不到SDL2_mixer库文件: ${SDL2_MIXER_LIBRARIES}")
        endif()
    endif()
endif()
