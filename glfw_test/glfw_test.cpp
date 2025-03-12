// glfw_test.cpp - 测试GLFW、GLEW、SDL2和OpenGL依赖
#ifdef _WIN32
// 确保使用标准 main 入口点
#define SDL_MAIN_HANDLED
#endif

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>

// 设置控制台代码页为 UTF-8 或 GBK
void setConsoleCodePage() {
    // 使用 UTF-8 代码页 (65001)
    SetConsoleOutputCP(65001);
    // 或使用简体中文 GBK 代码页 (936)
    // SetConsoleOutputCP(936);
}
#else
void setConsoleCodePage() {
    // 非Windows系统不需要设置
}
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <windows.h>

// GLFW错误回调
void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW错误 " << error << ": " << description << std::endl;
}

// 测试GLFW
bool test_glfw() {
    std::cout << "测试GLFW初始化..." << std::endl;
    
    // 设置错误回调
    glfwSetErrorCallback(glfw_error_callback);
    
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW初始化失败" << std::endl;
        return false;
    }
    
    std::cout << "GLFW初始化成功" << std::endl;
    
    // 尝试创建OpenGL 3.3窗口
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(640, 480, "GLFW测试窗口", NULL, NULL);
    if (!window) {
        std::cerr << "OpenGL 3.3窗口创建失败，尝试OpenGL 2.1..." << std::endl;
        
        // 尝试使用OpenGL 2.1
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        
        window = glfwCreateWindow(640, 480, "GLFW测试窗口 (OpenGL 2.1)", NULL, NULL);
        if (!window) {
            std::cerr << "OpenGL 2.1窗口创建也失败" << std::endl;
            glfwTerminate();
            return false;
        }
        std::cout << "成功创建OpenGL 2.1窗口" << std::endl;
    } else {
        std::cout << "成功创建OpenGL 3.3窗口" << std::endl;
    }
    
    // 设置OpenGL上下文
    glfwMakeContextCurrent(window);
    
    // 输出GLFW版本
    int major, minor, revision;
    glfwGetVersion(&major, &minor, &revision);
    std::cout << "GLFW版本: " << major << "." << minor << "." << revision << std::endl;
    
    // 等待用户看到窗口
    std::cout << "窗口已创建，按任意键继续..." << std::endl;
    std::cin.get();
    
    // 释放资源
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return true;
}

// 测试GLEW
bool test_glew() {
    std::cout << "测试GLEW初始化..." << std::endl;
    
    // GLEW需要有效的OpenGL上下文，所以先初始化GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW初始化失败(GLEW测试需要)" << std::endl;
        return false;
    }
    
    // 创建临时窗口
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "GLEW测试窗口", NULL, NULL);
    if (!window) {
        std::cerr << "创建GLEW测试窗口失败" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    
    // 初始化GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW初始化失败: " << glewGetErrorString(err) << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }
    
    std::cout << "GLEW初始化成功" << std::endl;
    std::cout << "GLEW版本: " << glewGetString(GLEW_VERSION) << std::endl;
    
    // 输出OpenGL信息
    std::cout << "OpenGL供应商: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL渲染器: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL版本: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL版本: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // 等待用户看到窗口
    std::cout << "窗口已创建，按任意键继续..." << std::endl;
    std::cin.get();
    
    // 释放资源
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return true;
}

// 测试SDL2
bool test_sdl() {
    std::cout << "测试SDL2初始化..." << std::endl;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL2初始化失败: " << SDL_GetError() << std::endl;
        return false;
    }
    
    std::cout << "SDL2初始化成功" << std::endl;
    std::cout << "SDL2版本: " << SDL_MAJOR_VERSION << "." << SDL_MINOR_VERSION << "." << SDL_PATCHLEVEL << std::endl;
    
    // 测试SDL_mixer
    std::cout << "测试SDL2_mixer初始化..." << std::endl;
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL2_mixer初始化失败: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    std::cout << "SDL2_mixer初始化成功" << std::endl;
    
    // 输出SDL2_mixer版本
    SDL_version compiled;
    SDL_MIXER_VERSION(&compiled);
    std::cout << "SDL2_mixer版本: " << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch << std::endl;
    
    // 创建SDL窗口
    SDL_Window* window = SDL_CreateWindow("SDL2测试窗口",
                                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                         640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL2窗口创建失败: " << SDL_GetError() << std::endl;
        Mix_CloseAudio();
        SDL_Quit();
        return false;
    }
    
    std::cout << "SDL2窗口创建成功" << std::endl;
    
    // 等待用户看到窗口
    std::cout << "窗口已创建，按任意键继续..." << std::endl;
    std::cin.get();
    
    // 释放资源
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    SDL_Quit();
    
    return true;
}

// 检查DLL文件
void check_dlls() {
    #ifdef _WIN32
    std::cout << "检查DLL文件..." << std::endl;
    
    std::vector<std::string> required_dlls = {
        "glew32.dll",
        "SDL2.dll",
        "SDL2_mixer.dll"
    };
    
    for (const auto& dll : required_dlls) {
        HMODULE handle = LoadLibraryA(dll.c_str());
        if (handle) {
            std::cout << "成功加载: " << dll << std::endl;
            FreeLibrary(handle);
        } else {
            DWORD error = GetLastError();
            std::cerr << "加载失败: " << dll << " (错误码: " << error << ")" << std::endl;
        }
    }
    #endif
}

int main() {
    setConsoleCodePage();
    std::cout << "=============================" << std::endl;
    std::cout << "OpenGL依赖库测试程序" << std::endl;
    std::cout << "=============================" << std::endl;
    
    // 1. 检查DLL文件
    check_dlls();
    
    // 2. 测试GLFW
    std::cout << "\n[1] 测试GLFW..." << std::endl;
    if (test_glfw()) {
        std::cout << "✓ GLFW测试通过" << std::endl;
    } else {
        std::cout << "✗ GLFW测试失败" << std::endl;
    }
    
    // 3. 测试GLEW和OpenGL
    std::cout << "\n[2] 测试GLEW和OpenGL..." << std::endl;
    if (test_glew()) {
        std::cout << "✓ GLEW测试通过" << std::endl;
    } else {
        std::cout << "✗ GLEW测试失败" << std::endl;
    }
    
    // 4. 测试SDL2和SDL2_mixer
    std::cout << "\n[3] 测试SDL2和SDL2_mixer..." << std::endl;
    if (test_sdl()) {
        std::cout << "✓ SDL2测试通过" << std::endl;
    } else {
        std::cout << "✗ SDL2测试失败" << std::endl;
    }
    
    std::cout << "\n=============================" << std::endl;
    std::cout << "测试完成" << std::endl;
    std::cout << "按回车键退出..." << std::endl;
    std::cin.get();
    
    return 0;
}