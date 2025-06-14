// main.cpp - Main program entry

// !!!IMPORTANT!!! SDL.h must be the first included header for proper WinMain definition
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

// Set console code page to UTF-8 or GBK
void setConsoleCodePage() {
    // Use UTF-8 code page (65001)
    SetConsoleOutputCP(65001);
    // Or use Simplified Chinese GBK code page (936)
    // SetConsoleOutputCP(936);
}
#else
void setConsoleCodePage() {
    // No need to set for non-Windows systems
}
#endif

// Add custom filesystem compatibility layer
#include "filesystem_compat.h"

// Texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// Image writing
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

// Ensure using SDL's main
#define SDL_MAIN_HANDLED

// OpenGL related libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui library (for UI)
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

void writeShaderFiles();

// Constants
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float POND_SIZE = 500.0f;
const float WATER_HEIGHT = 0.0f;

// Additional constants for better visuals without textures
const int STARS_COUNT = 150;      // 减少星星数量，避免过于密集
const int CLOUD_COUNT = 5;        // Number of clouds
const float MOON_SIZE = 20.0f;    // Moon size
const float MOON_X = 70.0f;       // Moon X coordinate
const float MOON_Y = 60.0f;       // Moon Y coordinate

// Star structure
struct Star {
    glm::vec3 position;
    float brightness;
    float twinkleSpeed;
    float size;
};

// Cloud structure
struct Cloud {
    glm::vec3 position;
    float size;
    float opacity;
    float speed;
};

// Lightning structure - 新增闪电结构
struct Lightning {
    std::vector<glm::vec3> segments;  // 闪电路径段
    glm::vec3 color;
    float intensity;
    float duration;
    float currentTime;
    float thickness;
    bool active;
    int branches;  // 分支数量
    
    Lightning() : 
        color(0.9f, 0.9f, 1.0f),
        intensity(1.0f),
        duration(0.3f),
        currentTime(0.0f),
        thickness(2.0f),
        active(false),
        branches(0) {}
        
    void generate(const glm::vec3& start, const glm::vec3& end) {
        segments.clear();
        
        glm::vec3 current = start;
        glm::vec3 target = end;
        
        // 主路径
        int numSegments = 8 + rand() % 6;  // 8-13个段
        for (int i = 0; i <= numSegments; i++) {
            float t = float(i) / numSegments;
            
            // 基础路径插值
            glm::vec3 point = glm::mix(start, end, t);
            
            // 添加随机偏移创造锯齿效果
            if (i > 0 && i < numSegments) {
                float maxOffset = 15.0f * (1.0f - abs(t - 0.5f) * 2.0f);  // 中间偏移更大
                point.x += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * maxOffset;
                point.z += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * maxOffset;
                point.y += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * maxOffset * 0.5f;
            }
            
            segments.push_back(point);
        }
        
        // 随机颜色变化
        color = glm::vec3(
            0.7f + static_cast<float>(rand()) / RAND_MAX * 0.3f,  // R: 0.7-1.0
            0.8f + static_cast<float>(rand()) / RAND_MAX * 0.2f,  // G: 0.8-1.0  
            0.9f + static_cast<float>(rand()) / RAND_MAX * 0.1f   // B: 0.9-1.0
        );
        
        intensity = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        duration = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        thickness = 1.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
        branches = rand() % 3;  // 0-2个分支
        
        currentTime = 0.0f;
        active = true;
    }
    
    bool update(float deltaTime) {
        if (!active) return false;
        
        currentTime += deltaTime;
        
        // 强度衰减
        float progress = currentTime / duration;
        intensity = (1.0f - progress) * (0.8f + 0.2f * sin(currentTime * 50.0f));
        
        if (currentTime >= duration) {
            active = false;
            return false;
        }
        
        return true;
    }
};

// Shader class
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        // 1. Read vertex and fragment shader source from files
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
            
            // Try looking for files in the project root if build directory fails
            try {
                // Try looking one directory up
                std::string projectRootVertPath = std::string("../") + vertexPath;
                std::string projectRootFragPath = std::string("../") + fragmentPath;
                
                vShaderFile.open(projectRootVertPath);
                fShaderFile.open(projectRootFragPath);
                
                std::stringstream vShaderStream, fShaderStream;
                
                vShaderStream << vShaderFile.rdbuf();
                fShaderStream << fShaderFile.rdbuf();
                
                vShaderFile.close();
                fShaderFile.close();
                
                vertexCode = vShaderStream.str();
                fragmentCode = fShaderStream.str();
                
                std::cout << "Successfully loaded shaders from project root directory" << std::endl;
            }
            catch(std::ifstream::failure& e2) {
                std::cerr << "ERROR: Failed to read shader files from both build and project directory" << std::endl;
                
                // If we can't load the file, use hardcoded shaders
                // This will be handled in writeShaderFiles() function
            }
        }
        
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        
        // 2. Compile shaders
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];
        
        // Vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        
        // Fragment shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        
        // Shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        
        // Delete shaders - they're linked to our program and no longer needed
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // Activate shader
    void use() {
        glUseProgram(ID);
    }

    // Uniform utility functions
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

// Forward declaration for application class
class RainSimulation;

// Enhanced Raindrop class with trail effect
class Raindrop {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float size;
    float lifespan;
    float lifetime;
    bool visible;
    int state; // 0: falling, 1: entered water, 2: disappeared
    RainSimulation* simulation;
    float brightness;
    float twinkleSpeed;
    
    // 新增拖尾效果相关属性
    std::vector<glm::vec3> trailPositions;  // 拖尾位置历史
    std::vector<float> trailAlphas;         // 拖尾透明度历史
    int maxTrailLength;                     // 最大拖尾长度
    float trailUpdateTime;                  // 拖尾更新计时器
    float trailUpdateInterval;              // 拖尾更新间隔
    float distanceFromCamera;               // 距离摄像机的距离
    float layerDepth;                       // 层次深度 (0=近, 1=远)

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
        maxTrailLength(8),
        trailUpdateTime(0.0f),
        trailUpdateInterval(0.05f),
        distanceFromCamera(0.0f),
        layerDepth(0.0f) {
        trailPositions.reserve(maxTrailLength);
        trailAlphas.reserve(maxTrailLength);
    }

    void init(const glm::vec3& _position, const glm::vec3& _color, RainSimulation* _simulation) {
        position = _position;
        color = _color;
        simulation = _simulation;
        
        // 根据距离调整雨滴属性 - 实现层次感
        distanceFromCamera = glm::length(_position - glm::vec3(0.0f, 60.0f, 120.0f)); // 假设摄像机位置
        layerDepth = std::min(distanceFromCamera / 200.0f, 1.0f); // 0-1范围
        
        velocity = glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.0f, // 增加水平运动
            -3.0f - static_cast<float>(rand()) / RAND_MAX * 5.0f,  // 更大的垂直速度变化
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 1.0f
        );
        
        // 近处雨滴更大更慢，远处雨滴更小更快
        size = (2.0f - layerDepth) * (1.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f);
        velocity.y *= 0.7f + layerDepth * 0.6f; // 远处雨滴下落更快
        
        lifespan = 4.0f + static_cast<float>(rand()) / RAND_MAX * 4.0f;
        lifetime = 0.0f;
        visible = true;
        state = 0;
        brightness = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        twinkleSpeed = 1.0f + static_cast<float>(rand()) / RAND_MAX * 5.0f;
        
        // 初始化拖尾系统
        maxTrailLength = 4 + static_cast<int>((1.0f - layerDepth) * 8); // 近处拖尾更长
        trailUpdateInterval = 0.03f + layerDepth * 0.02f; // 远处更新更快
        trailPositions.clear();
        trailAlphas.clear();
        trailUpdateTime = 0.0f;
    }

    bool update(float deltaTime);  // Declaration, implemented after RainSimulation class

    bool isDead() const {
        return state > 1 || lifetime > lifespan;
    }
    
    // 更新拖尾效果
    void updateTrail(float deltaTime) {
        trailUpdateTime += deltaTime;
        
        if (trailUpdateTime >= trailUpdateInterval) {
            // 添加当前位置到拖尾
            trailPositions.insert(trailPositions.begin(), position);
            trailAlphas.insert(trailAlphas.begin(), brightness);
            
            // 保持拖尾长度
            if (trailPositions.size() > maxTrailLength) {
                trailPositions.resize(maxTrailLength);
                trailAlphas.resize(maxTrailLength);
            }
            
            trailUpdateTime = 0.0f;
        }
        
        // 更新拖尾透明度衰减
        for (int i = 0; i < trailAlphas.size(); i++) {
            float trailFactor = 1.0f - (float(i) / maxTrailLength);
            trailAlphas[i] *= 0.98f; // 逐渐衰减
        }
    }
};

// Water ripple class - 优化涟漪渲染
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
    float pulseFrequency;
    float pulseAmplitude;
    float waveHeight;  // 新增：水面高度偏移
    
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
        pulseAmplitude(0.0f),
        waveHeight(0.0f) {
    }
    
    void init(const glm::vec3& _position, const glm::vec3& _color) {
        position = _position;
        position.y = WATER_HEIGHT + 0.02f; // 稍高于水面以确保可见
        color = _color;
        radius = 3.0f; // 更大的初始半径
        maxRadius = 80.0f + static_cast<float>(rand()) / RAND_MAX * 120.0f; // 超大涟漪
        thickness = 0.6f + static_cast<float>(rand()) / RAND_MAX * 1.2f; // 更厚的线条
        opacity = 1.0f; // 完全不透明开始
        growthRate = 15.0f + static_cast<float>(rand()) / RAND_MAX * 25.0f; // 超快扩散
        lifetime = 0.0f;
        maxLifetime = 6.0f + static_cast<float>(rand()) / RAND_MAX * 4.0f; // 更长寿命
        pulseFrequency = 3.0f + static_cast<float>(rand()) / RAND_MAX * 4.0f;
        pulseAmplitude = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        waveHeight = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
    }
    
    bool update(float deltaTime) {
        lifetime += deltaTime;
        
        float progress = lifetime / maxLifetime;
        float growthFactor = 1.0f - progress * 0.5f; // 更慢的减速
        radius += growthRate * deltaTime * growthFactor;
        
        // 动态厚度变化
        thickness = 0.3f + 0.4f * sinf(lifetime * pulseFrequency) * pulseAmplitude;
        
        // 改进的透明度衰减 - 更慢更自然
        opacity = 1.0f * (1.0f - powf(progress, 2.0f));
        
        // 波浪高度衰减
        waveHeight = (0.1f + 0.2f * sinf(lifetime * pulseFrequency * 1.2f)) * (1.0f - progress);
        
        return isDead();
    }
    
    bool isDead() const {
        return radius >= maxRadius || opacity <= 0.02f || lifetime >= maxLifetime;
    }
    
    float getCurrentThickness() const {
        return thickness;
    }
    
    float getCurrentWaveHeight() const {
        return waveHeight;
    }
};

// Application class
class RainSimulation {
public:
    // Window
    GLFWwindow* window;
    
    // Shaders
    std::unique_ptr<Shader> waterShader;
    std::unique_ptr<Shader> raindropShader;
    std::unique_ptr<Shader> rippleShader;
    std::unique_ptr<Shader> skyShader;    // New: sky shader
    std::unique_ptr<Shader> moonShader;   // New: moon shader
    std::unique_ptr<Shader> starShader;   // New: star shader
    std::unique_ptr<Shader> trailShader;  // New: raindrop trail shader
    std::unique_ptr<Shader> lightningShader; // New: lightning shader
    
    // Geometry
    unsigned int waterVAO, waterVBO;
    unsigned int raindropVAO, raindropVBO;
    unsigned int rippleVAO, rippleVBO;
    unsigned int skyVAO, skyVBO;          // New: sky
    unsigned int moonVAO, moonVBO;        // New: moon
    unsigned int starVAO, starVBO;        // New: stars
    unsigned int trailVAO, trailVBO;      // New: raindrop trail
    unsigned int lightningVAO, lightningVBO; // New: lightning
    unsigned int waterIndexCount;  // 水面索引数量
    unsigned int skyVertexCount;   // 天空顶点数量
    
    // Textures
    unsigned int waterNormalTexture;
    unsigned int waterDuDvTexture;
    unsigned int waterReflectionTexture;
    unsigned int raindropGlowTexture;
    unsigned int skyTexture;
    
    // Camera
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float cameraPitch;
    float cameraYaw;
    
    // Keyboard state tracking for smooth camera movement
    bool keys[1024] = {false};
    
    // Raindrops and water ripples
    std::vector<Raindrop> raindrops;
    std::vector<WaterRipple> ripples;
    
    // New: stars and clouds
    std::vector<Star> stars;
    std::vector<Cloud> clouds;
    
    // New: lightning system
    std::vector<Lightning> lightnings;
    float lightningTimer;
    float nextLightningTime;
    
    // Configuration
    struct {
        int rainDensity = 200;  // 增加雨滴密度
        float maxRippleSize = 60.0f; // 大幅增加最大涟漪大小
        float updateInterval = 0.008f; // 更频繁的更新
        float rippleFadeSpeed = 0.015f;
        std::vector<glm::vec3> raindropColors = {
            glm::vec3(0.9f, 0.2f, 1.0f), // 更亮的紫色
            glm::vec3(0.2f, 0.9f, 1.0f), // 更亮的青色
            glm::vec3(1.0f, 1.0f, 0.2f), // 更亮的黄色
            glm::vec3(1.0f, 0.5f, 0.1f), // 更亮的橙色
            glm::vec3(0.2f, 1.0f, 0.6f)  // 更亮的青绿色
        };
        // New: ripple colors
        std::vector<glm::vec3> rippleColors = {
            glm::vec3(0.6f, 0.8f, 1.0f), // Light blue
            glm::vec3(0.8f, 1.0f, 1.0f), // Light cyan
            glm::vec3(0.9f, 0.9f, 1.0f), // Light purple
            glm::vec3(0.7f, 0.9f, 1.0f), // Sky blue
            glm::vec3(0.6f, 0.9f, 0.9f)  // Teal gray
        };
        // Raindrop size range - 大幅增加雨滴大小
        float minRaindropSize = 0.8f;
        float maxRaindropSize = 2.5f;
        // Raindrop speed range
        float minRaindropSpeed = 2.0f;
        float maxRaindropSpeed = 6.0f;
        // Star twinkle speed
        float starTwinkleSpeed = 2.0f;
        // Ripple rings
        int rippleRings = 5; // 增加涟漪环数
        // Camera movement speed
        float cameraSpeed = 10.0f;
        // Water wave strength
        float waveStrength = 1.2f; // 增加波浪强度
        // Lightning settings
        float lightningFrequency = 8.0f; // 闪电频率（秒）
        float lightningIntensity = 1.0f; // 闪电强度
        bool lightningEnabled = true;
        // Ripple visibility
        float rippleVisibility = 2.0f; // 涟漪可见度增强
        // Show debug info
        bool showDebugInfo = true;
    } config;
    
    // SDL audio related members
    // Audio sounds
    Mix_Chunk* raindropSound;
    Mix_Music* ambientRainSound;
    Mix_Chunk* waterRippleSound;
    
    // Audio configuration
    struct {
        bool soundEnabled = true;
        float masterVolume = 0.8f;
        float raindropVolume = 0.5f;
        float ambientVolume = 0.3f;
        float rippleVolume = 0.4f;
    } audioConfig;
    
    // Time tracking
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    float rainAccumulator = 0.0f;
    float totalTime = 0.0f; // Total runtime
    
    // Performance metrics
    struct {
        float fps = 0.0f;
        float smoothedFps = 0.0f;
        float frameTimeMs = 0.0f;
        uint32_t totalFrames = 0;
        float fpsUpdateTime = 0.0f;
    } performanceMetrics;
    
    RainSimulation() : 
    window(nullptr),
    cameraPos(glm::vec3(0.0f, 60.0f, 120.0f)), // 进一步提高高度和距离以获得更好的全景视角
    cameraFront(glm::vec3(0.0f, -0.45f, -1.0f)), // 更大角度向下看以覆盖更大的池塘区域
    cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    cameraPitch(-25.0f), // 进一步增大俯仰角
    cameraYaw(-90.0f),
    raindropSound(nullptr),
    ambientRainSound(nullptr),
    waterRippleSound(nullptr),
    lightningTimer(0.0f),
    nextLightningTime(5.0f) {
    }
    
    ~RainSimulation() {
        // Release audio resources
        cleanup();

        // Release resources
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
        glDeleteVertexArrays(1, &lightningVAO);
        glDeleteBuffers(1, &lightningVBO);
        
        glDeleteTextures(1, &waterNormalTexture);
        glDeleteTextures(1, &waterDuDvTexture);
        glDeleteTextures(1, &waterReflectionTexture);
        glDeleteTextures(1, &raindropGlowTexture);
        glDeleteTextures(1, &skyTexture);
        
        // ImGui cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        // Close GLFW
        glfwTerminate();
    }
    
    // Cleanup function, called in destructor
    void cleanup() {
        // Release audio resources
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
        
        // Close SDL_mixer
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
    }
    
    // Function to initialize the audio subsystem
    bool initAudio() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Failed to initialize SDL_AUDIO: " << SDL_GetError() << std::endl;
            audioConfig.soundEnabled = false;
            return false;
        }
        
        // Initialize SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "Failed to initialize SDL_Mixer: " << Mix_GetError() << std::endl;
            audioConfig.soundEnabled = false;
            return false;
        }
        
        // Allocate more channels for simultaneous sound playback
        Mix_AllocateChannels(32);
        
        // Set master volume
        Mix_Volume(-1, static_cast<int>(audioConfig.masterVolume * MIX_MAX_VOLUME));
        
        // Ensure audio directory exists
        ensureAudioFilesExist();
        
        // Load audio files
        raindropSound = Mix_LoadWAV("audio/raindrop_splash.wav");
        if (!raindropSound) {
            std::cerr << "Failed to load raindrop sound effect: " << Mix_GetError() << std::endl;
            
            // Try alternate path
            raindropSound = Mix_LoadWAV("../audio/raindrop_splash.wav");
            if (raindropSound) {
                std::cout << "Successfully loaded raindrop sound from project root" << std::endl;
            }
        }
        
        ambientRainSound = Mix_LoadMUS("audio/ambient_rain.mp3");
        if (!ambientRainSound) {
            std::cerr << "Failed to load ambient rain sound: " << Mix_GetError() << std::endl;
            
            // Try alternate path
            ambientRainSound = Mix_LoadMUS("../audio/ambient_rain.mp3");
            if (ambientRainSound) {
                std::cout << "Successfully loaded ambient rain from project root" << std::endl;
            }
        }
        
        waterRippleSound = Mix_LoadWAV("audio/water_ripple.wav");
        if (!waterRippleSound) {
            std::cerr << "Failed to load water ripple sound effect: " << Mix_GetError() << std::endl;
            
            // Try alternate path
            waterRippleSound = Mix_LoadWAV("../audio/water_ripple.wav");
            if (waterRippleSound) {
                std::cout << "Successfully loaded water ripple sound from project root" << std::endl;
            }
        }
        
        // Set volume for each sound effect
        if (raindropSound) {
            Mix_VolumeChunk(raindropSound, static_cast<int>(audioConfig.raindropVolume * MIX_MAX_VOLUME));
        }
        
        if (waterRippleSound) {
            Mix_VolumeChunk(waterRippleSound, static_cast<int>(audioConfig.rippleVolume * MIX_MAX_VOLUME));
        }
        
        // Play background rain sound (loop)
        if (ambientRainSound) {
            Mix_VolumeMusic(static_cast<int>(audioConfig.ambientVolume * MIX_MAX_VOLUME));
            Mix_PlayMusic(ambientRainSound, -1); // -1 means infinite loop
        }
        
        return true;
    }

    // Function to play raindrop sound
    void playRaindropSound(const glm::vec3& position) {
        if (!audioConfig.soundEnabled || !raindropSound)
            return;
            
        // Calculate volume based on distance from camera
        float distance = glm::length(position - cameraPos);
        float volumeScale = 1.0f - std::min(distance / 50.0f, 0.95f); // 50.0f is maximum audible distance
        
        // Randomize volume and pitch for variety
        float volumeVariation = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        int volume = static_cast<int>(audioConfig.raindropVolume * MIX_MAX_VOLUME * volumeScale * volumeVariation);
        
        // Find an available channel to play
        int channel = Mix_PlayChannel(-1, raindropSound, 0);
        if (channel != -1) {
            // Set channel volume
            Mix_Volume(channel, volume);
        }
    }

    // Function to play water ripple sound
    void playRippleSound(const glm::vec3& position) {
        if (!audioConfig.soundEnabled || !waterRippleSound)
            return;
            
        // Calculate volume based on distance from camera
        float distance = glm::length(position - cameraPos);
        float volumeScale = 1.0f - std::min(distance / 50.0f, 0.95f); // 50.0f is maximum audible distance
        
        // Randomize volume for variety
        float volumeVariation = 0.7f + static_cast<float>(rand()) / RAND_MAX * 0.6f;
        int volume = static_cast<int>(audioConfig.rippleVolume * MIX_MAX_VOLUME * volumeScale * volumeVariation * 0.5f);
        
        // Find an available channel to play
        int channel = Mix_PlayChannel(-1, waterRippleSound, 0);
        if (channel != -1) {
            // Set channel volume
            Mix_Volume(channel, volume);
        }
    }

    // Update audio settings
    void updateAudioSettings() {
        if (!audioConfig.soundEnabled)
            return;
            
        // Update master volume
        Mix_Volume(-1, static_cast<int>(audioConfig.masterVolume * MIX_MAX_VOLUME));
        
        // Update sound effect volumes
        if (raindropSound) {
            Mix_VolumeChunk(raindropSound, static_cast<int>(audioConfig.raindropVolume * MIX_MAX_VOLUME));
        }
        
        if (waterRippleSound) {
            Mix_VolumeChunk(waterRippleSound, static_cast<int>(audioConfig.rippleVolume * MIX_MAX_VOLUME));
        }
        
        // Update music volume
        Mix_VolumeMusic(static_cast<int>(audioConfig.ambientVolume * MIX_MAX_VOLUME));
    }
    bool init() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Set OpenGL version
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for Mac OS X
#endif
        
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);  // For better error reporting
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);            // Disable window resizing
        glfwWindowHint(GLFW_VISIBLE, GL_TRUE);               // Make window visible
        glfwWindowHint(GLFW_FOCUSED, GL_TRUE);               // Give focus to the window

        // Create window
        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Colorful Rain Simulation", NULL, NULL);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        });
        
        // Set key callback to track key states
        glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            RainSimulation* app = static_cast<RainSimulation*>(glfwGetWindowUserPointer(window));
            if (key >= 0 && key < 1024) {
                if (action == GLFW_PRESS)
                    app->keys[key] = true;
                else if (action == GLFW_RELEASE)
                    app->keys[key] = false;
            }
        });
        
        // Store this instance as user pointer for callbacks
        glfwSetWindowUserPointer(window, this);
        
        // Initialize GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            return false;
        }
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        
        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Enable point size
        glEnable(GL_PROGRAM_POINT_SIZE);
        
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        
        // Set ImGui style
        ImGui::StyleColorsDark();
        
        // Initialize ImGui GLFW and OpenGL parts
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Initialize audio
        initAudio();
        
        // Load shaders
        loadShaders();
        
        // Create geometry
        createGeometry();
        
        // Load textures
        loadTextures();
        
        // Initialize stars
        initStars();
        
        // Initialize clouds
        initClouds();
        
        return true;
    }
    
    // Initialize stars
    void initStars() {
        stars.clear();
        
        for (int i = 0; i < STARS_COUNT; i++) {
            Star star;
            
            // Random position - in sky dome
            float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * glm::pi<float>();
            float phi = static_cast<float>(rand()) / RAND_MAX * glm::pi<float>() * 0.5f; // Upper hemisphere
            
            float radius = 200.0f + static_cast<float>(rand()) / RAND_MAX * 50.0f;
            star.position.x = radius * sin(phi) * cos(theta);
            star.position.y = radius * cos(phi) + 20.0f; // Offset upward
            star.position.z = radius * sin(phi) * sin(theta);
            
            // Random brightness and twinkle speed
            star.brightness = 0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f;
            star.twinkleSpeed = 0.5f + static_cast<float>(rand()) / RAND_MAX * 5.0f;
            star.size = 0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f;
            
            stars.push_back(star);
        }
    }
    
    // Initialize clouds
    void initClouds() {
        clouds.clear();
        
        for (int i = 0; i < CLOUD_COUNT; i++) {
            Cloud cloud;
            
            // Random position - in sky
            cloud.position.x = -100.0f + static_cast<float>(rand()) / RAND_MAX * 200.0f;
            cloud.position.y = 40.0f + static_cast<float>(rand()) / RAND_MAX * 30.0f;
            cloud.position.z = -100.0f + static_cast<float>(rand()) / RAND_MAX * 100.0f;
            
            // Random size, opacity and speed
            cloud.size = 10.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f;
            cloud.opacity = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
            cloud.speed = 0.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
            
            clouds.push_back(cloud);
        }
    }
    
    void loadShaders() {
        // Try both paths to find shader files
        const char* waterVertPath = "shaders/water.vert";
        const char* waterFragPath = "shaders/water.frag";
        const char* raindropVertPath = "shaders/raindrop.vert";
        const char* raindropFragPath = "shaders/raindrop.frag";
        const char* rippleVertPath = "shaders/ripple.vert";
        const char* rippleFragPath = "shaders/ripple.frag";
        
        // Check if shader files exist, otherwise use root path
        if (!file_exists(waterVertPath)) {
            std::cout << "Shader files not found in build directory, checking project root..." << std::endl;
            
            // Check if they exist in project root
            if (file_exists("../shaders/water.vert")) {
                waterVertPath = "../shaders/water.vert";
                waterFragPath = "../shaders/water.frag";
                raindropVertPath = "../shaders/raindrop.vert";
                raindropFragPath = "../shaders/raindrop.frag";
                rippleVertPath = "../shaders/ripple.vert";
                rippleFragPath = "../shaders/ripple.frag";
                
                std::cout << "Found shader files in project root directory" << std::endl;
            } else {
                std::cout << "Shader files not found, will write and use default shaders" << std::endl;
                writeShaderFiles();
            }
        }
        
        try {
            skyShader = std::make_unique<Shader>("shaders/sky.vert", "shaders/sky.frag");

            // Load water shader
            waterShader = std::make_unique<Shader>(waterVertPath, waterFragPath);
            
            // Load raindrop shader
            raindropShader = std::make_unique<Shader>(raindropVertPath, raindropFragPath);
            
            // Load ripple shader
            rippleShader = std::make_unique<Shader>(rippleVertPath, rippleFragPath);
            
            // Reuse shaders for other elements
            moonShader = std::make_unique<Shader>(raindropVertPath, raindropFragPath);
            starShader = std::make_unique<Shader>(raindropVertPath, raindropFragPath);
            trailShader = std::make_unique<Shader>(rippleVertPath, rippleFragPath);
            lightningShader = std::make_unique<Shader>("shaders/lightning.vert", "shaders/lightning.frag");
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading shaders: " << e.what() << std::endl;
            std::cout << "Writing and using default shaders instead" << std::endl;
            
            writeShaderFiles();
            
            // Load shaders again after writing defaults
            waterShader = std::make_unique<Shader>("shaders/water.vert", "shaders/water.frag");
            raindropShader = std::make_unique<Shader>("shaders/raindrop.vert", "shaders/raindrop.frag");
            rippleShader = std::make_unique<Shader>("shaders/ripple.vert", "shaders/ripple.frag");
            skyShader = std::make_unique<Shader>("shaders/water.vert", "shaders/water.frag");
            moonShader = std::make_unique<Shader>("shaders/raindrop.vert", "shaders/raindrop.frag");
            starShader = std::make_unique<Shader>("shaders/raindrop.vert", "shaders/raindrop.frag");
            trailShader = std::make_unique<Shader>("shaders/ripple.vert", "shaders/ripple.frag");
            lightningShader = std::make_unique<Shader>("shaders/lightning.vert", "shaders/lightning.frag");
        }
    }
    
    void createGeometry() {
        // 创建水面平面 - 使用更多顶点以支持更大的水面和更好的波浪效果
        std::vector<float> waterVertices;
        const int gridSize = 64;  // 大幅增加网格密度以减少锯齿
        const float cellSize = POND_SIZE / gridSize;
        
        for (int z = 0; z <= gridSize; z++) {
            for (int x = 0; x <= gridSize; x++) {
                float xPos = -POND_SIZE/2 + x * cellSize;
                float zPos = -POND_SIZE/2 + z * cellSize;
                float texU = (float)x / gridSize;
                float texV = (float)z / gridSize;
                
                waterVertices.push_back(xPos);
                waterVertices.push_back(0.0f);
                waterVertices.push_back(zPos);
                waterVertices.push_back(texU);
                waterVertices.push_back(texV);
            }
        }
        
        std::vector<unsigned int> waterIndices;
        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                unsigned int topLeft = z * (gridSize + 1) + x;
                unsigned int topRight = topLeft + 1;
                unsigned int bottomLeft = (z + 1) * (gridSize + 1) + x;
                unsigned int bottomRight = bottomLeft + 1;
                
                // 第一个三角形
                waterIndices.push_back(topLeft);
                waterIndices.push_back(bottomLeft);
                waterIndices.push_back(topRight);
                
                // 第二个三角形
                waterIndices.push_back(topRight);
                waterIndices.push_back(bottomLeft);
                waterIndices.push_back(bottomRight);
            }
        }
        
        unsigned int waterEBO;
        glGenVertexArrays(1, &waterVAO);
        glGenBuffers(1, &waterVBO);
        glGenBuffers(1, &waterEBO);
        
        glBindVertexArray(waterVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
        glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(float), waterVertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, waterIndices.size() * sizeof(unsigned int), waterIndices.data(), GL_STATIC_DRAW);
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // 纹理坐标属性
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // 保存索引数量供渲染时使用
        waterIndexCount = waterIndices.size();
        
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
        const int segments = 256; // 大幅增加细节以减少锯齿
        
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
        
        // 创建天空穹顶 - 使用更大的半径以避免裁剪
        std::vector<float> skyVertices;
        const int skySegments = 32;
        const float skyRadius = 500.0f;  // 增大天空半径以匹配水面大小
        
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
                
                // 纹理坐标 - 翻转Y坐标使纹理正确显示
                glm::vec2 t1(float(x) / float(skySegments), 1.0f - float(y) / float(skySegments / 2));
                glm::vec2 t2(float(x + 1) / float(skySegments), 1.0f - float(y) / float(skySegments / 2));
                glm::vec2 t3(float(x + 1) / float(skySegments), 1.0f - float(y + 1) / float(skySegments / 2));
                glm::vec2 t4(float(x) / float(skySegments), 1.0f - float(y + 1) / float(skySegments / 2));
                
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
        
        // 天空顶点数量 - 保存供渲染时使用
        skyVertexCount = skyVertices.size() / 5;
        
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
        
        // 创建闪电线条几何体 - 简单线条用于闪电渲染
        float lightningVertices[] = {
            0.0f, 0.0f, 0.0f,  // 起点
            1.0f, 1.0f, 1.0f   // 终点（会在渲染时动态更新）
        };
        
        glGenVertexArrays(1, &lightningVAO);
        glGenBuffers(1, &lightningVBO);
        
        glBindVertexArray(lightningVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lightningVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lightningVertices), lightningVertices, GL_DYNAMIC_DRAW); // 使用动态绘制
        
        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    // Modified loadTextures function
    void loadTextures() {
        // Ensure textures exist
        ensureTexturesExist();
        
        // Try to load textures from build directory first
        waterNormalTexture = loadTexture("textures/waternormal.jpeg");
        waterDuDvTexture = loadTexture("textures/waterDuDv.jpg");
        waterReflectionTexture = loadTexture("textures/waterReflection.jpg");
        raindropGlowTexture = loadTexture("textures/raindrop_glow.png");
        skyTexture = loadTexture("textures/night_sky.jpg");
    }
    
    unsigned int loadTexture(const char* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        
        // Bind texture, all subsequent GL_TEXTURE_2D operations will affect this texture
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Set texture wrapping and filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Load and generate texture
        int width, height, nrChannels;
        
        // Flip image as OpenGL expects bottom-left origin
        stbi_set_flip_vertically_on_load(true);
        
        // Try to load image
        unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
        
        // If loading failed, try alternate path
        if (!data) {
            std::string projectRootPath = std::string("../") + path;
            data = stbi_load(projectRootPath.c_str(), &width, &height, &nrChannels, 0);
            
            if (data) {
                std::cout << "Successfully loaded texture from project root: " << projectRootPath << std::endl;
            }
        }
        
        if (data) {
            GLenum format;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;
            else {
                std::cerr << "Unknown image format: " << path << " (channels: " << nrChannels << ")" << std::endl;
                format = GL_RGB; // Default to RGB
            }
            
            // Generate texture
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            std::cout << "Successfully loaded texture: " << path << " (" << width << "x" << height << ", " 
                      << nrChannels << " channels)" << std::endl;
        } else {
            std::cerr << "Texture loading failed: " << path << std::endl;
            
            // If texture loading failed, create a default texture
            // Create an 8x8 checkerboard texture as a fallback
            unsigned char defaultTextureData[8*8*4];
            
            if (strstr(path, "normal")) {
                // Default value for normal map: random normals
                for (int i = 0; i < 8*8; i++) {
                    // Random normal - mapped to RGB values
                    float nx = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.2f;
                    float ny = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
                    float nz = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.2f;
                    
                    float length = sqrt(nx*nx + ny*ny + nz*nz);
                    nx /= length;
                    ny /= length;
                    nz /= length;
                    
                    // Convert to RGB (0-255)
                    defaultTextureData[i*4+0] = static_cast<unsigned char>((nx * 0.5f + 0.5f) * 255); // R
                    defaultTextureData[i*4+1] = static_cast<unsigned char>((ny * 0.5f + 0.5f) * 255); // G
                    defaultTextureData[i*4+2] = static_cast<unsigned char>((nz * 0.5f + 0.5f) * 255); // B
                    defaultTextureData[i*4+3] = 255; // A
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "DuDv")) {
                // Default value for DuDv map: random distortions
                for (int i = 0; i < 8*8; i++) {
                    // Random distortion values
                    defaultTextureData[i*4+0] = 128 + rand() % 40 - 20; // R
                    defaultTextureData[i*4+1] = 128 + rand() % 40 - 20; // G
                    defaultTextureData[i*4+2] = 128; // B
                    defaultTextureData[i*4+3] = 255; // A
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "Reflection")) {
                // Default value for reflection map: generate random night sky reflection texture
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int i = y*8 + x;
                        
                        // Base deep blue tone
                        defaultTextureData[i*4+0] = 10 + rand() % 20;  // R
                        defaultTextureData[i*4+1] = 20 + rand() % 30;  // G
                        defaultTextureData[i*4+2] = 40 + rand() % 50;  // B
                        
                        // Occasionally add star reflections
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
                // Glow map: circular gradient
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int i = y*8 + x;
                        
                        // Calculate distance to center
                        float dx = (x - 3.5f) / 3.5f;
                        float dy = (y - 3.5f) / 3.5f;
                        float dist = sqrt(dx*dx + dy*dy);
                        
                        // Circular gradient brightness
                        float brightness = 1.0f - std::min(dist, 1.0f);
                        brightness = brightness * brightness; // Square to make edges softer
                        
                        defaultTextureData[i*4+0] = static_cast<unsigned char>(brightness * 255); // R
                        defaultTextureData[i*4+1] = static_cast<unsigned char>(brightness * 255); // G
                        defaultTextureData[i*4+2] = static_cast<unsigned char>(brightness * 255); // B
                        defaultTextureData[i*4+3] = static_cast<unsigned char>(brightness * 255); // A
                    }
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTextureData);
            } else if (strstr(path, "sky")) {
                // Sky map: gradient night sky
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int i = y*8 + x;
                        
                        // Gradient from bottom to top
                        float gradient = static_cast<float>(y) / 7.0f;
                        
                        // Deep blue to black gradient
                        defaultTextureData[i*4+0] = static_cast<unsigned char>(5 + (1.0f - gradient) * 15); // R
                        defaultTextureData[i*4+1] = static_cast<unsigned char>(10 + (1.0f - gradient) * 20); // G
                        defaultTextureData[i*4+2] = static_cast<unsigned char>(30 + (1.0f - gradient) * 50); // B
                        
                        // Randomly add stars
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
                // Default: checkerboard texture
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
            std::cout << "Generated default texture as fallback" << std::endl;
        }
        
        // Free image data
        stbi_image_free(data);
        
        return textureID;
    }
    
    // Function to create texture folder and ensure textures exist
    bool ensureTexturesExist() {
        // Check if texture directory exists, create if it doesn't
        if (!file_exists("textures")) {
            create_directory("textures");
            std::cout << "Created textures directory" << std::endl;
        }
        
        // Check if each required texture file exists
        std::vector<std::string> requiredTextures = {
            "textures/waternormal.jpeg",
            "textures/waterDuDv.jpg",
            "textures/waterReflection.jpg",
            "textures/raindrop_glow.png",
            "textures/night_sky.jpg"
        };
        
        bool allTexturesExist = true;
        
        for (const auto& texture : requiredTextures) {
            // First check in build directory
            bool exists = file_exists(texture);
            
            // If not in build directory, check in project root
            if (!exists) {
                std::string projectRootPath = std::string("../") + texture;
                exists = file_exists(projectRootPath);
                
                if (exists) {
                    std::cout << "Found texture in project root: " << projectRootPath << std::endl;
                }
            }
            
            if (!exists) {
                std::cerr << "Warning: Texture file not found: " << texture << std::endl;
                allTexturesExist = false;
                
                // Create a simple default texture file
                std::cout << "Generating default texture file: " << texture << std::endl;
                generateDefaultTexture(texture);
            }
        }
        
        return allTexturesExist;
    }

    // Generate default texture file
    void generateDefaultTexture(const std::string& path) {
        // Ensure directory exists
        std::string parent = parent_path(path);
        if (!parent.empty() && !file_exists(parent)) {
            create_directories(parent);
        }
        
        // Create an improved 8x8 texture
        const int width = 8;
        const int height = 8;
        const int channels = 4; // RGBA
        unsigned char data[width * height * channels];
        
        // Fill data - use different patterns based on texture type
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int i = (y * width + x) * channels;
                
                if (path.find("normal") != std::string::npos) {
                    // Normal map: random bumps
                    data[i+0] = 128 + (rand() % 40 - 20); // R
                    data[i+1] = 128 + (rand() % 40 - 20); // G
                    data[i+2] = 200 + (rand() % 55); // B - mainly upward
                    data[i+3] = 255; // A
                } else if (path.find("DuDv") != std::string::npos) {
                    // DuDv map: random distortions
                    data[i+0] = 128 + (rand() % 30 - 15); // R
                    data[i+1] = 128 + (rand() % 30 - 15); // G
                    data[i+2] = 128; // B
                    data[i+3] = 255; // A
                } else if (path.find("Reflection") != std::string::npos) {
                    // Reflection map: night sky star effect
                    data[i+0] = 10 + (rand() % 20); // R
                    data[i+1] = 20 + (rand() % 30); // G
                    data[i+2] = 50 + (rand() % 40); // B
                    
                    // Random sprinkle of stars
                    if (rand() % 20 == 0) {
                        data[i+0] = 200 + (rand() % 55); // R
                        data[i+1] = 200 + (rand() % 55); // G
                        data[i+2] = 200 + (rand() % 55); // B
                    }
                    data[i+3] = 255; // A
                } else if (path.find("glow") != std::string::npos) {
                    // Glow map: circular gradient
                    float dx = (x - width/2.0f) / (width/2.0f);
                    float dy = (y - height/2.0f) / (height/2.0f);
                    float dist = sqrt(dx*dx + dy*dy);
                    float intensity = std::max(0.0f, 1.0f - dist);
                    intensity = intensity * intensity; // Square to make edges softer
                    
                    data[i+0] = static_cast<unsigned char>(255 * intensity); // R
                    data[i+1] = static_cast<unsigned char>(255 * intensity); // G
                    data[i+2] = static_cast<unsigned char>(255 * intensity); // B
                    data[i+3] = static_cast<unsigned char>(255 * intensity); // A
                } else if (path.find("sky") != std::string::npos) {
                    // Sky map: night sky gradient
                    float gradient = static_cast<float>(y) / height;
                    
                    data[i+0] = static_cast<unsigned char>(5 + (1.0f - gradient) * 15); // R
                    data[i+1] = static_cast<unsigned char>(10 + (1.0f - gradient) * 20); // G
                    data[i+2] = static_cast<unsigned char>(30 + (1.0f - gradient) * 70); // B
                    
                    // Random stars
                    if (rand() % 20 == 0) {
                        data[i+0] = 200 + (rand() % 55); // R
                        data[i+1] = 200 + (rand() % 55); // G
                        data[i+2] = 200 + (rand() % 55); // B
                    }
                    
                    data[i+3] = 255; // A
                } else {
                    // Default: blue gradient
                    float t = static_cast<float>(y) / height;
                    data[i+0] = static_cast<unsigned char>(50 * (1.0f-t)); // R
                    data[i+1] = static_cast<unsigned char>(80 * (1.0f-t) + 20); // G
                    data[i+2] = static_cast<unsigned char>(120 * (1.0f-t) + 80); // B
                    data[i+3] = 255; // A
                }
            }
        }
        
        // Write to file
        if (path.find(".png") != std::string::npos) {
            stbi_write_png(path.c_str(), width, height, channels, data, width * channels);
        } else {
            stbi_write_jpg(path.c_str(), width, height, channels, data, 95); // 95 is quality setting (0-100)
        }
    }

    // Ensure audio files exist
    bool ensureAudioFilesExist() {
        // Check if audio directory exists, create if it doesn't
        if (!file_exists("audio")) {
            create_directory("audio");
            std::cout << "Created audio directory" << std::endl;
        }
        
        // Check if each required audio file exists
        std::vector<std::string> requiredAudioFiles = {
            "audio/raindrop_splash.wav",
            "audio/ambient_rain.mp3",
            "audio/water_ripple.wav"
        };
        
        bool allAudioFilesExist = true;
        
        for (const auto& audioFile : requiredAudioFiles) {
            // First check in build directory
            bool exists = file_exists(audioFile);
            
            // If not in build directory, check in project root
            if (!exists) {
                std::string projectRootPath = std::string("../") + audioFile;
                exists = file_exists(projectRootPath);
                
                if (exists) {
                    std::cout << "Found audio file in project root: " << projectRootPath << std::endl;
                }
            }
            
            if (!exists) {
                std::cerr << "Warning: Audio file not found: " << audioFile << std::endl;
                allAudioFilesExist = false;
                
                // Create placeholder audio file
                generatePlaceholderAudioFile(audioFile);
            }
        }
        
        return allAudioFilesExist;
    }

    // Generate placeholder audio file
    void generatePlaceholderAudioFile(const std::string& path) {
        // Ensure directory exists
        std::string parent = parent_path(path);
        if (!parent.empty() && !file_exists(parent)) {
            create_directories(parent);
        }
        
        // Create a simple text file indicating missing audio file
        std::ofstream placeholderFile(path + ".placeholder.txt");
        placeholderFile << "This is a placeholder for " << path << ". Please download or create the actual audio file.";
        placeholderFile.close();
        
        // Note: Creating actual audio files would require complex audio encoding libraries
        std::cout << "Created placeholder for missing audio file: " << path << ".placeholder.txt" << std::endl;
        std::cout << "Please place actual audio files in the corresponding location to enable sound" << std::endl;
    }
    
    void run() {
        // 检查OpenGL错误
        GLenum err;
        while((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "OpenGL startup error: " << err << std::endl;
        }

        // Main application loop
        while (!glfwWindowShouldClose(window)) {
            // Handle time
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            totalTime += deltaTime;
            
            // Update performance metrics
            performanceMetrics.totalFrames++;
            performanceMetrics.frameTimeMs = deltaTime * 1000.0f;
            performanceMetrics.fps = 1.0f / deltaTime;
            
            // Smooth FPS calculation (exponential moving average)
            if (performanceMetrics.smoothedFps == 0.0f) {
                performanceMetrics.smoothedFps = performanceMetrics.fps;
            } else {
                performanceMetrics.smoothedFps = performanceMetrics.smoothedFps * 0.95f + performanceMetrics.fps * 0.05f;
            }
            
            // Process input
            processInput();
            
            // Update
            update();
            
            // Render
            render();
            
            // Render UI
            renderUI();
            
            // Swap buffers and poll IO events
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    
    void processInput() {
        if (keys[GLFW_KEY_ESCAPE])
            glfwSetWindowShouldClose(window, true);
            
        // Camera movement with smooth transitions
        float cameraSpeed = config.cameraSpeed * deltaTime;
        
        // Forward/backward
        if (keys[GLFW_KEY_W])
            cameraPos += cameraSpeed * cameraFront;
        if (keys[GLFW_KEY_S])
            cameraPos -= cameraSpeed * cameraFront;
        
        // Left/right
        if (keys[GLFW_KEY_A])
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (keys[GLFW_KEY_D])
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        
        // Up/down - Changed to Space and Left Control
        if (keys[GLFW_KEY_SPACE])
            cameraPos += cameraUp * cameraSpeed;
        if (keys[GLFW_KEY_LEFT_CONTROL])
            cameraPos -= cameraUp * cameraSpeed;
            
        // Camera rotation - arrow keys
        float rotateSpeed = 30.0f * deltaTime;
        if (keys[GLFW_KEY_UP])
            cameraPitch += rotateSpeed;
        if (keys[GLFW_KEY_DOWN])
            cameraPitch -= rotateSpeed;
        if (keys[GLFW_KEY_LEFT])
            cameraYaw -= rotateSpeed;
        if (keys[GLFW_KEY_RIGHT])
            cameraYaw += rotateSpeed;
            
        // Constrain camera pitch
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
            
        // Update camera front vector
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        cameraFront = glm::normalize(front);
    }
    
    void update() {
        // Generate new raindrops
        rainAccumulator += deltaTime;
        if (rainAccumulator >= config.updateInterval) {
            rainAccumulator = 0.0f;
            generateRaindrops();
        }
        
        // Update raindrops
        for (auto it = raindrops.begin(); it != raindrops.end();) {
            bool createRipple = it->update(deltaTime);
            
            if (createRipple) {
                // Create water ripple
                WaterRipple ripple;
                ripple.init(it->position, it->color);
                ripples.push_back(ripple);
                
                // Play water ripple sound (probabilistic, not for every ripple)
                if (rand() % 100 < 30) { // 30% chance to play ripple sound
                    playRippleSound(ripple.position);
                }
            }
            
            if (it->isDead()) {
                it = raindrops.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update water ripples
        for (auto it = ripples.begin(); it != ripples.end();) {
            if (it->update(deltaTime)) {
                it = ripples.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update star twinkling
        for (auto& star : stars) {
            star.brightness = 0.5f + 0.5f * sin(totalTime * star.twinkleSpeed);
        }
        
        // Update cloud positions
        for (auto& cloud : clouds) {
            cloud.position.x += cloud.speed * deltaTime;
            
            // If cloud moves out of view, reposition on the other side
            if (cloud.position.x > POND_SIZE) {
                cloud.position.x = -POND_SIZE;
                cloud.position.z = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                cloud.opacity = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
            }
        }
        
        // Update lightning system
        if (config.lightningEnabled) {
            lightningTimer += deltaTime;
            
            // Generate new lightning
            if (lightningTimer >= nextLightningTime) {
                generateLightning();
                lightningTimer = 0.0f;
                // Random interval for next lightning
                nextLightningTime = config.lightningFrequency + (static_cast<float>(rand()) / RAND_MAX) * config.lightningFrequency;
            }
            
            // Update existing lightning
            for (auto it = lightnings.begin(); it != lightnings.end();) {
                if (!it->update(deltaTime)) {
                    it = lightnings.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    void generateRaindrops() {
        int raindropsToGenerate = config.rainDensity / 4; // 增加生成数量
        
        for (int i = 0; i < raindropsToGenerate; ++i) {
            if (rand() % 100 < 80) { // 大幅提高生成概率到80%
                Raindrop raindrop;
                
                // 改进的位置生成策略 - 创造更好的层次感
                float cameraDistance = glm::length(cameraPos);
                float nearRadius = cameraDistance * 0.3f;   // 近距离范围
                float farRadius = cameraDistance * 1.5f;    // 远距离范围
                
                // 随机选择距离层次
                float layerChoice = static_cast<float>(rand()) / RAND_MAX;
                float radius, height;
                
                if (layerChoice < 0.4f) {
                    // 40% 概率生成近距离大雨滴
                    radius = nearRadius;
                    height = 15.0f + static_cast<float>(rand()) / RAND_MAX * 25.0f;
                } else if (layerChoice < 0.7f) {
                    // 30% 概率生成中距离雨滴
                    radius = (nearRadius + farRadius) * 0.5f;
                    height = 25.0f + static_cast<float>(rand()) / RAND_MAX * 35.0f;
                } else {
                    // 30% 概率生成远距离小雨滴
                    radius = farRadius;
                    height = 35.0f + static_cast<float>(rand()) / RAND_MAX * 50.0f;
                }
                
                // 在圆形区域内随机生成位置
                float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * glm::pi<float>();
                float distance = static_cast<float>(rand()) / RAND_MAX * radius;
                
                float x = cameraPos.x + distance * cos(angle);
                float z = cameraPos.z + distance * sin(angle);
                float y = cameraPos.y + height;
                
                // 随机颜色
                int colorIndex = rand() % config.raindropColors.size();
                
                raindrop.init(glm::vec3(x, y, z), config.raindropColors[colorIndex], this);
                raindrops.push_back(raindrop);
            }
        }
    }
    
    // 新增：生成闪电
    void generateLightning() {
        Lightning lightning;
        
        // 随机闪电起点（天空中的位置）
        glm::vec3 startPos(
            cameraPos.x + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 400.0f,
            cameraPos.y + 100.0f + static_cast<float>(rand()) / RAND_MAX * 100.0f,
            cameraPos.z + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 400.0f
        );
        
        // 随机闪电终点（地面或水面附近）
        glm::vec3 endPos(
            startPos.x + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 100.0f,
            WATER_HEIGHT + 5.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f,
            startPos.z + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 100.0f
        );
        
        lightning.generate(startPos, endPos);
        lightnings.push_back(lightning);
    }
    
    void render() {
        // 清空缓冲
        glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 视图/投影矩阵
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        
        // 顺序很重要：先天空、再月亮和星星、然后水面、最后雨滴、波纹和闪电
        renderSky(view, projection);
        renderMoon(view, projection);
        renderStars(view, projection);
        renderWater(view, projection);
        renderRaindrops(view, projection);
        renderRipples(view, projection);
        renderLightning(view, projection);
        
        // 渲染ImGui界面
        renderUI();

        // 检查渲染错误
        GLenum err;
        while((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "Render error: " << err << std::endl;
        }
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
        
        // 设置水面属性 - 优化波浪效果
        waterShader->setFloat("time", totalTime);
        waterShader->setVec3("viewPos", cameraPos);
        waterShader->setFloat("waveStrength", config.waveStrength * 3.0f); // 适度增强波浪，避免过于夸张
        waterShader->setFloat("waveSpeed", 1.8f); // 适当加快波浪速度
        waterShader->setFloat("waterDepth", 0.9f); // 进一步加深水色
        
        // 使用索引绘制水面
        glBindVertexArray(waterVAO);
        glDrawElements(GL_TRIANGLES, waterIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void renderRaindrops(const glm::mat4& view, const glm::mat4& projection) {
        // 首先渲染流星拖尾效果 - 增强视觉效果
        trailShader->use();
        trailShader->setMat4("view", view);
        trailShader->setMat4("projection", projection);
        
        // 启用线条宽度设置
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        
        glBindVertexArray(lightningVAO); // 重用闪电的线条VAO
        
        for (const auto& raindrop : raindrops) {
            if (!raindrop.visible || raindrop.state > 0 || raindrop.trailPositions.empty())
                continue;
                
            // 渲染每条雨滴的拖尾
            for (int i = 0; i < raindrop.trailPositions.size() - 1; i++) {
                // 计算拖尾衰减
                float trailFactor = 1.0f - (float(i) / raindrop.maxTrailLength);
                float alpha = raindrop.trailAlphas[i] * trailFactor * 0.8f;
                
                if (alpha < 0.05f) continue; // 跳过过于透明的部分
                
                // 更新线条顶点数据
                std::vector<float> lineVertices = {
                    raindrop.trailPositions[i].x, raindrop.trailPositions[i].y, raindrop.trailPositions[i].z,
                    raindrop.trailPositions[i+1].x, raindrop.trailPositions[i+1].y, raindrop.trailPositions[i+1].z
                };
                
                glBindBuffer(GL_ARRAY_BUFFER, lightningVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
                
                // 设置模型矩阵
                glm::mat4 model = glm::mat4(1.0f);
                trailShader->setMat4("model", model);
                
                // 设置拖尾颜色和透明度 - 增强流星效果
                glm::vec3 trailColor = raindrop.color * (1.2f + 0.3f * sin(totalTime * 5.0f));
                trailShader->setVec3("rippleColor", trailColor);
                trailShader->setFloat("opacity", alpha);
                
                // 动态线条宽度
                float lineWidth = raindrop.size * (2.0f - raindrop.layerDepth) * trailFactor * 2.0f;
                glLineWidth(std::max(lineWidth, 1.0f));
                
                // 绘制拖尾线段
                glDrawArrays(GL_LINES, 0, 2);
            }
        }
        
        glDisable(GL_LINE_SMOOTH);
        
        // 然后渲染雨滴主体 - 改进的点渲染
        raindropShader->use();
        raindropShader->setMat4("view", view);
        raindropShader->setMat4("projection", projection);
        
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_POINT_SMOOTH); // 启用点的抗锯齿
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        
        glBindVertexArray(raindropVAO);
        
        // 按距离排序雨滴以实现正确的透明度混合
        std::vector<std::pair<float, const Raindrop*>> sortedRaindrops;
        for (const auto& raindrop : raindrops) {
            if (raindrop.visible && raindrop.state == 0) {
                float distance = glm::length(raindrop.position - cameraPos);
                sortedRaindrops.push_back({distance, &raindrop});
            }
        }
        
        // 从远到近排序
        std::sort(sortedRaindrops.begin(), sortedRaindrops.end(), 
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        
        for (const auto& [distance, raindrop] : sortedRaindrops) {
            // 设置模型矩阵
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, raindrop->position);
            raindropShader->setMat4("model", model);
            
            // 基于距离的动态大小调整 - 显著改善层次感
            float baseSizeScale = 100.0f / std::max(distance, 10.0f); // 防止除零
            float finalSize = raindrop->size * baseSizeScale;
            
            // 近处雨滴明显更大，远处雨滴相对较小
            if (raindrop->layerDepth < 0.3f) {
                finalSize *= 3.0f; // 近处雨滴3倍大小
            } else if (raindrop->layerDepth < 0.6f) {
                finalSize *= 2.0f; // 中等距离2倍大小
            }
            
            // 设置雨滴属性
            glm::vec3 enhancedColor = raindrop->color * raindrop->brightness;
            // 添加荧光效果
            float glowEffect = 1.0f + 0.4f * sin(totalTime * raindrop->twinkleSpeed + raindrop->position.x);
            enhancedColor *= glowEffect;
            
            raindropShader->setVec3("raindropColor", enhancedColor);
            raindropShader->setFloat("raindropSize", finalSize);
            raindropShader->setFloat("brightness", raindrop->brightness);
            
            // 绘制雨滴
            glDrawArrays(GL_POINTS, 0, 1);
        }
        
        glDisable(GL_POINT_SMOOTH);
        glDisable(GL_PROGRAM_POINT_SIZE);
        glBindVertexArray(0);
    }
    
    void renderRipples(const glm::mat4& view, const glm::mat4& projection) {
        rippleShader->use();
        
        // 设置变换矩阵
        rippleShader->setMat4("view", view);
        rippleShader->setMat4("projection", projection);
        
        // 增强的透明度混合设置
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 改为加法混合以增强可见性
        
        // 启用线条抗锯齿
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        
        // 遍历所有水波 - 改进的涟漪渲染
        glBindVertexArray(rippleVAO);
        for (const auto& ripple : ripples) {
            // 多层涟漪渲染以增强效果
            for (int layer = 0; layer < 3; layer++) {
                glm::mat4 model = glm::mat4(1.0f);
                
                // 添加水面波浪高度偏移
                glm::vec3 ripplePos = ripple.position;
                ripplePos.y += ripple.getCurrentWaveHeight() * sin(totalTime * 2.0f + layer);
                model = glm::translate(model, ripplePos);
                
                // 每层略微不同的旋转和大小
                float layerRotation = totalTime * (0.1f + layer * 0.05f);
                model = glm::rotate(model, layerRotation, glm::vec3(0.0f, 1.0f, 0.0f));
                
                float layerScale = ripple.radius * (1.0f + layer * 0.1f);
                model = glm::scale(model, glm::vec3(layerScale));
                
                rippleShader->setMat4("model", model);
                
                // 多层颜色效果
                float layerIntensity = 1.0f - (layer * 0.3f);
                float colorPulse = 1.0f + 0.3f * sin(totalTime * ripple.pulseFrequency + layer);
                glm::vec3 layerColor = ripple.color * colorPulse * layerIntensity * config.rippleVisibility;
                
                // 增强涟漪亮度和对比度
                layerColor = glm::min(layerColor * 2.0f, glm::vec3(1.0f));
                
                rippleShader->setVec3("rippleColor", layerColor);
                
                // 动态透明度 - 更强的初始透明度
                float layerOpacity = ripple.opacity * layerIntensity * 0.8f;
                rippleShader->setFloat("opacity", layerOpacity);
                
                // 绘制水波环
                glDrawArrays(GL_TRIANGLES, 0, 6 * 256 * config.rippleRings);
            }
        }
        
        glDisable(GL_LINE_SMOOTH);
        
        // 恢复标准透明度混合
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(0);
    }
    
    // New: render sky
    void renderSky(const glm::mat4& view, const glm::mat4& projection) {
        // 保存当前深度测试状态
        GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
        
        // 禁用深度测试，确保天空在后面
        glDisable(GL_DEPTH_TEST);
        
        skyShader->use();
        
        // 创建单独的视图矩阵，仅保留旋转部分，不包含平移
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        
        // 设置变换矩阵 - 天空跟随相机旋转但不跟随位置
        glm::mat4 model = glm::mat4(1.0f);
        skyShader->setMat4("model", model);
        skyShader->setMat4("view", skyView); // 使用特殊的天空视图矩阵
        skyShader->setMat4("projection", projection);
        
        // 设置天空属性 - 不再依赖纹理
        skyShader->setFloat("time", totalTime * 0.5f); // 加快时间变化以增加动态效果
        
        // 绘制天空
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, skyVertexCount);
        glBindVertexArray(0);
        
        // 恢复原始深度测试状态
        if (depthTest) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
    }
    
    // New: render moon
    void renderMoon(const glm::mat4& view, const glm::mat4& projection) {
        moonShader->use();
        
        // Set moon position
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(MOON_X, MOON_Y, -100.0f));
        model = glm::scale(model, glm::vec3(MOON_SIZE));
        moonShader->setMat4("model", model);
        moonShader->setMat4("view", view);
        moonShader->setMat4("projection", projection);
        
        // Set moon color - pale yellow
        glm::vec3 moonColor(0.98f, 0.97f, 0.85f);
        moonShader->setVec3("raindropColor", moonColor);
        moonShader->setFloat("raindropSize", 1.0f); // Don't need point size, drawing triangles
        moonShader->setFloat("brightness", 1.0f);
        
        // Draw moon
        glBindVertexArray(moonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 66); // Vertex count of disc
        glBindVertexArray(0);
        
        // Draw moon halo
        float haloSize = MOON_SIZE * 1.5f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(MOON_X, MOON_Y, -100.0f));
        model = glm::scale(model, glm::vec3(haloSize));
        moonShader->setMat4("model", model);
        
        // Set moon halo color - transparent light blue
        glm::vec3 haloColor(0.6f, 0.7f, 0.9f);
        moonShader->setVec3("raindropColor", haloColor);
        moonShader->setFloat("brightness", 0.4f);
        
        glBindVertexArray(moonVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 66);
        glBindVertexArray(0);
    }
    
    // New: render stars
    void renderStars(const glm::mat4& view, const glm::mat4& projection) {
        starShader->use();
        
        // Set transformation matrices
        starShader->setMat4("view", view);
        starShader->setMat4("projection", projection);
        
        // Enable point size
        glEnable(GL_PROGRAM_POINT_SIZE);
        
        // Draw all stars
        glBindVertexArray(raindropVAO); // Reuse raindrop VAO
        
        for (const auto& star : stars) {
            // Set model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, star.position);
            starShader->setMat4("model", model);
            
            // Set star properties
            glm::vec3 starColor(0.9f, 0.9f, 1.0f); // White with slight blue tint
            starShader->setVec3("raindropColor", starColor);
            starShader->setFloat("raindropSize", star.size * 2.0f);
            starShader->setFloat("brightness", star.brightness);
            
            // Draw star
            glDrawArrays(GL_POINTS, 0, 1);
        }
        
        glBindVertexArray(0);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
    
    // 新增：渲染闪电效果
    void renderLightning(const glm::mat4& view, const glm::mat4& projection) {
        if (lightnings.empty()) return;
        
        lightningShader->use();
        lightningShader->setMat4("view", view);
        lightningShader->setMat4("projection", projection);
        
        // 启用线条渲染设置
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 加法混合用于闪电发光效果
        
        glBindVertexArray(lightningVAO);
        
        for (const auto& lightning : lightnings) {
            if (!lightning.active || lightning.segments.size() < 2) continue;
            
            // 渲染主闪电路径
            for (int i = 0; i < lightning.segments.size() - 1; i++) {
                // 更新线条顶点数据
                std::vector<float> lineVertices = {
                    lightning.segments[i].x, lightning.segments[i].y, lightning.segments[i].z,
                    lightning.segments[i+1].x, lightning.segments[i+1].y, lightning.segments[i+1].z
                };
                
                glBindBuffer(GL_ARRAY_BUFFER, lightningVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
                
                // 设置模型矩阵
                glm::mat4 model = glm::mat4(1.0f);
                lightningShader->setMat4("model", model);
                
                // 闪电颜色和强度
                glm::vec3 lightningColor = lightning.color * lightning.intensity * config.lightningIntensity;
                lightningShader->setVec3("lightningColor", lightningColor);
                lightningShader->setFloat("intensity", lightning.intensity);
                
                // 动态线条宽度
                float lineWidth = lightning.thickness * lightning.intensity;
                glLineWidth(std::max(lineWidth, 1.0f));
                
                // 绘制闪电段
                glDrawArrays(GL_LINES, 0, 2);
            }
            
            // 渲染闪电光晕效果 - 多层渲染
            for (int glow = 1; glow <= 3; glow++) {
                for (int i = 0; i < lightning.segments.size() - 1; i++) {
                    std::vector<float> lineVertices = {
                        lightning.segments[i].x, lightning.segments[i].y, lightning.segments[i].z,
                        lightning.segments[i+1].x, lightning.segments[i+1].y, lightning.segments[i+1].z
                    };
                    
                    glBindBuffer(GL_ARRAY_BUFFER, lightningVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, lineVertices.size() * sizeof(float), lineVertices.data());
                    
                    glm::mat4 model = glm::mat4(1.0f);
                    lightningShader->setMat4("model", model);
                    
                    // 光晕颜色 - 逐层衰减
                    float glowIntensity = lightning.intensity * (0.5f / glow);
                    glm::vec3 glowColor = lightning.color * glowIntensity * 0.3f;
                    lightningShader->setVec3("lightningColor", glowColor);
                    lightningShader->setFloat("intensity", glowIntensity);
                    
                    // 光晕线条宽度
                    float glowWidth = lightning.thickness * (1.0f + glow * 2.0f) * lightning.intensity;
                    glLineWidth(std::max(glowWidth, 1.0f));
                    
                    glDrawArrays(GL_LINES, 0, 2);
                }
            }
        }
        
        // 恢复渲染状态
        glDisable(GL_LINE_SMOOTH);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(0);
    }
    
    void renderUI() {
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Create control panel
        ImGui::Begin("Control Panel");
        
        ImGui::Text("Colorful Rain Simulation");
        ImGui::Separator();
        
        // Stats display
        char fpsText[32];
        sprintf(fpsText, "FPS: %.1f", performanceMetrics.smoothedFps);
        ImGui::Text(fpsText);
        
        ImGui::Text("Raindrops: %lu", raindrops.size());
        ImGui::Text("Ripples: %lu", ripples.size());
        
        ImGui::Separator();
        
        // Rain settings
        if (ImGui::CollapsingHeader("Rain Settings")) {
            ImGui::SliderInt("Rain Density", &config.rainDensity, 50, 800); // 提高密度范围
            ImGui::SliderFloat("Min Raindrop Size", &config.minRaindropSize, 0.3f, 1.5f); // 调整雨滴大小范围
            ImGui::SliderFloat("Max Raindrop Size", &config.maxRaindropSize, 1.0f, 4.0f); // 大幅提高最大大小
            ImGui::SliderFloat("Min Raindrop Speed", &config.minRaindropSpeed, 1.0f, 5.0f);
            ImGui::SliderFloat("Max Raindrop Speed", &config.maxRaindropSpeed, 3.0f, 10.0f);
            
            // Color editing
            if (ImGui::TreeNode("Raindrop Colors")) {
                for (int i = 0; i < config.raindropColors.size(); i++) {
                    char colorName[32];
                    sprintf(colorName, "Color %d", i+1);
                    float color[3] = {
                        config.raindropColors[i].r,
                        config.raindropColors[i].g,
                        config.raindropColors[i].b
                    };
                    if (ImGui::ColorEdit3(colorName, color)) {
                        config.raindropColors[i] = glm::vec3(color[0], color[1], color[2]);
                    }
                }
                ImGui::TreePop();
            }
        }
        
        // Water settings
        if (ImGui::CollapsingHeader("Water Settings")) {
            ImGui::SliderFloat("Wave Strength", &config.waveStrength, 0.0f, 3.0f); // 增加波浪强度范围
            ImGui::SliderFloat("Max Ripple Size", &config.maxRippleSize, 20.0f, 150.0f); // 大幅增加涟漪大小范围
            ImGui::SliderFloat("Ripple Visibility", &config.rippleVisibility, 0.5f, 5.0f); // 新增涟漪可见度控制
            ImGui::SliderInt("Ripple Rings", &config.rippleRings, 2, 8); // 增加涟漪环数范围
            ImGui::SliderFloat("Update Interval", &config.updateInterval, 0.01f, 0.1f);
            
            // Ripple color editing
            if (ImGui::TreeNode("Ripple Colors")) {
                for (int i = 0; i < config.rippleColors.size(); i++) {
                    char colorName[32];
                    sprintf(colorName, "Color %d", i+1);
                    float color[3] = {
                        config.rippleColors[i].r,
                        config.rippleColors[i].g,
                        config.rippleColors[i].b
                    };
                    if (ImGui::ColorEdit3(colorName, color)) {
                        config.rippleColors[i] = glm::vec3(color[0], color[1], color[2]);
                    }
                }
                ImGui::TreePop();
            }
        }
        
        // Lightning settings - 新增闪电设置
        if (ImGui::CollapsingHeader("Lightning Settings")) {
            ImGui::Checkbox("Enable Lightning", &config.lightningEnabled);
            ImGui::SliderFloat("Lightning Frequency (s)", &config.lightningFrequency, 2.0f, 20.0f);
            ImGui::SliderFloat("Lightning Intensity", &config.lightningIntensity, 0.1f, 3.0f);
            
            ImGui::Text("Active Lightning: %lu", lightnings.size());
        }
        
        // Camera settings
        if (ImGui::CollapsingHeader("Camera Settings")) {
            ImGui::SliderFloat("Camera Speed", &config.cameraSpeed, 1.0f, 30.0f);
            ImGui::Text("Controls:");
            ImGui::BulletText("WASD: Move camera");
            ImGui::BulletText("Space/Ctrl: Up/Down");
            ImGui::BulletText("Arrow Keys: Rotate view");
            
            ImGui::Separator();
            
            // Current camera info
            ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", cameraPos.x, cameraPos.y, cameraPos.z);
            ImGui::Text("Look Direction: (%.1f, %.1f, %.1f)", cameraFront.x, cameraFront.y, cameraFront.z);
        }
        
        // Audio settings
        if (ImGui::CollapsingHeader("Audio Settings")) {
            bool soundEnabledChanged = ImGui::Checkbox("Enable Sound", &audioConfig.soundEnabled);
            
            ImGui::SliderFloat("Master Volume", &audioConfig.masterVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("Raindrop Volume", &audioConfig.raindropVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("Ambient Rain Volume", &audioConfig.ambientVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("Ripple Volume", &audioConfig.rippleVolume, 0.0f, 1.0f);
            
            // Update SDL audio settings when changed
            if (ImGui::IsItemEdited() || soundEnabledChanged) {
                updateAudioSettings();
            }
        }
        
        ImGui::End();
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

// Raindrop::update method - enhanced with trail effects
bool Raindrop::update(float deltaTime) {
    lifetime += deltaTime;
    
    // Update distance from camera for layer depth calculation
    if (simulation) {
        distanceFromCamera = glm::length(position - simulation->cameraPos);
        layerDepth = std::min(distanceFromCamera / 200.0f, 1.0f);
    }
    
    // Enhanced twinkling with layer-based variations
    brightness = 0.7f + 0.3f * sin(lifetime * twinkleSpeed + position.x * 0.1f);
    brightness *= (1.2f - layerDepth * 0.4f); // 近处雨滴更亮
    
    if (state == 0) { // Falling state
        // Update trail positions before updating main position
        updateTrail(deltaTime);
        
        position += velocity * deltaTime;
        
        // Enhanced motion with layer-dependent swaying
        float swayAmount = 0.1f * (1.0f - layerDepth); // 近处雨滴摆动更明显
        velocity.x += (cos(lifetime * 3.0f + position.z) * swayAmount - velocity.x * 0.1f) * deltaTime;
        velocity.z += (sin(lifetime * 2.5f + position.x) * swayAmount - velocity.z * 0.1f) * deltaTime;
        
        // Gravity with layer-dependent acceleration
        float gravityMultiplier = 0.8f + layerDepth * 0.4f; // 远处雨滴受重力影响更大
        velocity.y -= 2.0f * gravityMultiplier * deltaTime;
        
        // Check if hit water surface
        if (position.y <= WATER_HEIGHT) {
            state = 1; // Water entry state
            visible = false;
            
            // Play water entry sound
            if (simulation) {
                simulation->playRaindropSound(position);
            }
            
            return true; // Tell to create ripple
        }
    } else if (state == 1) { // Water entry state
        // If below water surface, gradually fade
        brightness -= deltaTime * 3.0f;
        if (brightness <= 0.0f) {
            state = 2; // Disappeared state
        }
    }
    
    return false;
}

// Write shader files
void writeShaderFiles() {
    // Create shader directory
    if (!file_exists("shaders")) {
        create_directory("shaders");
    }
    
    // 创建新的天空着色器：shaders/sky.vert
    const char* skyVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoords = aTexCoords;
}
)";
    
    // 创建新的天空着色器：shaders/sky.frag
    const char* skyFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float time;

void main() {
    // 基于纹理坐标创建夜空渐变
    float height = TexCoords.y;
    
    // 夜空渐变：从地平线的深蓝到天顶的黑色
    vec3 horizonColor = vec3(0.15, 0.25, 0.45);  // 地平线深蓝色
    vec3 zenithColor = vec3(0.02, 0.02, 0.08); // 天顶接近黑色
    
    // 基于高度插值，使用平滑步进函数
    float gradientFactor = smoothstep(0.0, 1.0, height);
    gradientFactor = pow(gradientFactor, 0.8); // 调整渐变曲线
    vec3 skyColor = mix(horizonColor, zenithColor, gradientFactor);
    
    // 添加更自然的星星分布
    float starField = 0.0;
    vec2 starCoord = TexCoords * 80.0; // 调整星星密度
    
    // 使用多层噪声创建更自然的星星分布
    float star1 = fract(sin(dot(floor(starCoord), vec2(12.9898, 78.233))) * 43758.5453);
    float star2 = fract(sin(dot(floor(starCoord * 1.3), vec2(93.9898, 67.345))) * 28458.5453);
    
    // 只在天空上半部分显示星星，并且有随机分布
    if (star1 > 0.996 && height > 0.4) {
        float twinkle = 0.6 + 0.4 * sin(time * 2.0 + star1 * 50.0);
        starField += twinkle * 0.8 * (0.5 + 0.5 * star2);
    }
    
    // 添加一些较小的星星
    if (star2 > 0.998 && height > 0.3) {
        float twinkle = 0.4 + 0.3 * sin(time * 3.0 + star2 * 80.0);
        starField += twinkle * 0.4;
    }
    
    // 添加月光晕染效果
    vec2 moonPos = vec2(0.75, 0.85);
    float moonDist = distance(TexCoords, moonPos);
    vec3 moonGlow = vec3(0.6, 0.6, 0.4) * smoothstep(0.25, 0.0, moonDist) * 0.4;
    
    // 添加微妙的云层效果
    float cloudPattern = sin(TexCoords.x * 15.0 + time * 0.1) * sin(TexCoords.y * 8.0 + time * 0.05);
    vec3 cloudColor = vec3(0.05, 0.05, 0.1) * smoothstep(0.3, 0.8, cloudPattern) * 0.3;
    
    // 最终颜色组合
    vec3 finalColor = skyColor + starField * vec3(0.9, 0.9, 1.0) + moonGlow + cloudColor;
    
    FragColor = vec4(finalColor, 1.0);
}
)";

    // Vertex shader - water (enhanced)
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
    
    // 多层复杂波浪效果 - 增加更多层次
    vec3 pos = aPos;
    
    // 第一层：大波浪
    float wave1 = sin(pos.x * 0.08 + time * waveSpeed) * cos(pos.z * 0.08 + time * waveSpeed * 0.8) * waveStrength;
    
    // 第二层：中等波浪
    float wave2 = sin(pos.x * 0.15 + time * waveSpeed * 1.3) * cos(pos.z * 0.12 + time * waveSpeed * 1.1) * waveStrength * 0.6;
    
    // 第三层：小波浪
    float wave3 = sin(pos.x * 0.25 + time * waveSpeed * 1.8) * cos(pos.z * 0.22 + time * waveSpeed * 1.5) * waveStrength * 0.3;
    
    // 第四层：微波
    float wave4 = sin(pos.x * 0.4 + time * waveSpeed * 2.2) * cos(pos.z * 0.35 + time * waveSpeed * 2.0) * waveStrength * 0.15;
    
    // 第五层：细微波纹
    float wave5 = sin(pos.x * 0.6 + time * waveSpeed * 2.8) * cos(pos.z * 0.55 + time * waveSpeed * 2.5) * waveStrength * 0.08;
    
    pos.y = wave1 + wave2 + wave3 + wave4 + wave5;
    
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoords = aTexCoords;
    
    // 计算更精确的法线 - 基于所有波浪层的导数
    // X方向导数
    float dx1 = 0.08 * cos(pos.x * 0.08 + time * waveSpeed) * cos(pos.z * 0.08 + time * waveSpeed * 0.8) * waveStrength;
    float dx2 = 0.15 * cos(pos.x * 0.15 + time * waveSpeed * 1.3) * cos(pos.z * 0.12 + time * waveSpeed * 1.1) * waveStrength * 0.6;
    float dx3 = 0.25 * cos(pos.x * 0.25 + time * waveSpeed * 1.8) * cos(pos.z * 0.22 + time * waveSpeed * 1.5) * waveStrength * 0.3;
    float dx4 = 0.4 * cos(pos.x * 0.4 + time * waveSpeed * 2.2) * cos(pos.z * 0.35 + time * waveSpeed * 2.0) * waveStrength * 0.15;
    float dx5 = 0.6 * cos(pos.x * 0.6 + time * waveSpeed * 2.8) * cos(pos.z * 0.55 + time * waveSpeed * 2.5) * waveStrength * 0.08;
    
    // Z方向导数
    float dz1 = 0.08 * sin(pos.x * 0.08 + time * waveSpeed) * -sin(pos.z * 0.08 + time * waveSpeed * 0.8) * waveStrength;
    float dz2 = 0.12 * sin(pos.x * 0.15 + time * waveSpeed * 1.3) * -sin(pos.z * 0.12 + time * waveSpeed * 1.1) * waveStrength * 0.6;
    float dz3 = 0.22 * sin(pos.x * 0.25 + time * waveSpeed * 1.8) * -sin(pos.z * 0.22 + time * waveSpeed * 1.5) * waveStrength * 0.3;
    float dz4 = 0.35 * sin(pos.x * 0.4 + time * waveSpeed * 2.2) * -sin(pos.z * 0.35 + time * waveSpeed * 2.0) * waveStrength * 0.15;
    float dz5 = 0.55 * sin(pos.x * 0.6 + time * waveSpeed * 2.8) * -sin(pos.z * 0.55 + time * waveSpeed * 2.5) * waveStrength * 0.08;
    
    vec3 tangent = normalize(vec3(1.0, dx1 + dx2 + dx3 + dx4 + dx5, 0.0));
    vec3 bitangent = normalize(vec3(0.0, dz1 + dz2 + dz3 + dz4 + dz5, 1.0));
    Normal = normalize(cross(tangent, bitangent));
}
)";

    // Fragment shader - water (enhanced with better ripple integration)
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
    // Enhanced distorted texture coordinates for better wave effects
    vec2 distortedTexCoords = vec2(
        TexCoords.x + sin(TexCoords.y * 15.0 + time * 1.2) * 0.015,
        TexCoords.y + sin(TexCoords.x * 12.0 + time * 0.8) * 0.012
    );
    
    // Generate enhanced dynamic normal
    vec3 normal = normalize(Normal);
    
    // Multi-layer normal disturbance for more realistic water surface
    normal.x += sin(TexCoords.x * 40.0 + time * 4.0) * sin(TexCoords.y * 30.0 + time * 3.0) * 0.04;
    normal.z += cos(TexCoords.x * 35.0 + time * 3.5) * cos(TexCoords.y * 45.0 + time * 4.5) * 0.04;
    
    // Add fine detail ripples
    normal.x += sin(TexCoords.x * 80.0 + time * 8.0) * sin(TexCoords.y * 70.0 + time * 7.0) * 0.01;
    normal.z += cos(TexCoords.x * 75.0 + time * 7.5) * cos(TexCoords.y * 85.0 + time * 8.5) * 0.01;
    
    normal = normalize(normal);
    
    // Enhanced ambient lighting
    vec3 ambient = vec3(0.08, 0.12, 0.25);
    
    vec3 result = ambient;
    
    // Main moonlight source with enhanced intensity
    {
        vec3 lightDir = normalize(vec3(0.4, 1.0, 0.2));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.7, 0.8, 1.0) * 0.4;
        
        // Enhanced specular reflection
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
        vec3 specular = spec * vec3(1.0, 1.0, 1.0) * 0.8;
        
        result += diffuse + specular;
    }
    
    // Secondary light sources for more complex lighting
    {
        vec3 lightDir = normalize(vec3(-0.6, 0.8, 0.3));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.2, 0.3, 0.5) * 0.2;
        result += diffuse;
    }
    
    // Enhanced water color system
    vec3 waterColorDeep = vec3(0.02, 0.08, 0.18); // Deeper blue
    vec3 waterColorShallow = vec3(0.15, 0.4, 0.7); // Brighter shallow water
    
    // More sophisticated fresnel calculation
    vec3 viewDir = normalize(viewPos - FragPos);
    float fresnelFactor = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.5);
    
    // Dynamic water color blending
    vec3 waterColor = mix(waterColorDeep, waterColorShallow, 
                          fresnelFactor * 0.6 + 0.3 * sin(time * 0.2) + 0.2);
    
    // Enhanced reflection system
    float skyFresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 1.8);
    vec3 skyColor = vec3(0.02, 0.05, 0.12);
    
    // Enhanced moonlight reflection
    vec2 moonPos = vec2(0.75, 0.82);
    float moonDist = distance(distortedTexCoords, moonPos);
    vec3 moonColor = vec3(0.9, 0.9, 0.7) * smoothstep(0.2, 0.0, moonDist) * 1.2;
    
    // Enhanced star reflections
    float stars = 0.0;
    float starNoise = fract(sin(distortedTexCoords.x * 150.0) * sin(distortedTexCoords.y * 150.0) * 43758.5453);
    if (starNoise > 0.995) {
        stars = 0.6 + 0.4 * sin(time * 3.0 + distortedTexCoords.x * 15.0);
    }
    
    vec3 reflection = skyColor + moonColor + stars * vec3(0.9, 0.9, 1.0);
    
    // More sophisticated blending
    result = mix(result, reflection, skyFresnel * 0.6);
    result = mix(waterColor, result, 0.7);
    
    // Enhanced edge highlighting for wave crests
    float edgeHighlight = pow(1.0 - abs(dot(normal, vec3(0.0, 1.0, 0.0))), 12.0) * 0.8;
    result += vec3(edgeHighlight * 0.5, edgeHighlight * 0.7, edgeHighlight);
    
    // Add foam effect on wave peaks
    float waveHeight = sin(TexCoords.x * 20.0 + time * 2.0) + cos(TexCoords.y * 18.0 + time * 1.8);
    if (waveHeight > 1.5) {
        result += vec3(0.3, 0.4, 0.5) * (waveHeight - 1.5) * 0.5;
    }
    
    // Dynamic transparency
    float alpha = 0.85 + edgeHighlight * 0.15;
    
    FragColor = vec4(result, alpha);
}
)";

    // Vertex shader - raindrop (enhanced)
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
    gl_PointSize = raindropSize / gl_Position.w; // Size adjusted by distance
    Color = raindropColor;
    Brightness = brightness;
}
)";

    // Fragment shader - raindrop (enhanced with better glow effects)
    const char* raindropFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 Color;
in float Brightness;

void main() {
    // Create circular point with improved gradient
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist = length(circCoord);
    
    // Discard pixels outside circle
    if (dist > 1.0) {
        discard;
    }
    
    // Multi-layer glow effect for meteor-like appearance
    float coreBrightness = 1.0 - smoothstep(0.0, 0.3, dist);  // Bright core
    float middleGlow = 1.0 - smoothstep(0.2, 0.7, dist);      // Middle glow
    float outerGlow = 1.0 - smoothstep(0.5, 1.0, dist);       // Outer glow
    
    // Combine glow layers
    float totalGlow = coreBrightness * 2.0 + middleGlow * 1.5 + outerGlow * 0.8;
    
    // Enhanced color with bloom effect
    vec3 finalColor = Color * Brightness * totalGlow;
    
    // Add sparkle effect for nearby raindrops
    float sparkle = 1.0 + 0.3 * sin(dist * 20.0) * (1.0 - dist);
    finalColor *= sparkle;
    
    // Dynamic alpha for proper blending
    float alpha = totalGlow * 0.9;
    
    // Boost brightness for better visibility
    finalColor = clamp(finalColor * 1.5, 0.0, 3.0);
    
    FragColor = vec4(finalColor, alpha);
}
)";

    // Vertex shader - water ripple (enhanced)
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

    // Fragment shader - water ripple (enhanced with better visibility)
    const char* rippleFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 rippleColor;
uniform float opacity;

void main() {
    // Calculate ripple's radial position
    vec3 center = vec3(0.0, 0.0, 0.0);
    vec2 fromCenter = vec2(FragPos.x, FragPos.z);
    float dist = length(fromCenter);
    
    // Multi-frequency wave patterns for realistic ripple appearance
    float mainWave = sin(dist * 25.0) * 0.8;
    float detailWave = sin(dist * 50.0) * 0.3;
    float fineDetail = sin(dist * 100.0) * 0.1;
    
    float wavePattern = mainWave + detailWave + fineDetail;
    
    // Enhanced color with wave pattern
    vec3 color = rippleColor * (1.0 + wavePattern * 0.5);
    
    // Improved edge handling for better visibility
    float edgeFade = smoothstep(0.85, 1.0, dist);
    float innerFade = smoothstep(0.0, 0.15, dist);
    float ringIntensity = smoothstep(0.2, 0.8, abs(sin(dist * 30.0)));
    
    // Combine all factors for final intensity
    float intensity = (1.0 - edgeFade) * innerFade * (0.6 + ringIntensity * 0.4);
    
    // Enhanced brightness for better visibility
    color *= intensity * 2.0;
    float alpha = opacity * intensity;
    
    // Boost alpha for better visibility against water
    alpha = clamp(alpha * 1.5, 0.0, 1.0);
    
    FragColor = vec4(color, alpha);
}
)";
    
    std::ofstream skyVert("shaders/sky.vert");
    skyVert << skyVertexShader;
    skyVert.close();

    std::ofstream skyFrag("shaders/sky.frag");
    skyFrag << skyFragmentShader;
    skyFrag.close();

    // Water shaders
    std::ofstream waterVert("shaders/water.vert");
    waterVert << waterVertexShader;
    waterVert.close();
    
    std::ofstream waterFrag("shaders/water.frag");
    waterFrag << waterFragmentShader;
    waterFrag.close();
    
    // Raindrop shaders
    std::ofstream raindropVert("shaders/raindrop.vert");
    raindropVert << raindropVertexShader;
    raindropVert.close();
    
    std::ofstream raindropFrag("shaders/raindrop.frag");
    raindropFrag << raindropFragmentShader;
    raindropFrag.close();
    
    // Water ripple shaders
    std::ofstream rippleVert("shaders/ripple.vert");
    rippleVert << rippleVertexShader;
    rippleVert.close();
    
    std::ofstream rippleFrag("shaders/ripple.frag");
    rippleFrag << rippleFragmentShader;
    rippleFrag.close();
    
    // Lightning shaders - 新增闪电着色器
    const char* lightningVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const char* lightningFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 lightningColor;
uniform float intensity;

void main() {
    // 闪电发光效果
    vec3 finalColor = lightningColor * intensity;
    
    // 限制颜色范围防止过曝
    finalColor = clamp(finalColor, 0.0, 2.0);
    
    // 添加闪烁效果
    float flicker = 0.8 + 0.2 * fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453);
    finalColor *= flicker;
    
    FragColor = vec4(finalColor, intensity);
}
)";

    // Write lightning shaders
    std::ofstream lightningVert("shaders/lightning.vert");
    lightningVert << lightningVertexShader;
    lightningVert.close();
    
    std::ofstream lightningFrag("shaders/lightning.frag");
    lightningFrag << lightningFragmentShader;
    lightningFrag.close();

    // Create texture directory
    if (!file_exists("textures")) {
        create_directory("textures");
    }
    
    // Create audio directory
    if (!file_exists("audio")) {
        create_directory("audio");
    }
}

// Use SDL-compatible main function
#ifdef __WINDOWS__
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
#else
int main(int argc, char* argv[]) {
#endif

    setConsoleCodePage();
    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Write shader files
    writeShaderFiles();
    
    // Set random seed
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Create and run simulation
    RainSimulation simulation;
    if (simulation.init()) {
        simulation.run();
    }
    
    // Clean up SDL before exit
    SDL_Quit();
    
    return 0;
}