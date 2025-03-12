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

// 定义一些额外的常量，以便在没有纹理的情况下生成更好的视觉效果
const int STARS_COUNT = 300;      // 夜空中的星星数量
const int CLOUD_COUNT = 5;        // 云朵数量
const float MOON_SIZE = 20.0f;    // 月亮大小
const float MOON_X = 70.0f;       // 月亮X坐标
const float MOON_Y = 60.0f;       // 月亮Y坐标

// 星星结构
struct Star {
    glm::vec3 position;
    float brightness;
    float twinkleSpeed;
    float size;
};

// 云朵结构
struct Cloud {
    glm::vec3 position;
    float size;
    float opacity;
    float speed;
};

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
    float brightness; // 亮度变化
    float twinkleSpeed; // 闪烁速度
    float trail; // 雨滴轨迹长度

    Raindrop() : 
        position(0.0f),
        velocity(0.0f),
        color(1.0f),
        size(0.1f),
        lifespan(3.0f),
        lifetime(0.0f),
        visible(true),
        state(0),
        simulation(nullptr),
        brightness(1.0f),
        twinkleSpeed(0.0f),
        trail(0.0f) {
    }

    void init(const glm::vec3& _position, const glm::vec3& _color, RainSimulation* _simulation) {
        position = _position;
        color = _color;
        simulation = _simulation;
        velocity = glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f, // 轻微的横向运动
            -2.0f - static_cast<float>(rand()) / RAND_MAX * 3.0f,  // 更多速度变化
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f  // 轻微的前后运动
        );
        size = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.15f;
        lifespan = 3.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        lifetime = 0.0f;
        visible = true;
        state = 0;
        brightness = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        twinkleSpeed = 1.0f + static_cast<float>(rand()) / RAND_MAX * 5.0f;
        trail = 0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f; // 轨迹长度
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
    float pulseFrequency; // 脉动频率
    float pulseAmplitude; // 脉动幅度
    
    WaterRipple() : 
        position(0.0f),
        color(1.0f),
        radius(0.5f),
        maxRadius(5.0f),
        thickness(0.2f),
        opacity(0.8f),
        growthRate(2.0f),
        lifetime(0.0f),
        maxLifetime(2.0f),
        pulseFrequency(0.0f),
        pulseAmplitude(0.0f) {
    }
    
    void init(const glm::vec3& _position, const glm::vec3& _color) {
        position = _position;
        position.y = WATER_HEIGHT + 0.01f; // 略高于水面
        color = _color;
        radius = 0.2f; // 更小的初始半径
        maxRadius = 5.0f + static_cast<float>(rand()) / RAND_MAX * 15.0f;
        thickness = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.2f; // 更多厚度变化
        opacity = 0.7f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
        growthRate = 1.5f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        lifetime = 0.0f;
        maxLifetime = 1.5f + static_cast<float>(rand()) / RAND_MAX * 1.0f;
        pulseFrequency = 2.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        pulseAmplitude = 0.05f + static_cast<float>(rand()) / RAND_MAX * 0.1f;
    }
    
    bool update(float deltaTime) {
        lifetime += deltaTime;
        
        // 非线性增长 - 开始快，然后减慢
        float progress = lifetime / maxLifetime;
        float growthFactor = 1.0f - progress * 0.7f;
        radius += growthRate * deltaTime * growthFactor;
        
        // 加入脉动效果
        thickness = 0.1f + 0.1f * sinf(lifetime * pulseFrequency) * pulseAmplitude;
        
        // 逐渐减少不透明度，使用平滑的衰减曲线
        opacity = 0.8f * (1.0f - powf(progress, 1.5f));
        
        return isDead();
    }
    
    bool isDead() const {
        return radius >= maxRadius || opacity <= 0.05f || lifetime >= maxLifetime;
    }
    
    // 获取当前厚度，考虑脉动效果
    float getCurrentThickness() const {
        return thickness;
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
    std::unique_ptr<Shader> skyShader;    // 新增：天空着色器
    std::unique_ptr<Shader> moonShader;   // 新增：月亮着色器
    std::unique_ptr<Shader> starShader;   // 新增：星星着色器
    std::unique_ptr<Shader> trailShader;  // 新增：雨滴轨迹着色器
    
    // 几何体
    unsigned int waterVAO, waterVBO;
    unsigned int raindropVAO, raindropVBO;
    unsigned int rippleVAO, rippleVBO;
    unsigned int skyVAO, skyVBO;          // 新增：天空
    unsigned int moonVAO, moonVBO;        // 新增：月亮
    unsigned int starVAO, starVBO;        // 新增：星星
    unsigned int trailVAO, trailVBO;      // 新增：雨滴轨迹
    
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
    
    // 新增：星星和云朵
    std::vector<Star> stars;
    std::vector<Cloud> clouds;
    
    // 配置
    struct {
        int rainDensity = 80;  // 增加默认雨量
        float maxRippleSize = 15.0f;
        float updateInterval = 0.02f; // 更频繁的更新
        float rippleFadeSpeed = 0.02f;
        std::vector<glm::vec3> raindropColors = {
            glm::vec3(0.7f, 0.0f, 0.9f), // 紫色
            glm::vec3(0.0f, 0.8f, 1.0f), // 青色
            glm::vec3(1.0f, 0.9f, 0.0f), // 黄色
            glm::vec3(1.0f, 0.3f, 0.0f), // 橙色
            glm::vec3(0.0f, 0.9f, 0.4f)  // 青绿色
        };
        // 新增：水波颜色
        std::vector<glm::vec3> rippleColors = {
            glm::vec3(0.4f, 0.6f, 1.0f), // 浅蓝色
            glm::vec3(0.6f, 0.8f, 1.0f), // 浅青色
            glm::vec3(0.7f, 0.7f, 1.0f), // 浅紫色
            glm::vec3(0.5f, 0.7f, 0.9f), // 天蓝色
            glm::vec3(0.4f, 0.7f, 0.7f)  // 青灰色
        };
        // 雨滴大小范围
        float minRaindropSize = 0.1f;
        float maxRaindropSize = 0.3f;
        // 雨滴速度范围
        float minRaindropSpeed = 2.0f;
        float maxRaindropSpeed = 6.0f;
        // 星星闪烁速度
        float starTwinkleSpeed = 2.0f;
        // 水波环数
        int rippleRings = 3;
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
    float totalTime = 0.0f; // 总运行时间
    
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
        glDeleteVertexArrays(1, &skyVAO);
        glDeleteBuffers(1, &skyVBO);
        glDeleteVertexArrays(1, &moonVAO);
        glDeleteBuffers(1, &moonVBO);
        glDeleteVertexArrays(1, &starVAO);
        glDeleteBuffers(1, &starVBO);
        glDeleteVertexArrays(1, &trailVAO);
        glDeleteBuffers(1, &trailVBO);
        
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
        
        // 启用点大小
        glEnable(GL_PROGRAM_POINT_SIZE);
        
        // 初始化ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        
        // 设置ImGui样式
        ImGui::StyleColorsDark();
        
        // 解决中文显示问题 - 加载中文字体
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\simhei.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
        
        // 初始化ImGui的GLFW和OpenGL部分
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
        
        // 初始化星星
        initStars();
        
        // 初始化云朵
        initClouds();
        
        return true;
    }
    
    // 初始化星星
    void initStars() {
        stars.clear();
        
        for (int i = 0; i < STARS_COUNT; i++) {
            Star star;
            
            // 随机位置 - 在天空穹顶
            float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * glm::pi<float>();
            float phi = static_cast<float>(rand()) / RAND_MAX * glm::pi<float>() * 0.5f; // 上半球
            
            float radius = 200.0f + static_cast<float>(rand()) / RAND_MAX * 50.0f;
            star.position.x = radius * sin(phi) * cos(theta);
            star.position.y = radius * cos(phi) + 20.0f; // 向上偏移
            star.position.z = radius * sin(phi) * sin(theta);
            
            // 随机亮度和闪烁速度
            star.brightness = 0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f;
            star.twinkleSpeed = 0.5f + static_cast<float>(rand()) / RAND_MAX * 5.0f;
            star.size = 0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f;
            
            stars.push_back(star);
        }
    }
    
    // 初始化云朵
    void initClouds() {
        clouds.clear();
        
        for (int i = 0; i < CLOUD_COUNT; i++) {
            Cloud cloud;
            
            // 随机位置 - 在天空
            cloud.position.x = -100.0f + static_cast<float>(rand()) / RAND_MAX * 200.0f;
            cloud.position.y = 40.0f + static_cast<float>(rand()) / RAND_MAX * 30.0f;
            cloud.position.z = -100.0f + static_cast<float>(rand()) / RAND_MAX * 100.0f;
            
            // 随机大小、不透明度和速度
            cloud.size = 10.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f;
            cloud.opacity = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
            cloud.speed = 0.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
            
            clouds.push_back(cloud);
        }
    }
    
    void loadShaders() {
        // 加载水面着色器
        waterShader = std::make_unique<Shader>("shaders/water.vert", "shaders/water.frag");
        
        // 加载雨滴着色器
        raindropShader = std::make_unique<Shader>("shaders/raindrop.vert", "shaders/raindrop.frag");
        
        // 加载水波着色器
        rippleShader = std::make_unique<Shader>("shaders/ripple.vert", "shaders/ripple.frag");
        
        // 加载天空着色器
        skyShader = std::make_unique<Shader>("shaders/water.vert", "shaders/water.frag");
        
        // 加载月亮着色器
        moonShader = std::make_unique<Shader>("shaders/raindrop.vert", "shaders/raindrop.frag");
        
        // 加载星星着色器
        starShader = std::make_unique<Shader>("shaders/raindrop.vert", "shaders/raindrop.frag");
        
        // 加载雨滴轨迹着色器
        trailShader = std::make_unique<Shader>("shaders/ripple.vert", "shaders/ripple.frag");
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
        
        // 创建水波环 - 更多节点和细节
        std::vector<float> rippleVertices;
        const int segments = 128; // 增加细节
        
        // 创建多个同心环
        for (int ring = 0; ring < config.rippleRings; ring++) {
            float innerRadius = 0.7f + 0.1f * ring;
            float outerRadius = 0.9f + 0.1f * ring;
            
            for (int i = 0; i < segments; ++i) {
                float theta1 = 2.0f * glm::pi<float>() * float(i) / float(segments);
                float theta2 = 2.0f * glm::pi<float>() * float(i + 1) / float(segments);
                
                // 内环点
                rippleVertices.push_back(innerRadius * cos(theta1));
                rippleVertices.push_back(0.0f);
                rippleVertices.push_back(innerRadius * sin(theta1));
                
                // 外环点
                rippleVertices.push_back(outerRadius * cos(theta1));
                rippleVertices.push_back(0.0f);
                rippleVertices.push_back(outerRadius * sin(theta1));
                
                // 外环点 (下一个)
                rippleVertices.push_back(outerRadius * cos(theta2));
                rippleVertices.push_back(0.0f);
                rippleVertices.push_back(outerRadius * sin(theta2));
                
                // 内环点
                rippleVertices.push_back(innerRadius * cos(theta1));
                rippleVertices.push_back(0.0f);
                rippleVertices.push_back(innerRadius * sin(theta1));
                
                // 外环点 (下一个)
                rippleVertices.push_back(outerRadius * cos(theta2));
                rippleVertices.push_back(0.0f);
                rippleVertices.push_back(outerRadius * sin(theta2));
                
                // 内环点 (下一个)
                rippleVertices.push_back(innerRadius * cos(theta2));
                rippleVertices.push_back(0.0f);
                rippleVertices.push_back(innerRadius * sin(theta2));
            }
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
        
        // 创建天空穹顶
        std::vector<float> skyVertices;
        const int skySegments = 32;
        const float skyRadius = 300.0f;
        
        // 创建半球形天空
        for (int y = 0; y < skySegments / 2; y++) {
            for (int x = 0; x < skySegments; x++) {
                float theta1 = 2.0f * glm::pi<float>() * float(x) / float(skySegments);
                float theta2 = 2.0f * glm::pi<float>() * float(x + 1) / float(skySegments);
                
                float phi1 = glm::pi<float>() * float(y) / float(skySegments / 2);
                float phi2 = glm::pi<float>() * float(y + 1) / float(skySegments / 2);
                
                // 四个点构成两个三角形
                glm::vec3 p1(skyRadius * sin(phi1) * cos(theta1), skyRadius * cos(phi1), skyRadius * sin(phi1) * sin(theta1));
                glm::vec3 p2(skyRadius * sin(phi1) * cos(theta2), skyRadius * cos(phi1), skyRadius * sin(phi1) * sin(theta2));
                glm::vec3 p3(skyRadius * sin(phi2) * cos(theta2), skyRadius * cos(phi2), skyRadius * sin(phi2) * sin(theta2));
                glm::vec3 p4(skyRadius * sin(phi2) * cos(theta1), skyRadius * cos(phi2), skyRadius * sin(phi2) * sin(theta1));
                
                // 纹理坐标
                glm::vec2 t1(float(x) / float(skySegments), float(y) / float(skySegments / 2));
                glm::vec2 t2(float(x + 1) / float(skySegments), float(y) / float(skySegments / 2));
                glm::vec2 t3(float(x + 1) / float(skySegments), float(y + 1) / float(skySegments / 2));
                glm::vec2 t4(float(x) / float(skySegments), float(y + 1) / float(skySegments / 2));
                
                // 第一个三角形
                skyVertices.push_back(p1.x); skyVertices.push_back(p1.y); skyVertices.push_back(p1.z); skyVertices.push_back(t1.x); skyVertices.push_back(t1.y);
                skyVertices.push_back(p2.x); skyVertices.push_back(p2.y); skyVertices.push_back(p2.z); skyVertices.push_back(t2.x); skyVertices.push_back(t2.y);
                skyVertices.push_back(p3.x); skyVertices.push_back(p3.y); skyVertices.push_back(p3.z); skyVertices.push_back(t3.x); skyVertices.push_back(t3.y);
                
                // 第二个三角形
                skyVertices.push_back(p1.x); skyVertices.push_back(p1.y); skyVertices.push_back(p1.z); skyVertices.push_back(t1.x); skyVertices.push_back(t1.y);
                skyVertices.push_back(p3.x); skyVertices.push_back(p3.y); skyVertices.push_back(p3.z); skyVertices.push_back(t3.x); skyVertices.push_back(t3.y);
                skyVertices.push_back(p4.x); skyVertices.push_back(p4.y); skyVertices.push_back(p4.z); skyVertices.push_back(t4.x); skyVertices.push_back(t4.y);
            }
        }
        
        glGenVertexArrays(1, &skyVAO);
        glGenBuffers(1, &skyVBO);
        
        glBindVertexArray(skyVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
        glBufferData(GL_ARRAY_BUFFER, skyVertices.size() * sizeof(float), skyVertices.data(), GL_STATIC_DRAW);
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // 纹理坐标属性
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        // 创建月亮圆盘
        std::vector<float> moonVertices;
        const int moonSegments = 64;
        
        // 月亮中心点
        moonVertices.push_back(0.0f);
        moonVertices.push_back(0.0f);
        moonVertices.push_back(0.0f);
        
        // 创建月亮圆盘
        for (int i = 0; i <= moonSegments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(moonSegments);
            float x = cos(theta);
            float y = sin(theta);
            
            moonVertices.push_back(x);
            moonVertices.push_back(y);
            moonVertices.push_back(0.0f);
        }
        
        glGenVertexArrays(1, &moonVAO);
        glGenBuffers(1, &moonVBO);
        
        glBindVertexArray(moonVAO);
        glBindBuffer(GL_ARRAY_BUFFER, moonVBO);
        glBufferData(GL_ARRAY_BUFFER, moonVertices.size() * sizeof(float), moonVertices.data(), GL_STATIC_DRAW);
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        // 创建雨滴轨迹几何体
        float trailVertices[] = {
            -0.5f, 0.0f, 0.0f,
             0.5f, 0.0f, 0.0f,
             0.0f, 2.0f, 0.0f
        };
        
        glGenVertexArrays(1, &trailVAO);
        glGenBuffers(1, &trailVBO);
        
        glBindVertexArray(trailVAO);
        glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(trailVertices), trailVertices, GL_STATIC_DRAW);
        
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
            // 创建一个8x8的棋盘格纹理，用于替代丢失的纹理
            unsigned char defaultTextureData[8*8*4];
            
            if (strstr(path, "normal")) {
                // 法线贴图的默认值: 随机法线
                for (int i = 0; i < 8*8; i++) {
                    // 随机法线 - 映射为RGB值
                    float nx = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.2f;
                    float ny = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
                    float nz = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.2f;
                    
                    float length = sqrt(nx*nx + ny*ny + nz*nz);
                    nx /= length;
                    ny /= length;
                    nz /= length;
                    
                    // 转换为RGB (0-255)
                    defaultTextureData[i*4+0] = static_cast<unsigned char>((nx * 0.5f + 0.5f) * 255); // R
                    defaultTextureData[i*4+1] = static_cast<unsigned char>((ny * 0.5f + 0.5f) * 255); // G
                    defaultTextureData[i*4+2] = static_cast<unsigned char>((nz * 0.5f + 0.5f) * 255); // B
                    defaultTextureData[i*4+3] = 255; // A
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "DuDv")) {
                // DuDv贴图的默认值: 随机扰动
                for (int i = 0; i < 8*8; i++) {
                    // 随机扰动值
                    defaultTextureData[i*4+0] = 128 + rand() % 40 - 20; // R
                    defaultTextureData[i*4+1] = 128 + rand() % 40 - 20; // G
                    defaultTextureData[i*4+2] = 128; // B
                    defaultTextureData[i*4+3] = 255; // A
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "Reflection")) {
                // 反射贴图默认值: 生成随机的夜空反射纹理
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int i = y*8 + x;
                        
                        // 基础深蓝色调
                        defaultTextureData[i*4+0] = 10 + rand() % 20;  // R
                        defaultTextureData[i*4+1] = 20 + rand() % 30;  // G
                        defaultTextureData[i*4+2] = 40 + rand() % 50;  // B
                        
                        // 偶尔添加星星反射
                        if (rand() % 20 == 0) {
                            defaultTextureData[i*4+0] = 200 + rand() % 55; // R
                            defaultTextureData[i*4+1] = 200 + rand() % 55; // G
                            defaultTextureData[i*4+2] = 200 + rand() % 55; // B
                        }
                        
                        defaultTextureData[i*4+3] = 255; // A
                    }
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "glow")) {
                // 光晕贴图: 圆形渐变
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int i = y*8 + x;
                        
                        // 计算到中心的距离
                        float dx = (x - 3.5f) / 3.5f;
                        float dy = (y - 3.5f) / 3.5f;
                        float dist = sqrt(dx*dx + dy*dy);
                        
                        // 圆形渐变亮度
                        float brightness = 1.0f - std::min(dist, 1.0f);
                        brightness = brightness * brightness; // 平方使边缘更柔和
                        
                        defaultTextureData[i*4+0] = static_cast<unsigned char>(brightness * 255); // R
                        defaultTextureData[i*4+1] = static_cast<unsigned char>(brightness * 255); // G
                        defaultTextureData[i*4+2] = static_cast<unsigned char>(brightness * 255); // B
                        defaultTextureData[i*4+3] = static_cast<unsigned char>(brightness * 255); // A
                    }
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "sky")) {
                // 天空贴图: 渐变夜空
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int i = y*8 + x;
                        
                        // 从底部到顶部的渐变
                        float gradient = static_cast<float>(y) / 7.0f;
                        
                        // 深蓝到黑色渐变
                        defaultTextureData[i*4+0] = static_cast<unsigned char>(5 + (1.0f - gradient) * 15); // R
                        defaultTextureData[i*4+1] = static_cast<unsigned char>(10 + (1.0f - gradient) * 20); // G
                        defaultTextureData[i*4+2] = static_cast<unsigned char>(30 + (1.0f - gradient) * 50); // B
                        
                        // 随机添加星星
                        if (rand() % 30 == 0) {
                            defaultTextureData[i*4+0] = 200 + rand() % 55; // R
                            defaultTextureData[i*4+1] = 200 + rand() % 55; // G
                            defaultTextureData[i*4+2] = 200 + rand() % 55; // B
                        }
                        
                        defaultTextureData[i*4+3] = 255; // A
                    }
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else {
                // 默认: 棋盘格纹理
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int i = y*8 + x;
                        bool isWhite = (x + y) % 2 == 0;
                        
                        defaultTextureData[i*4+0] = isWhite ? 100 : 50; // R
                        defaultTextureData[i*4+1] = isWhite ? 150 : 100; // G
                        defaultTextureData[i*4+2] = isWhite ? 255 : 200; // B
                        defaultTextureData[i*4+3] = 255; // A
                    }
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
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
        
        // 创建一个改进的8x8纹理
        const int width = 8;
        const int height = 8;
        const int channels = 4; // RGBA
        unsigned char data[width * height * channels];
        
        // 填充数据 - 根据纹理类型使用不同的模式
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int i = (y * width + x) * channels;
                
                if (path.find("normal") != std::string::npos) {
                    // 法线贴图: 随机凹凸
                    data[i+0] = 128 + (rand() % 40 - 20); // R
                    data[i+1] = 128 + (rand() % 40 - 20); // G
                    data[i+2] = 200 + (rand() % 55); // B - 主要向上
                    data[i+3] = 255; // A
                } else if (path.find("DuDv") != std::string::npos) {
                    // DuDv贴图: 随机扰动
                    data[i+0] = 128 + (rand() % 30 - 15); // R
                    data[i+1] = 128 + (rand() % 30 - 15); // G
                    data[i+2] = 128; // B
                    data[i+3] = 255; // A
                } else if (path.find("Reflection") != std::string::npos) {
                    // 反射贴图: 夜空星星效果
                    data[i+0] = 10 + (rand() % 20); // R
                    data[i+1] = 20 + (rand() % 30); // G
                    data[i+2] = 50 + (rand() % 40); // B
                    
                    // 随机点缀星星
                    if (rand() % 20 == 0) {
                        data[i+0] = 200 + (rand() % 55); // R
                        data[i+1] = 200 + (rand() % 55); // G
                        data[i+2] = 200 + (rand() % 55); // B
                    }
                    data[i+3] = 255; // A
                } else if (path.find("glow") != std::string::npos) {
                    // 光晕贴图: 圆形渐变
                    float dx = (x - width/2.0f) / (width/2.0f);
                    float dy = (y - height/2.0f) / (height/2.0f);
                    float dist = sqrt(dx*dx + dy*dy);
                    float intensity = std::max(0.0f, 1.0f - dist);
                    intensity = intensity * intensity; // 平方使边缘更柔和
                    
                    data[i+0] = static_cast<unsigned char>(255 * intensity); // R
                    data[i+1] = static_cast<unsigned char>(255 * intensity); // G
                    data[i+2] = static_cast<unsigned char>(255 * intensity); // B
                    data[i+3] = static_cast<unsigned char>(255 * intensity); // A
                } else if (path.find("sky") != std::string::npos) {
                    // 天空贴图: 夜空渐变
                    float gradient = static_cast<float>(y) / height;
                    
                    data[i+0] = static_cast<unsigned char>(5 + (1.0f - gradient) * 15); // R
                    data[i+1] = static_cast<unsigned char>(10 + (1.0f - gradient) * 20); // G
                    data[i+2] = static_cast<unsigned char>(30 + (1.0f - gradient) * 70); // B
                    
                    // 随机添加星星
                    if (rand() % 20 == 0) {
                        data[i+0] = 200 + (rand() % 55); // R
                        data[i+1] = 200 + (rand() % 55); // G
                        data[i+2] = 200 + (rand() % 55); // B
                    }
                    
                    data[i+3] = 255; // A
                } else {
                    // 默认: 蓝色渐变
                    float t = static_cast<float>(y) / height;
                    data[i+0] = static_cast<unsigned char>(50 * (1.0f-t)); // R
                    data[i+1] = static_cast<unsigned char>(80 * (1.0f-t) + 20); // G
                    data[i+2] = static_cast<unsigned char>(120 * (1.0f-t) + 80); // B
                    data[i+3] = 255; // A
                }
            }
        }
        
        // 写入文件
        if (path.find(".png") != std::string::npos) {
            stbi_write_png(path.c_str(), width, height, channels, data, width * channels);
        } else {
            stbi_write_jpg(path.c_str(), width, height, channels, data, 95); // 95是质量设置(0-100)
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
            totalTime += deltaTime;
            
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
            
        // 相机旋转 - 方向键
        float rotateSpeed = 30.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPitch += rotateSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPitch -= rotateSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraYaw -= rotateSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraYaw += rotateSpeed;
            
        // 限制相机俯仰角
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
            
        // 更新相机前向量
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        cameraFront = glm::normalize(front);
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
        
        // 更新星星闪烁
        for (auto& star : stars) {
            star.brightness = 0.5f + 0.5f * sin(totalTime * star.twinkleSpeed);
        }
        
        // 更新云朵位置
        for (auto& cloud : clouds) {
            cloud.position.x += cloud.speed * deltaTime;
            
            // 如果云朵移出视野，重新放置到另一侧
            if (cloud.position.x > POND_SIZE) {
                cloud.position.x = -POND_SIZE;
                cloud.position.z = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                cloud.opacity = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
            }
        }
    }
    
    void generateRaindrops() {
        int raindropsToGenerate = config.rainDensity / 20;
        
        for (int i = 0; i < raindropsToGenerate; ++i) {
            if (rand() % 100 < 20) { // 20%的概率生成雨滴
                Raindrop raindrop;
                
                // 随机位置 - 在水面上方的更大区域
                float x = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                float z = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                float y = 20.0f + static_cast<float>(rand()) / RAND_MAX * 15.0f;
                
                // 随机颜色
                int colorIndex = rand() % config.raindropColors.size();
                
                raindrop.init(glm::vec3(x, y, z), config.raindropColors[colorIndex], this);
                raindrops.push_back(raindrop);
            }
        }
    }
    
    void render() {
        // 清空缓冲
        glClearColor(0.01f, 0.02f, 0.05f, 1.0f); // 更深的夜空颜色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 视图/投影矩阵
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        
        // 渲染天空
        renderSky(view, projection);
        
        // 渲染月亮
        renderMoon(view, projection);
        
        // 渲染星星
        renderStars(view, projection);
        
        // 渲染水面
        renderWater(view, projection);
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
        waterShader->setFloat("time", totalTime); // 使用总时间使水面动起来
        waterShader->setVec3("viewPos", cameraPos);
        waterShader->setFloat("waveStrength", 0.05f + 0.02f * sin(totalTime * 0.5f)); // 波浪强度随时间变化
        waterShader->setFloat("waveSpeed", 0.8f); // 波浪速度
        waterShader->setFloat("waterDepth", 0.3f + 0.05f * sin(totalTime * 0.3f)); // 水深度
        
        // 绘制水面
        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
    
    void renderRaindrops(const glm::mat4& view, const glm::mat4& projection) {
        // 先渲染雨滴轨迹
        trailShader->use();
        trailShader->setMat4("view", view);
        trailShader->setMat4("projection", projection);
        
        glBindVertexArray(trailVAO);
        for (const auto& raindrop : raindrops) {
            if (!raindrop.visible || raindrop.state > 0 || raindrop.velocity.y > -2.0f)
                continue;
                
            // 设置模型矩阵 - 根据雨滴速度方向缩放和旋转轨迹
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, raindrop.position);
            
            // 根据雨滴速度确定轨迹方向
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 direction = glm::normalize(-raindrop.velocity);
            float angleY = atan2(direction.x, direction.z);
            float angleX = acos(glm::dot(direction, up));
            
            model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, angleX, glm::vec3(1.0f, 0.0f, 0.0f));
            
            // 根据速度和大小缩放轨迹
            float trailLength = glm::length(raindrop.velocity) * raindrop.trail * 0.5f;
            model = glm::scale(model, glm::vec3(raindrop.size * 0.4f, trailLength, raindrop.size * 0.4f));
            
            trailShader->setMat4("model", model);
            
            // 设置轨迹颜色 - 稍微透明
            glm::vec3 trailColor = raindrop.color * 0.9f;
            float trailOpacity = 0.3f * raindrop.brightness;
            trailShader->setVec3("rippleColor", trailColor);
            trailShader->setFloat("opacity", trailOpacity);
            
            // 绘制轨迹
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        
        // 然后渲染雨滴点
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
            
            // 设置雨滴属性 - 大小随距离变化
            float distanceFactor = glm::length(raindrop.position - cameraPos);
            float sizeScale = std::max(0.5f, 30.0f / distanceFactor);
            
            raindropShader->setVec3("raindropColor", raindrop.color * raindrop.brightness);
            raindropShader->setFloat("raindropSize", raindrop.size * 15.0f * sizeScale); // 放大点大小
            
            // 添加闪烁效果
            float twinkle = 0.7f + 0.3f * sin(totalTime * raindrop.twinkleSpeed);
            raindropShader->setFloat("brightness", raindrop.brightness * twinkle);
            
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
            
            // 随时间轻微旋转水波
            model = glm::rotate(model, totalTime * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
            
            // 根据当前厚度稍微缩放不同的环
            float thicknessFactor = ripple.getCurrentThickness() / 0.2f;
            model = glm::scale(model, glm::vec3(ripple.radius * (1.0f + 0.02f * thicknessFactor)));
            
            rippleShader->setMat4("model", model);
            
            // 设置水波属性 - 颜色随时间和厚度变化
            float colorPulse = 0.9f + 0.1f * sin(totalTime * ripple.pulseFrequency);
            glm::vec3 pulsingColor = ripple.color * colorPulse;
            
            rippleShader->setVec3("rippleColor", pulsingColor);
            rippleShader->setFloat("opacity", ripple.opacity);
            
            // 绘制水波 - 三角形数量取决于环数和节点数
            glDrawArrays(GL_TRIANGLES, 0, 6 * 128 * config.rippleRings);
        }
        glBindVertexArray(0);
    }
    
    // 新增：渲染天空
    void renderSky(const glm::mat4& view, const glm::mat4& projection) {
        skyShader->use();
        
        // 设置变换矩阵 - 使天空跟随相机移动
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(cameraPos.x, 0.0f, cameraPos.z));
        skyShader->setMat4("model", model);
        skyShader->setMat4("view", view);
        skyShader->setMat4("projection", projection);
        
        // 设置天空属性
        skyShader->setFloat("time", totalTime * 0.01f); // 非常缓慢的移动
        skyShader->setVec3("viewPos", cameraPos);
        
        // 设置纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyTexture);
        skyShader->setInt("normalMap", 0); // 使用天空纹理代替法线图
        
        // 绘制天空
        glDepthMask(GL_FALSE); // 禁用深度写入，确保天空在后面
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 32 * 16 * 6); // 天空穹顶的三角形数量
        glBindVertexArray(0);
        glDepthMask(GL_TRUE); // 重新启用深度写入
    }
    
    // 新增：渲染月亮
    void renderMoon(const glm::mat4& view, const glm::mat4& projection) {
        moonShader->use();
        
        // 设置月亮位置
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(MOON_X, MOON_Y, -100.0f));
        model = glm::scale(model, glm::vec3(MOON_SIZE));
        moonShader->setMat4("model", model);
        moonShader->setMat4("view", view);
        moonShader->setMat4("projection", projection);
        
        // 设置月亮颜色 - 淡黄色
        glm::vec3 moonColor(0.98f, 0.97f, 0.85f);
        moonShader->setVec3("raindropColor", moonColor);
        moonShader->setFloat("raindropSize", 1.0f); // 不需要点大小，因为我们绘制的是三角形
        moonShader->setFloat("brightness", 1.0f);
        
        // 绘制月亮
        glBindVertexArray(moonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 66); // 圆盘的顶点数量
        glBindVertexArray(0);
        
        // 绘制月晕
        float haloSize = MOON_SIZE * 1.5f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(MOON_X, MOON_Y, -100.0f));
        model = glm::scale(model, glm::vec3(haloSize));
        moonShader->setMat4("model", model);
        
        // 设置月晕颜色 - 透明淡蓝色
        glm::vec3 haloColor(0.6f, 0.7f, 0.9f);
        moonShader->setVec3("raindropColor", haloColor);
        moonShader->setFloat("brightness", 0.4f);
        
        glBindVertexArray(moonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 66);
        glBindVertexArray(0);
    }
    
    // 新增：渲染星星
    void renderStars(const glm::mat4& view, const glm::mat4& projection) {
        starShader->use();
        
        // 设置变换矩阵
        starShader->setMat4("view", view);
        starShader->setMat4("projection", projection);
        
        // 启用点大小
        glEnable(GL_PROGRAM_POINT_SIZE);
        
        // 绘制所有星星
        glBindVertexArray(raindropVAO); // 复用雨滴VAO
        
        for (const auto& star : stars) {
            // 设置模型矩阵
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, star.position);
            starShader->setMat4("model", model);
            
            // 设置星星属性
            glm::vec3 starColor(0.9f, 0.9f, 1.0f); // 白色偏蓝
            starShader->setVec3("raindropColor", starColor);
            starShader->setFloat("raindropSize", star.size * 2.0f);
            starShader->setFloat("brightness", star.brightness);
            
            // 绘制星星
            glDrawArrays(GL_POINTS, 0, 1);
        }
        
        glBindVertexArray(0);
        glDisable(GL_PROGRAM_POINT_SIZE);
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
        if (ImGui::CollapsingHeader("雨滴设置")) {
            ImGui::SliderInt("雨滴密度", &config.rainDensity, 10, 300);
            ImGui::SliderFloat("雨滴最小大小", &config.minRaindropSize, 0.05f, 0.3f);
            ImGui::SliderFloat("雨滴最大大小", &config.maxRaindropSize, 0.1f, 0.5f);
            ImGui::SliderFloat("雨滴最小速度", &config.minRaindropSpeed, 1.0f, 5.0f);
            ImGui::SliderFloat("雨滴最大速度", &config.maxRaindropSpeed, 3.0f, 10.0f);
        }
        
        if (ImGui::CollapsingHeader("水波设置")) {
            ImGui::SliderFloat("最大水圈大小", &config.maxRippleSize, 5.0f, 30.0f);
            ImGui::SliderInt("水波环数", &config.rippleRings, 1, 5);
            ImGui::SliderFloat("水波更新间隔", &config.updateInterval, 0.01f, 0.1f);
        }
        
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
        
        if (ImGui::CollapsingHeader("相机控制")) {
            ImGui::Text("W/S/A/D: 移动相机");
            ImGui::Text("空格/Shift: 上升/下降");
            ImGui::Text("方向键: 旋转视角");
            
            ImGui::SliderFloat("相机X", &cameraPos.x, -100.0f, 100.0f);
            ImGui::SliderFloat("相机Y", &cameraPos.y, 1.0f, 50.0f);
            ImGui::SliderFloat("相机Z", &cameraPos.z, -100.0f, 100.0f);
            ImGui::SliderFloat("俯仰角", &cameraPitch, -89.0f, 89.0f);
            ImGui::SliderFloat("偏航角", &cameraYaw, -180.0f, 180.0f);
        }
        
        ImGui::Separator();
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
    
    // 随机闪烁，使用亮度而不是可见性
    brightness = 0.7f + 0.3f * sin(lifetime * twinkleSpeed + position.x * 0.1f);
    
    if (state == 0) { // 下落状态
        position += velocity * deltaTime;
        
        // 添加一些随机运动 - 雨滴轻微摆动
        float swayAmount = 0.05f;
        velocity.x += (cos(lifetime * 3.0f + position.z) * swayAmount - velocity.x * 0.1f) * deltaTime;
        velocity.z += (sin(lifetime * 2.5f + position.x) * swayAmount - velocity.z * 0.1f) * deltaTime;
        
        // 速度随时间略微增加 - 模拟重力
        velocity.y -= 0.2f * deltaTime;
        
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
    } else if (state == 1) { // 入水状态
        // 如果在水面下，逐渐消失
        brightness -= deltaTime * 2.0f;
        if (brightness <= 0.0f) {
            state = 2; // 消失状态
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
    
    // 顶点着色器 - 水面 (增强版)
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
uniform float waveStrength;
uniform float waveSpeed;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // 多层波浪效果
    vec3 pos = aPos;
    float wave1 = sin(pos.x * 0.1 + time * waveSpeed) * cos(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    float wave2 = sin(pos.x * 0.2 + time * waveSpeed * 1.2) * cos(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    float wave3 = sin(pos.x * 0.05 + time * waveSpeed * 0.7) * cos(pos.z * 0.06 + time * waveSpeed * 0.9) * waveStrength * 0.3;
    
    pos.y = wave1 + wave2 + wave3;
    
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoords = aTexCoords;
    
    // 波面法线计算 (基于波浪导数)
    float dx1 = 0.1 * cos(pos.x * 0.1 + time * waveSpeed) * cos(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    float dz1 = 0.1 * sin(pos.x * 0.1 + time * waveSpeed) * -sin(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    
    float dx2 = 0.2 * cos(pos.x * 0.2 + time * waveSpeed * 1.2) * cos(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    float dz2 = 0.15 * sin(pos.x * 0.2 + time * waveSpeed * 1.2) * -sin(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    
    vec3 tangent = normalize(vec3(1.0, dx1 + dx2, 0.0));
    vec3 bitangent = normalize(vec3(0.0, dz1 + dz2, 1.0));
    Normal = normalize(cross(tangent, bitangent));
}
)";

    // 片段着色器 - 水面 (增强版)
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
uniform float waterDepth;
uniform float waveStrength;

void main() {
    // 生成扰动纹理坐标
    vec2 distortedTexCoords = vec2(
        TexCoords.x + sin(TexCoords.y * 10.0 + time) * 0.01,
        TexCoords.y + sin(TexCoords.x * 10.0 + time * 0.8) * 0.01
    );
    
    // 在没有法线贴图的情况下生成动态法线
    vec3 normal = normalize(Normal);
    
    // 动态变化法线以模拟水面微小波动
    normal.x += sin(TexCoords.x * 30.0 + time * 3.0) * sin(TexCoords.y * 20.0 + time * 2.0) * 0.03;
    normal.z += cos(TexCoords.x * 25.0 + time * 2.5) * cos(TexCoords.y * 35.0 + time * 3.5) * 0.03;
    normal = normalize(normal);
    
    // 环境光
    vec3 ambient = vec3(0.05, 0.1, 0.2);
    
    // 漫反射 - 使用多个光源
    vec3 result = ambient;
    
    // 主光源 - 月光
    {
        vec3 lightDir = normalize(vec3(0.3, 1.0, 0.1));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.6, 0.7, 0.9) * 0.3; // 柔和的蓝白色月光
        
        // 反射
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = spec * vec3(0.8, 0.9, 1.0) * 0.5;
        
        result += diffuse + specular;
    }
    
    // 第二光源 - 环境光
    {
        vec3 lightDir = normalize(vec3(-0.5, 0.5, 0.2));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.1, 0.2, 0.4) * 0.1; // 微弱的蓝色环境光
        
        result += diffuse;
    }
    
    // 水面颜色 - 混合深浅色调
    vec3 waterColorDeep = vec3(0.0, 0.05, 0.15); // 深水
    vec3 waterColorShallow = vec3(0.1, 0.3, 0.6); // 浅水
    
    // 视角越垂直看水面，越能看到水底
    float fresnelFactor = pow(1.0 - max(dot(normal, normalize(viewPos - FragPos)), 0.0), 3.0);
    
    // 根据视角和波浪动态混合深浅水颜色
    vec3 waterColor = mix(waterColorDeep, waterColorShallow, 
                          fresnelFactor * 0.5 + 0.2 * sin(time * 0.1) + 0.3);
    
    // 倒影效果 - 在没有倒影纹理的情况下生成模拟倒影
    vec3 reflection = vec3(0.0);
    
    // 生成模拟的夜空倒影
    float skyFresnel = pow(1.0 - max(dot(normal, normalize(viewPos - FragPos)), 0.0), 2.0);
    vec3 skyColor = vec3(0.0, 0.02, 0.1); // 深蓝色夜空
    
    // 添加模拟的月光倒影和星星
    vec2 moonPos = vec2(0.7, 0.8); // 月亮在倒影中的位置
    float moonDist = distance(distortedTexCoords, moonPos);
    vec3 moonColor = vec3(0.8, 0.8, 0.6) * smoothstep(0.15, 0.0, moonDist) * 0.8;
    
    // 随机星星
    float stars = 0.0;
    if (fract(sin(distortedTexCoords.x * 100.0) * sin(distortedTexCoords.y * 100.0) * 43758.5453) > 0.996) {
        stars = 0.5 + 0.5 * sin(time * 2.0 + distortedTexCoords.x * 10.0);
    }
    
    reflection = skyColor + moonColor + stars * vec3(0.8, 0.8, 1.0);
    
    // 最终混合所有组件
    result = mix(result, reflection, skyFresnel * 0.5);
    result = mix(waterColor, result, 0.5);
    
    // 添加波纹边缘高光
    float edgeHighlight = pow(1.0 - abs(dot(normal, vec3(0.0, 1.0, 0.0))), 8.0) * 0.5;
    result += vec3(edgeHighlight);
    
    // 水面半透明效果
    float alpha = 0.8 + edgeHighlight * 0.2;
    
    FragColor = vec4(result, alpha);
}
)";

    // 顶点着色器 - 雨滴 (增强版)
    const char* raindropVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float raindropSize;
uniform vec3 raindropColor;
uniform float brightness;

out vec3 Color;
out float Brightness;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = raindropSize / gl_Position.w; // 根据距离调整大小
    Color = raindropColor;
    Brightness = brightness;
}
)";

    // 片段着色器 - 雨滴 (增强版)
    const char* raindropFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 Color;
in float Brightness;

void main() {
    // 创建圆形点
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist = length(circCoord);
    
    // 平滑圆形边缘
    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);
    
    // 淡出边缘并提亮中心
    if (dist > 1.0) {
        discard;
    }
    
    // 创建雨滴光晕效果
    float innerGlow = 1.0 - dist * dist;
    float outerGlow = 0.5 * (1.0 - smoothstep(0.5, 1.0, dist));
    
    // 根据雨滴大小调整亮度和透明度
    vec3 finalColor = Color * Brightness * (0.7 + 0.6 * innerGlow);
    float finalAlpha = alpha * (0.6 + 0.4 * innerGlow);
    
    // 添加轻微的内部结构
    float detail = 0.1 * sin(circCoord.x * 10.0) * sin(circCoord.y * 10.0);
    finalColor += detail * innerGlow * Brightness;
    
    FragColor = vec4(finalColor, finalAlpha);
}
)";

    // 顶点着色器 - 水波 (增强版)
    const char* rippleVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    // 片段着色器 - 水波 (增强版)
    const char* rippleFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 rippleColor;
uniform float opacity;

void main() {
    // 模拟水波的闪光和透明度变化
    vec3 color = rippleColor;
    
    // 使用片段位置来计算水波的径向位置
    vec3 center = vec3(0.0, 0.0, 0.0); // 水波中心在模型空间
    vec2 fromCenter = vec2(FragPos.x, FragPos.z);
    float dist = length(fromCenter);
    
    // 添加纹理变化 - 让水波看起来更细致
    float detail = sin(dist * 20.0) * 0.1;
    color += detail * rippleColor;
    
    // 平滑边缘
    float edgeFade = smoothstep(0.9, 1.0, dist);
    float innerFade = smoothstep(0.0, 0.2, dist);
    
    // 最终颜色和透明度
    color = color * (1.0 - edgeFade) * innerFade;
    float alpha = opacity * (1.0 - edgeFade) * innerFade;
    
    FragColor = vec4(color, alpha);
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
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
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