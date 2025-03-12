// main.cpp - 主程序入口

// !!!重要!!! SDL.h必须是第一个包含的头文件，以便正确定义WinMain
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <vector>
#include <random>
#include <memory>
#include <chrono>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>

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

// 添加自定义filesystem兼容层
#include "filesystem_compat.h"

// 纹理加载
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// 图像写入
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

// 确保使用SDL的main
#define SDL_MAIN_HANDLED

// OpenGL相关库
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui库（用于UI）
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// 常量定义
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float POND_SIZE = 100.0f;
const float WATER_HEIGHT = 0.0f;

// 着色器类
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        // 1. 从文件读取顶点和片段着色器源码
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            
            std::stringstream vShaderStream, fShaderStream;
            
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            
            vShaderFile.close();
            fShaderFile.close();
            
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch(std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        
        // 2. 编译着色器
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];
        
        // 顶点着色器
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        
        // 片段着色器
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        
        // 着色器程序
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        
        // 删除着色器，它们已链接到我们的程序中，不再需要
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // 激活着色器
    void use() {
        glUseProgram(ID);
    }

    // uniform工具函数
    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setVec2(const std::string &name, const glm::vec2 &value) const {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, const glm::vec4 &value) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

// 应用类前置声明
class RainSimulation;

// 雨滴类
class Raindrop {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float size;
    float lifespan;
    float lifetime;
    bool visible;
    int state; // 0: 下落, 1: 入水, 2: 消失
    RainSimulation* simulation; // 指向主应用的指针，用于调用声音函数

    Raindrop() : 
        position(0.0f),
        velocity(0.0f),
        color(1.0f),
        size(0.1f),
        lifespan(3.0f),
        lifetime(0.0f),
        visible(true),
        state(0),
        simulation(nullptr) {
    }

    void init(const glm::vec3& _position, const glm::vec3& _color, RainSimulation* _simulation) {
        position = _position;
        color = _color;
        simulation = _simulation;
        velocity = glm::vec3(0.0f, -2.0f - static_cast<float>(rand()) / RAND_MAX * 2.0f, 0.0f);
        size = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.1f;
        lifespan = 3.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
        lifetime = 0.0f;
        visible = true;
        state = 0;
    }

    bool update(float deltaTime);  // 声明，但在RainSimulation类后实现

    bool isDead() const {
        return state > 1 || lifetime > lifespan;
    }
};

// 水波类
class WaterRipple {
public:
    glm::vec3 position;
    glm::vec3 color;
    float radius;
    float maxRadius;
    float thickness;
    float opacity;
    float growthRate;
    float lifetime;
    float maxLifetime;
    
    WaterRipple() : 
        position(0.0f),
        color(1.0f),
        radius(0.5f),
        maxRadius(5.0f),
        thickness(0.2f),
        opacity(0.8f),
        growthRate(2.0f),
        lifetime(0.0f),
        maxLifetime(2.0f) {
    }
    
    void init(const glm::vec3& _position, const glm::vec3& _color) {
        position = _position;
        position.y = WATER_HEIGHT + 0.01f; // 略高于水面
        color = _color;
        radius = 0.5f;
        maxRadius = 5.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f;
        thickness = 0.2f;
        opacity = 0.8f;
        growthRate = 2.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
        lifetime = 0.0f;
        maxLifetime = 2.0f;
    }
    
    bool update(float deltaTime) {
        lifetime += deltaTime;
        radius += growthRate * deltaTime;
        opacity = 0.8f * (1.0f - lifetime / maxLifetime);
        
        return isDead();
    }
    
    bool isDead() const {
        return radius >= maxRadius || opacity <= 0.0f || lifetime >= maxLifetime;
    }
};

// 应用类
class RainSimulation {
public:
    // 窗口
    GLFWwindow* window;
    
    // 着色器
    std::unique_ptr<Shader> waterShader;
    std::unique_ptr<Shader> raindropShader;
    std::unique_ptr<Shader> rippleShader;
    
    // 几何体
    unsigned int waterVAO, waterVBO;
    unsigned int raindropVAO, raindropVBO;
    unsigned int rippleVAO, rippleVBO;
    
    // 纹理
    unsigned int waterNormalTexture;
    unsigned int waterDuDvTexture;
    unsigned int waterReflectionTexture;
    unsigned int raindropGlowTexture;
    unsigned int skyTexture;
    
    // 相机
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float cameraPitch;
    float cameraYaw;
    
    // 雨滴和水波
    std::vector<Raindrop> raindrops;
    std::vector<WaterRipple> ripples;
    
    // 配置
    struct {
        int rainDensity = 50;
        float maxRippleSize = 15.0f;
        float updateInterval = 0.03f;
        float rippleFadeSpeed = 0.02f;
        std::vector<glm::vec3> raindropColors = {
            glm::vec3(1.0f, 0.0f, 1.0f), // 紫色
            glm::vec3(0.0f, 1.0f, 1.0f), // 青色
            glm::vec3(1.0f, 1.0f, 0.0f), // 黄色
            glm::vec3(1.0f, 0.0f, 0.0f), // 红色
            glm::vec3(0.0f, 1.0f, 0.0f)  // 绿色
        };
    } config;
    
    // SDL音频相关成员
    // 音频声音
    Mix_Chunk* raindropSound;
    Mix_Music* ambientRainSound;
    Mix_Chunk* waterRippleSound;
    
    // 音频配置
    struct {
        bool soundEnabled = true;
        float masterVolume = 0.8f;
        float raindropVolume = 0.5f;
        float ambientVolume = 0.3f;
        float rippleVolume = 0.4f;
    } audioConfig;
    
    // 时间跟踪
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    float rainAccumulator = 0.0f;
    
    RainSimulation() : 
        window(nullptr),
        cameraPos(glm::vec3(0.0f, 10.0f, 40.0f)),
        cameraFront(glm::vec3(0.0f, 0.0f, -1.0f)),
        cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
        cameraPitch(-15.0f),
        cameraYaw(-90.0f),
        raindropSound(nullptr),
        ambientRainSound(nullptr),
        waterRippleSound(nullptr) {
    }
    
    ~RainSimulation() {
        // 释放音频资源
        cleanup();

        // 释放资源
        glDeleteVertexArrays(1, &waterVAO);
        glDeleteBuffers(1, &waterVBO);
        glDeleteVertexArrays(1, &raindropVAO);
        glDeleteBuffers(1, &raindropVBO);
        glDeleteVertexArrays(1, &rippleVAO);
        glDeleteBuffers(1, &rippleVBO);
        
        glDeleteTextures(1, &waterNormalTexture);
        glDeleteTextures(1, &waterDuDvTexture);
        glDeleteTextures(1, &waterReflectionTexture);
        glDeleteTextures(1, &raindropGlowTexture);
        glDeleteTextures(1, &skyTexture);
        
        // ImGui清理
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        // 关闭GLFW
        glfwTerminate();
    }
    
    // 清理函数，在析构函数中调用
    void cleanup() {
        // 释放音频资源
        if (raindropSound != nullptr) {
            Mix_FreeChunk(raindropSound);
            raindropSound = nullptr;
        }
        
        if (ambientRainSound != nullptr) {
            Mix_FreeMusic(ambientRainSound);
            ambientRainSound = nullptr;
        }
        
        if (waterRippleSound != nullptr) {
            Mix_FreeChunk(waterRippleSound);
            waterRippleSound = nullptr;
        }
        
        // 关闭SDL_mixer
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
    }
    
    // 初始化音频子系统的函数
    bool initAudio() {
        // 初始化SDL
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "无法初始化SDL_AUDIO: " << SDL_GetError() << std::endl;
            audioConfig.soundEnabled = false;
            return false;
        }
        
        // 初始化SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "无法初始化SDL_Mixer: " << Mix_GetError() << std::endl;
            audioConfig.soundEnabled = false;
            return false;
        }
        
        // 分配更多通道用于同时播放多个音效
        Mix_AllocateChannels(32);
        
        // 设置主音量
        Mix_Volume(-1, static_cast<int>(audioConfig.masterVolume * MIX_MAX_VOLUME));
        
        // 确保音频目录存在
        ensureAudioFilesExist();
        
        // 加载音频文件
        raindropSound = Mix_LoadWAV("audio/raindrop_splash.wav");
        if (!raindropSound) {
            std::cerr << "无法加载雨滴音效: " << Mix_GetError() << std::endl;
        }
        
        ambientRainSound = Mix_LoadMUS("audio/ambient_rain.mp3");
        if (!ambientRainSound) {
            std::cerr << "无法加载环境雨声: " << Mix_GetError() << std::endl;
        }
        
        waterRippleSound = Mix_LoadWAV("audio/water_ripple.wav");
        if (!waterRippleSound) {
            std::cerr << "无法加载水波音效: " << Mix_GetError() << std::endl;
        }
        
        // 设置各个音效的音量
        if (raindropSound) {
            Mix_VolumeChunk(raindropSound, static_cast<int>(audioConfig.raindropVolume * MIX_MAX_VOLUME));
        }
        
        if (waterRippleSound) {
            Mix_VolumeChunk(waterRippleSound, static_cast<int>(audioConfig.rippleVolume * MIX_MAX_VOLUME));
        }
        
        // 播放背景雨声（循环）
        if (ambientRainSound) {
            Mix_VolumeMusic(static_cast<int>(audioConfig.ambientVolume * MIX_MAX_VOLUME));
            Mix_PlayMusic(ambientRainSound, -1); // -1表示无限循环
        }
        
        return true;
    }

    // 播放雨滴声音的函数
    void playRaindropSound(const glm::vec3& position) {
        if (!audioConfig.soundEnabled || !raindropSound)
            return;
            
        // 计算音量基于与相机的距离
        float distance = glm::length(position - cameraPos);
        float volumeScale = 1.0f - std::min(distance / 50.0f, 0.95f); // 50.0f是最大听觉距离
        
        // 随机化音量和音高以增加多样性
        float volumeVariation = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        int volume = static_cast<int>(audioConfig.raindropVolume * MIX_MAX_VOLUME * volumeScale * volumeVariation);
        
        // 找一个空闲通道播放
        int channel = Mix_PlayChannel(-1, raindropSound, 0);
        if (channel != -1) {
            // 设置声道音量
            Mix_Volume(channel, volume);
        }
    }

    // 播放水波声音的函数
    void playRippleSound(const glm::vec3& position) {
        if (!audioConfig.soundEnabled || !waterRippleSound)
            return;
            
        // 计算音量基于与相机的距离
        float distance = glm::length(position - cameraPos);
        float volumeScale = 1.0f - std::min(distance / 50.0f, 0.95f); // 50.0f是最大听觉距离
        
        // 随机化音量以增加多样性
        float volumeVariation = 0.7f + static_cast<float>(rand()) / RAND_MAX * 0.6f;
        int volume = static_cast<int>(audioConfig.rippleVolume * MIX_MAX_VOLUME * volumeScale * volumeVariation * 0.5f);
        
        // 找一个空闲通道播放
        int channel = Mix_PlayChannel(-1, waterRippleSound, 0);
        if (channel != -1) {
            // 设置声道音量
            Mix_Volume(channel, volume);
        }
    }

    // 更新音频设置
    void updateAudioSettings() {
        if (!audioConfig.soundEnabled)
            return;
            
        // 更新主音量
        Mix_Volume(-1, static_cast<int>(audioConfig.masterVolume * MIX_MAX_VOLUME));
        
        // 更新音效音量
        if (raindropSound) {
            Mix_VolumeChunk(raindropSound, static_cast<int>(audioConfig.raindropVolume * MIX_MAX_VOLUME));
        }
        
        if (waterRippleSound) {
            Mix_VolumeChunk(waterRippleSound, static_cast<int>(audioConfig.rippleVolume * MIX_MAX_VOLUME));
        }
        
        // 更新音乐音量
        Mix_VolumeMusic(static_cast<int>(audioConfig.ambientVolume * MIX_MAX_VOLUME));
    }

    bool init() {

        // 初始化GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // 设置OpenGL版本
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Mac OS X需要
#endif
        
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);  // For better error reporting
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);            // Disable window resizing
        glfwWindowHint(GLFW_VISIBLE, GL_TRUE);               // Make window visible
        glfwWindowHint(GLFW_FOCUSED, GL_TRUE);               // Give focus to the window

        // 创建窗口
        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "池塘夜雨彩色雨滴模拟", NULL, NULL);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        });
        
        // 初始化GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }
        
        // 启用深度测试
        glEnable(GL_DEPTH_TEST);
        
        // 启用混合
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // 初始化ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // 初始化音频
        initAudio();
        
        // 加载着色器
        loadShaders();
        
        // 创建几何体
        createGeometry();
        
        // 加载纹理
        loadTextures();
        
        return true;
    }
    
    void loadShaders() {
        // 加载水面着色器
        waterShader = std::make_unique<Shader>("shaders/water.vert", "shaders/water.frag");
        
        // 加载雨滴着色器
        raindropShader = std::make_unique<Shader>("shaders/raindrop.vert", "shaders/raindrop.frag");
        
        // 加载水波着色器
        rippleShader = std::make_unique<Shader>("shaders/ripple.vert", "shaders/ripple.frag");
    }
    
    void createGeometry() {
        // 创建水面平面
        float waterVertices[] = {
            // 位置              // 纹理坐标
            -POND_SIZE/2, 0.0f, -POND_SIZE/2,  0.0f, 0.0f,
             POND_SIZE/2, 0.0f, -POND_SIZE/2,  1.0f, 0.0f,
             POND_SIZE/2, 0.0f,  POND_SIZE/2,  1.0f, 1.0f,
            
            -POND_SIZE/2, 0.0f, -POND_SIZE/2,  0.0f, 0.0f,
             POND_SIZE/2, 0.0f,  POND_SIZE/2,  1.0f, 1.0f,
            -POND_SIZE/2, 0.0f,  POND_SIZE/2,  0.0f, 1.0f
        };
        
        glGenVertexArrays(1, &waterVAO);
        glGenBuffers(1, &waterVBO);
        
        glBindVertexArray(waterVAO);
        glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(waterVertices), waterVertices, GL_STATIC_DRAW);
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // 纹理坐标属性
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        // 创建雨滴点
        float raindropVertices[] = {
            // 一个点
            0.0f, 0.0f, 0.0f
        };
        
        glGenVertexArrays(1, &raindropVAO);
        glGenBuffers(1, &raindropVBO);
        
        glBindVertexArray(raindropVAO);
        glBindBuffer(GL_ARRAY_BUFFER, raindropVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(raindropVertices), raindropVertices, GL_STATIC_DRAW);
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        // 创建水波环
        std::vector<float> rippleVertices;
        const int segments = 64;
        const float radius = 1.0f;
        
        for (int i = 0; i < segments; ++i) {
            float theta1 = 2.0f * glm::pi<float>() * float(i) / float(segments);
            float theta2 = 2.0f * glm::pi<float>() * float(i + 1) / float(segments);
            
            // 内环点
            rippleVertices.push_back(radius * 0.8f * cos(theta1));
            rippleVertices.push_back(0.0f);
            rippleVertices.push_back(radius * 0.8f * sin(theta1));
            
            // 外环点
            rippleVertices.push_back(radius * cos(theta1));
            rippleVertices.push_back(0.0f);
            rippleVertices.push_back(radius * sin(theta1));
            
            // 外环点 (下一个)
            rippleVertices.push_back(radius * cos(theta2));
            rippleVertices.push_back(0.0f);
            rippleVertices.push_back(radius * sin(theta2));
            
            // 内环点
            rippleVertices.push_back(radius * 0.8f * cos(theta1));
            rippleVertices.push_back(0.0f);
            rippleVertices.push_back(radius * 0.8f * sin(theta1));
            
            // 外环点 (下一个)
            rippleVertices.push_back(radius * cos(theta2));
            rippleVertices.push_back(0.0f);
            rippleVertices.push_back(radius * sin(theta2));
            
            // 内环点 (下一个)
            rippleVertices.push_back(radius * 0.8f * cos(theta2));
            rippleVertices.push_back(0.0f);
            rippleVertices.push_back(radius * 0.8f * sin(theta2));
        }
        
        glGenVertexArrays(1, &rippleVAO);
        glGenBuffers(1, &rippleVBO);
        
        glBindVertexArray(rippleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, rippleVBO);
        glBufferData(GL_ARRAY_BUFFER, rippleVertices.size() * sizeof(float), rippleVertices.data(), GL_STATIC_DRAW);
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    // 修改loadTextures函数
    void loadTextures() {
        // 确保纹理存在
        ensureTexturesExist();
        
        // 加载所有需要的纹理
        waterNormalTexture = loadTexture("textures/waternormal.jpg");
        waterDuDvTexture = loadTexture("textures/waterDuDv.jpg");
        waterReflectionTexture = loadTexture("textures/waterReflection.jpg");
        raindropGlowTexture = loadTexture("textures/raindrop_glow.png");
        skyTexture = loadTexture("textures/night_sky.jpg");
    }
    
    unsigned int loadTexture(const char* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        
        // 绑定纹理，后续的所有GL_TEXTURE_2D操作都会影响这个纹理
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // 设置纹理包装和过滤参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // 加载并生成纹理
        int width, height, nrChannels;
        
        // 翻转图像，因为OpenGL期望以左下角为原点
        stbi_set_flip_vertically_on_load(true);
        
        // 尝试加载图像
        unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
        
        if (data) {
            GLenum format;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;
            else {
                std::cerr << "未知的图像格式: " << path << " (通道数: " << nrChannels << ")" << std::endl;
                format = GL_RGB; // 默认尝试RGB
            }
            
            // 生成纹理
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            std::cout << "成功加载纹理: " << path << " (" << width << "x" << height << ", " 
                      << nrChannels << " channels)" << std::endl;
        } else {
            std::cerr << "纹理加载失败: " << path << std::endl;
            
            // 如果纹理加载失败，创建一个默认纹理
            // 创建一个1x1的纹理，用于替代丢失的纹理
            unsigned char defaultTextureData[4] = {255, 255, 255, 255}; // 白色像素
            
            if (strstr(path, "normal")) {
                // 法线贴图的默认值: 指向上的法线(0, 0, 1)映射到RGB(128, 128, 255)
                defaultTextureData[0] = 128; // R
                defaultTextureData[1] = 128; // G
                defaultTextureData[2] = 255; // B
                defaultTextureData[3] = 255; // A
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "DuDv")) {
                // DuDv贴图的默认值: 无偏移(128, 128)
                defaultTextureData[0] = 128; // R
                defaultTextureData[1] = 128; // G
                defaultTextureData[2] = 128; // B
                defaultTextureData[3] = 255; // A
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "Reflection")) {
                // 反射贴图默认值: 深蓝色
                defaultTextureData[0] = 0;   // R
                defaultTextureData[1] = 40;  // G
                defaultTextureData[2] = 80;  // B
                defaultTextureData[3] = 255; // A
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else {
                // 其他纹理的默认值: 浅蓝色
                defaultTextureData[0] = 100; // R
                defaultTextureData[1] = 150; // G
                defaultTextureData[2] = 255; // B
                defaultTextureData[3] = 255; // A
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            }
            
            glGenerateMipmap(GL_TEXTURE_2D);
            std::cout << "已生成默认纹理作为替代" << std::endl;
        }
        
        // 释放图像数据
        stbi_image_free(data);
        
        return textureID;
    }
    
    // 创建纹理文件夹和确保纹理存在的函数
    bool ensureTexturesExist() {
        // 检查纹理目录是否存在，如果不存在则创建
        if (!file_exists("textures")) {
            create_directory("textures");
            std::cout << "已创建textures目录" << std::endl;
        }
        
        // 检查每个必需的纹理文件是否存在
        std::vector<std::string> requiredTextures = {
            "textures/waternormal.jpg",
            "textures/waterDuDv.jpg",
            "textures/waterReflection.jpg",
            "textures/raindrop_glow.png",
            "textures/night_sky.jpg"
        };
        
        bool allTexturesExist = true;
        
        for (const auto& texture : requiredTextures) {
            if (!file_exists(texture)) {
                std::cerr << "警告: 找不到纹理文件: " << texture << std::endl;
                allTexturesExist = false;
                
                // 创建一个简单的默认纹理文件
                std::cout << "正在生成默认纹理文件: " << texture << std::endl;
                generateDefaultTexture(texture);
            }
        }
        
        return allTexturesExist;
    }

    // 生成默认纹理文件
    void generateDefaultTexture(const std::string& path) {
        // 确保目录存在
        std::string parent = parent_path(path);
        if (!parent.empty() && !file_exists(parent)) {
            create_directories(parent);
        }
        
        // 创建一个简单的4x4纹理
        const int width = 4;
        const int height = 4;
        const int channels = 3; // RGB
        unsigned char data[width * height * channels];
        
        // 填充数据
        for (int i = 0; i < width * height; i++) {
            if (path.find("normal") != std::string::npos) {
                // 法线贴图: 蓝紫色
                data[i*channels+0] = 128; // R
                data[i*channels+1] = 128; // G
                data[i*channels+2] = 255; // B
            } else if (path.find("DuDv") != std::string::npos) {
                // DuDv贴图: 灰色
                data[i*channels+0] = 128; // R
                data[i*channels+1] = 128; // G
                data[i*channels+2] = 128; // B
            } else if (path.find("Reflection") != std::string::npos) {
                // 反射贴图: 深蓝色
                data[i*channels+0] = 0;   // R
                data[i*channels+1] = 40;  // G
                data[i*channels+2] = 80;  // B
            } else if (path.find("glow") != std::string::npos) {
                // 光晕贴图: 白色
                data[i*channels+0] = 255; // R
                data[i*channels+1] = 255; // G
                data[i*channels+2] = 255; // B
            } else if (path.find("sky") != std::string::npos) {
                // 天空贴图: 深蓝色
                data[i*channels+0] = 20;  // R
                data[i*channels+1] = 30;  // G
                data[i*channels+2] = 80;  // B
            } else {
                // 默认: 浅蓝色
                data[i*channels+0] = 100; // R
                data[i*channels+1] = 150; // G
                data[i*channels+2] = 255; // B
            }
        }
        
        // 写入JPG或PNG文件
        if (path.find(".png") != std::string::npos) {
            stbi_write_png(path.c_str(), width, height, channels, data, width * channels);
        } else {
            stbi_write_jpg(path.c_str(), width, height, channels, data, 90); // 90是质量设置(0-100)
        }
    }

    // 确保音频文件存在的函数
    bool ensureAudioFilesExist() {
        // 检查音频目录是否存在，如果不存在则创建
        if (!file_exists("audio")) {
            create_directory("audio");
            std::cout << "已创建audio目录" << std::endl;
        }
        
        // 检查每个必需的音频文件是否存在
        std::vector<std::string> requiredAudioFiles = {
            "audio/raindrop_splash.wav",
            "audio/ambient_rain.mp3",
            "audio/water_ripple.wav"
        };
        
        bool allAudioFilesExist = true;
        
        for (const auto& audioFile : requiredAudioFiles) {
            if (!file_exists(audioFile)) {
                std::cerr << "警告: 找不到音频文件: " << audioFile << std::endl;
                allAudioFilesExist = false;
                
                // 创建占位符音频文件
                generatePlaceholderAudioFile(audioFile);
            }
        }
        
        return allAudioFilesExist;
    }

    // 生成占位符音频文件
    void generatePlaceholderAudioFile(const std::string& path) {
        // 确保目录存在
        std::string parent = parent_path(path);
        if (!parent.empty() && !file_exists(parent)) {
            create_directories(parent);
        }
        
        // 创建一个简单的文本文件指示音频文件缺失
        std::ofstream placeholderFile(path + ".placeholder.txt");
        placeholderFile << "这是" << path << "的占位符文件。请下载或创建实际的音频文件。";
        placeholderFile.close();
        
        // 注意：由于创建实际音频文件需要复杂的音频编码库，这里只创建占位符
        std::cout << "已为缺失的音频文件创建占位符: " << path << ".placeholder.txt" << std::endl;
        std::cout << "请将实际的音频文件放在对应位置以启用声音" << std::endl;
    }
    
    void run() {
        while (!glfwWindowShouldClose(window)) {
            // 处理时间
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            // 处理输入
            processInput();
            
            // 更新
            update();
            
            // 渲染
            render();
            
            // 交换缓冲并轮询IO事件
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    
    void processInput() {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
            
        // 相机控制
        float cameraSpeed = 5.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += cameraUp * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos -= cameraUp * cameraSpeed;
    }
    
    void update() {
        // 生成新雨滴
        rainAccumulator += deltaTime;
        if (rainAccumulator >= config.updateInterval) {
            rainAccumulator = 0.0f;
            generateRaindrops();
        }
        
        // 更新雨滴
        for (auto it = raindrops.begin(); it != raindrops.end();) {
            bool createRipple = it->update(deltaTime);
            
            if (createRipple) {
                // 创建水波
                WaterRipple ripple;
                ripple.init(it->position, it->color);
                ripples.push_back(ripple);
                
                // 播放水波声音(概率性的，不是每个水波都播放)
                if (rand() % 100 < 30) { // 30%的概率播放水波声音
                    playRippleSound(ripple.position);
                }
            }
            
            if (it->isDead()) {
                it = raindrops.erase(it);
            } else {
                ++it;
            }
        }
        
        // 更新水波
        for (auto it = ripples.begin(); it != ripples.end();) {
            if (it->update(deltaTime)) {
                it = ripples.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void generateRaindrops() {
        int raindropsToGenerate = config.rainDensity / 50;
        
        for (int i = 0; i < raindropsToGenerate; ++i) {
            if (rand() % 100 < 10) { // 10%的概率生成雨滴
                Raindrop raindrop;
                
                // 随机位置
                float x = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                float z = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                float y = 20.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f;
                
                // 随机颜色
                int colorIndex = rand() % config.raindropColors.size();
                
                raindrop.init(glm::vec3(x, y, z), config.raindropColors[colorIndex], this);
                raindrops.push_back(raindrop);
            }
        }
    }
    
    void render() {
        // 清空缓冲
        glClearColor(0.02f, 0.05f, 0.2f, 1.0f); // 深蓝色夜空
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 视图/投影矩阵
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        
        // 渲染水面
        renderWater(view, projection);
        
        // 渲染雨滴
        renderRaindrops(view, projection);
        
        // 渲染水波
        renderRipples(view, projection);
        
        // 渲染UI
        renderUI();
    }
    
    void renderWater(const glm::mat4& view, const glm::mat4& projection) {
        waterShader->use();
        
        // 设置变换矩阵
        glm::mat4 model = glm::mat4(1.0f);
        waterShader->setMat4("model", model);
        waterShader->setMat4("view", view);
        waterShader->setMat4("projection", projection);
        
        // 设置纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, waterNormalTexture);
        waterShader->setInt("normalMap", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, waterDuDvTexture);
        waterShader->setInt("dudvMap", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, waterReflectionTexture);
        waterShader->setInt("reflectionMap", 2);
        
        // 设置水面属性
        waterShader->setFloat("time", lastFrame * 0.05f); // 使水面动起来
        waterShader->setVec3("viewPos", cameraPos);
        
        // 绘制水面
        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
    
    void renderRaindrops(const glm::mat4& view, const glm::mat4& projection) {
        raindropShader->use();
        
        // 设置变换矩阵
        raindropShader->setMat4("view", view);
        raindropShader->setMat4("projection", projection);
        
        // 启用点大小
        glEnable(GL_PROGRAM_POINT_SIZE);
        
        // 遍历所有雨滴
        glBindVertexArray(raindropVAO);
        for (const auto& raindrop : raindrops) {
            if (!raindrop.visible || raindrop.state > 0)
                continue;
                
            // 设置模型矩阵
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, raindrop.position);
            raindropShader->setMat4("model", model);
            
            // 设置雨滴属性
            raindropShader->setVec3("raindropColor", raindrop.color);
            raindropShader->setFloat("raindropSize", raindrop.size * 10.0f); // 放大点大小
            
            // 绘制雨滴
            glDrawArrays(GL_POINTS, 0, 1);
        }
        glBindVertexArray(0);
        
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
    
    void renderRipples(const glm::mat4& view, const glm::mat4& projection) {
        rippleShader->use();
        
        // 设置变换矩阵
        rippleShader->setMat4("view", view);
        rippleShader->setMat4("projection", projection);
        
        // 遍历所有水波
        glBindVertexArray(rippleVAO);
        for (const auto& ripple : ripples) {
            // 设置模型矩阵
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, ripple.position);
            model = glm::scale(model, glm::vec3(ripple.radius));
            rippleShader->setMat4("model", model);
            
            // 设置水波属性
            rippleShader->setVec3("rippleColor", ripple.color);
            rippleShader->setFloat("opacity", ripple.opacity);
            
            // 绘制水波
            glDrawArrays(GL_TRIANGLES, 0, 6 * 64); // 64个三角形条带
        }
        glBindVertexArray(0);
    }
    
    void renderUI() {
        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // 创建控制面板
        ImGui::Begin("控制面板");
        
        ImGui::Text("池塘夜雨彩色雨滴模拟");
        ImGui::Separator();
        
        // 视觉效果控制
        ImGui::SliderInt("雨点密度", &config.rainDensity, 10, 200);
        ImGui::SliderFloat("最大水圈大小", &config.maxRippleSize, 5.0f, 30.0f);
        ImGui::SliderFloat("更新间隔 (秒)", &config.updateInterval, 0.01f, 0.1f);
        
        // 音频控制
        if (ImGui::CollapsingHeader("音频设置")) {
            bool soundEnabledChanged = ImGui::Checkbox("启用声音", &audioConfig.soundEnabled);
            
            ImGui::SliderFloat("主音量", &audioConfig.masterVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("雨滴音量", &audioConfig.raindropVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("环境雨声音量", &audioConfig.ambientVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("水波音量", &audioConfig.rippleVolume, 0.0f, 1.0f);
            
            // 当音频设置改变时更新SDL音频设置
            if (ImGui::IsItemEdited() || soundEnabledChanged) {
                updateAudioSettings();
            }
        }
        
        ImGui::Text("统计信息");
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Text("雨滴数量: %lu", raindrops.size());
        ImGui::Text("水波数量: %lu", ripples.size());
        
        ImGui::End();
        
        // 渲染ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

// 雨滴类的update方法 - 使用RainSimulation的playRaindropSound方法
bool Raindrop::update(float deltaTime) {
    lifetime += deltaTime;
    
    // 闪烁效果
    if (rand() % 100 < 5) { // 5%的概率切换可见性
        visible = !visible;
    }
    
    if (state == 0) { // 下落状态
        position += velocity * deltaTime;
        
        // 检查是否碰到水面
        if (position.y <= WATER_HEIGHT) {
            state = 1; // 入水状态
            visible = false;
            
            // 播放入水声音
            if (simulation) {
                simulation->playRaindropSound(position);
            }
            
            return true; // 告知需要创建波纹
        }
    }
    
    return false;
}

// 写入着色器文件
void writeShaderFiles() {
    // 创建着色器目录
    if (!file_exists("shaders")) {
        create_directory("shaders");
    }
    
    // 顶点着色器 - 水面
    const char* waterVertexShader = R"(
#version 330 core
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
}
)";

    // 片段着色器 - 水面
    const char* waterFragmentShader = R"(
#version 330 core
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
}
)";

    // 顶点着色器 - 雨滴
    const char* raindropVertexShader = R"(
#version 330 core
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
}
)";

    // 片段着色器 - 雨滴
    const char* raindropFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 Color;

void main() {
    // 创建圆形点
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    if (dot(circCoord, circCoord) > 1.0) {
        discard;
    }
    
    // 添加一些亮度变化
    float brightness = 0.7 + 0.3 * (1.0 - length(circCoord));
    
    FragColor = vec4(Color * brightness, 0.7);
}
)";

    // 顶点着色器 - 水波
    const char* rippleVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    // 片段着色器 - 水波
    const char* rippleFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 rippleColor;
uniform float opacity;

void main() {
    FragColor = vec4(rippleColor, opacity);
}
)";
    
    // 水面着色器
    std::ofstream waterVert("shaders/water.vert");
    waterVert << waterVertexShader;
    waterVert.close();
    
    std::ofstream waterFrag("shaders/water.frag");
    waterFrag << waterFragmentShader;
    waterFrag.close();
    
    // 雨滴着色器
    std::ofstream raindropVert("shaders/raindrop.vert");
    raindropVert << raindropVertexShader;
    raindropVert.close();
    
    std::ofstream raindropFrag("shaders/raindrop.frag");
    raindropFrag << raindropFragmentShader;
    raindropFrag.close();
    
    // 水波着色器
    std::ofstream rippleVert("shaders/ripple.vert");
    rippleVert << rippleVertexShader;
    rippleVert.close();
    
    std::ofstream rippleFrag("shaders/ripple.frag");
    rippleFrag << rippleFragmentShader;
    rippleFrag.close();
    
    // 创建纹理目录
    if (!file_exists("textures")) {
        create_directory("textures");
    }
    
    // 创建音频目录
    if (!file_exists("audio")) {
        create_directory("audio");
    }
}

// 使用与SDL兼容的main函数
#ifdef __WINDOWS__
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
#else
int main(int argc, char* argv[]) {
#endif

    setConsoleCodePage();
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "无法初始化SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // 写入着色器文件
    writeShaderFiles();
    
    // 设置随机数种子
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // 创建并运行模拟
    RainSimulation simulation;
    if (simulation.init()) {
        simulation.run();
    }
    
    // 退出前清理SDL
    SDL_Quit();
    
    return 0;
}
