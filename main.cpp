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
const int STARS_COUNT = 300;      // Number of stars in night sky
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

// Raindrop class
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
    RainSimulation* simulation; // Pointer to main app for sound functions
    float brightness; // Brightness variation
    float twinkleSpeed; // Twinkle speed
    float trail; // Raindrop trail length

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
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f, // Slight horizontal movement
            -2.0f - static_cast<float>(rand()) / RAND_MAX * 3.0f,  // More velocity variation
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f  // Slight front/back movement
        );
        size = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.15f;
        lifespan = 3.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        lifetime = 0.0f;
        visible = true;
        state = 0;
        brightness = 0.8f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
        twinkleSpeed = 1.0f + static_cast<float>(rand()) / RAND_MAX * 5.0f;
        trail = 0.5f + static_cast<float>(rand()) / RAND_MAX * 1.5f; // Trail length
    }

    bool update(float deltaTime);  // Declaration, but implemented after RainSimulation class

    bool isDead() const {
        return state > 1 || lifetime > lifespan;
    }
};

// Water ripple class
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
    float pulseFrequency; // Pulse frequency
    float pulseAmplitude; // Pulse amplitude
    
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
        radius = 1.0f; // 增大初始半径
        maxRadius = 20.0f + static_cast<float>(rand()) / RAND_MAX * 30.0f; // 大幅增加最大半径
        thickness = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.4f; // 增加厚度
        opacity = 0.7f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
        growthRate = 5.0f + static_cast<float>(rand()) / RAND_MAX * 8.0f; // 大幅增加生长速率
        lifetime = 0.0f;
        maxLifetime = 3.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f; // 延长寿命
        pulseFrequency = 2.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        pulseAmplitude = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.3f; // 增加脉冲幅度
    }
    
    bool update(float deltaTime) {
        lifetime += deltaTime;
        
        // Non-linear growth - fast at start, then slower
        float progress = lifetime / maxLifetime;
        float growthFactor = 1.0f - progress * 0.7f;
        radius += growthRate * deltaTime * growthFactor;
        
        // Add pulsing effect
        thickness = 0.1f + 0.1f * sinf(lifetime * pulseFrequency) * pulseAmplitude;
        
        // Gradually reduce opacity, using a smooth decay curve
        opacity = 0.8f * (1.0f - powf(progress, 1.5f));
        
        return isDead();
    }
    
    bool isDead() const {
        return radius >= maxRadius || opacity <= 0.05f || lifetime >= maxLifetime;
    }
    
    // Get current thickness, taking pulsing into account
    float getCurrentThickness() const {
        return thickness;
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
    
    // Geometry
    unsigned int waterVAO, waterVBO;
    unsigned int raindropVAO, raindropVBO;
    unsigned int rippleVAO, rippleVBO;
    unsigned int skyVAO, skyVBO;          // New: sky
    unsigned int moonVAO, moonVBO;        // New: moon
    unsigned int starVAO, starVBO;        // New: stars
    unsigned int trailVAO, trailVBO;      // New: raindrop trail
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
    
    // Configuration
    struct {
        int rainDensity = 150;  // Increased default rain amount
        float maxRippleSize = 25.0f;
        float updateInterval = 0.01f; // More frequent updates
        float rippleFadeSpeed = 0.02f;
        std::vector<glm::vec3> raindropColors = {
            glm::vec3(0.7f, 0.0f, 0.9f), // Purple
            glm::vec3(0.0f, 0.8f, 1.0f), // Cyan
            glm::vec3(1.0f, 0.9f, 0.0f), // Yellow
            glm::vec3(1.0f, 0.3f, 0.0f), // Orange
            glm::vec3(0.0f, 0.9f, 0.4f)  // Teal
        };
        // New: ripple colors
        std::vector<glm::vec3> rippleColors = {
            glm::vec3(0.4f, 0.6f, 1.0f), // Light blue
            glm::vec3(0.6f, 0.8f, 1.0f), // Light cyan
            glm::vec3(0.7f, 0.7f, 1.0f), // Light purple
            glm::vec3(0.5f, 0.7f, 0.9f), // Sky blue
            glm::vec3(0.4f, 0.7f, 0.7f)  // Teal gray
        };
        // Raindrop size range
        float minRaindropSize = 0.1f;
        float maxRaindropSize = 0.3f;
        // Raindrop speed range
        float minRaindropSpeed = 2.0f;
        float maxRaindropSpeed = 6.0f;
        // Star twinkle speed
        float starTwinkleSpeed = 2.0f;
        // Ripple rings
        int rippleRings = 3;
        // Camera movement speed
        float cameraSpeed = 10.0f;
        // Water wave strength
        float waveStrength = 0.8f;
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
    cameraPos(glm::vec3(0.0f, 40.0f, 100.0f)), // 提高高度和距离
    cameraFront(glm::vec3(0.0f, -0.3f, -1.0f)), // 更倾斜地向下看
    cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    cameraPitch(-20.0f), // 更大的俯仰角
    cameraYaw(-90.0f),
    raindropSound(nullptr),
    ambientRainSound(nullptr),
    waterRippleSound(nullptr) {
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
        }
    }
    
    void createGeometry() {
        // 创建水面平面 - 使用更多顶点以支持更大的水面和更好的波浪效果
        std::vector<float> waterVertices;
        const int gridSize = 32;  // 增加网格密度
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
    }
    
    void generateRaindrops() {
        int raindropsToGenerate = config.rainDensity / 10; // 增加生成数量
        
        for (int i = 0; i < raindropsToGenerate; ++i) {
            if (rand() % 100 < 40) { // 提高生成概率到40%
                Raindrop raindrop;
                
                // 随机位置 - 在水面上方的更大区域，使用整个水面
                float x = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                float z = -POND_SIZE/2 + static_cast<float>(rand()) / RAND_MAX * POND_SIZE;
                float y = 20.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f;
                
                // 随机颜色
                int colorIndex = rand() % config.raindropColors.size();
                
                raindrop.init(glm::vec3(x, y, z), config.raindropColors[colorIndex], this);
                raindrops.push_back(raindrop);
            }
        }
    }
    
    void render() {
        // 清空缓冲
        glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 视图/投影矩阵
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        
        // 顺序很重要：先天空、再月亮和星星、然后水面、最后雨滴和波纹
        renderSky(view, projection);
        renderMoon(view, projection);
        renderStars(view, projection);
        renderWater(view, projection);
        renderRaindrops(view, projection);
        renderRipples(view, projection);
        
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
        
        // 设置水面属性 - 进一步增加波浪效果
        waterShader->setFloat("time", totalTime);
        waterShader->setVec3("viewPos", cameraPos);
        waterShader->setFloat("waveStrength", config.waveStrength * 10.0f); // 进一步增强波浪
        waterShader->setFloat("waveSpeed", 1.5f); // 加快波浪速度
        waterShader->setFloat("waterDepth", 0.8f); // 加深水色
        
        // 使用索引绘制水面
        glBindVertexArray(waterVAO);
        glDrawElements(GL_TRIANGLES, waterIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void renderRaindrops(const glm::mat4& view, const glm::mat4& projection) {
        // First render raindrop trails
        trailShader->use();
        trailShader->setMat4("view", view);
        trailShader->setMat4("projection", projection);
        
        glBindVertexArray(trailVAO);
        for (const auto& raindrop : raindrops) {
            if (!raindrop.visible || raindrop.state > 0 || raindrop.velocity.y > -2.0f)
                continue;
                
            // Set model matrix - scale and rotate trail based on raindrop velocity direction
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, raindrop.position);
            
            // Determine trail direction based on raindrop velocity
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 direction = glm::normalize(-raindrop.velocity);
            float angleY = atan2(direction.x, direction.z);
            float angleX = acos(glm::dot(direction, up));
            
            model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, angleX, glm::vec3(1.0f, 0.0f, 0.0f));
            
            // Scale trail based on velocity and size
            float trailLength = glm::length(raindrop.velocity) * raindrop.trail * 0.5f;
            model = glm::scale(model, glm::vec3(raindrop.size * 0.4f, trailLength, raindrop.size * 0.4f));
            
            trailShader->setMat4("model", model);
            
            // Set trail color - slightly transparent
            glm::vec3 trailColor = raindrop.color * 0.9f;
            float trailOpacity = 0.3f * raindrop.brightness;
            trailShader->setVec3("rippleColor", trailColor);
            trailShader->setFloat("opacity", trailOpacity);
            
            // Draw trail
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        
        // Then render raindrop points
        raindropShader->use();
        
        // Set transformation matrices
        raindropShader->setMat4("view", view);
        raindropShader->setMat4("projection", projection);
        
        // Enable point size
        glEnable(GL_PROGRAM_POINT_SIZE);
        
        // Loop through all raindrops
        glBindVertexArray(raindropVAO);
        for (const auto& raindrop : raindrops) {
            if (!raindrop.visible || raindrop.state > 0)
                continue;
                
            // Set model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, raindrop.position);
            raindropShader->setMat4("model", model);
            
            // Set raindrop properties - size varies with distance
            float distanceFactor = glm::length(raindrop.position - cameraPos);
            float sizeScale = std::max(0.5f, 30.0f / distanceFactor);
            
            raindropShader->setVec3("raindropColor", raindrop.color * raindrop.brightness);
            raindropShader->setFloat("raindropSize", raindrop.size * 15.0f * sizeScale); // Magnify point size
            
            // Add twinkle effect
            float twinkle = 0.7f + 0.3f * sin(totalTime * raindrop.twinkleSpeed);
            raindropShader->setFloat("brightness", raindrop.brightness * twinkle);
            
            // Draw raindrop
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
        
        // 增加波纹渲染的透明度混合
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // 遍历所有水波
        glBindVertexArray(rippleVAO);
        for (const auto& ripple : ripples) {
            // 设置模型矩阵
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, ripple.position);
            
            // 轻微旋转水波
            model = glm::rotate(model, totalTime * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
            
            // 使用更大的缩放
            model = glm::scale(model, glm::vec3(ripple.radius));
            
            rippleShader->setMat4("model", model);
            
            // 增加颜色亮度和透明度
            float colorPulse = 1.0f + 0.2f * sin(totalTime * ripple.pulseFrequency);
            glm::vec3 pulsingColor = ripple.color * colorPulse * 1.5f; // 增加亮度
            
            rippleShader->setVec3("rippleColor", pulsingColor);
            rippleShader->setFloat("opacity", ripple.opacity * 1.2f); // 增加透明度
            
            // 绘制水波
            glDrawArrays(GL_TRIANGLES, 0, 6 * 128 * config.rippleRings);
        }
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
        
        // 设置天空属性
        skyShader->setFloat("time", totalTime * 0.01f);
        skyShader->setVec3("viewPos", glm::vec3(0.0f)); // 天空中心位置
        
        // 为天空着色器设置特殊的Uniform以区别于水面着色器
        skyShader->setBool("isSky", true);
        
        // 设置纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyTexture);
        skyShader->setInt("skyTexture", 0);
        
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
            ImGui::SliderInt("Rain Density", &config.rainDensity, 10, 500);
            ImGui::SliderFloat("Min Raindrop Size", &config.minRaindropSize, 0.05f, 0.3f);
            ImGui::SliderFloat("Max Raindrop Size", &config.maxRaindropSize, 0.1f, 0.5f);
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
            ImGui::SliderFloat("Wave Strength", &config.waveStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Max Ripple Size", &config.maxRippleSize, 5.0f, 30.0f);
            ImGui::SliderInt("Ripple Rings", &config.rippleRings, 1, 5);
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

// Raindrop::update method - uses RainSimulation's playRaindropSound method
bool Raindrop::update(float deltaTime) {
    lifetime += deltaTime;
    
    // Random twinkling, using brightness rather than visibility
    brightness = 0.7f + 0.3f * sin(lifetime * twinkleSpeed + position.x * 0.1f);
    
    if (state == 0) { // Falling state
        position += velocity * deltaTime;
        
        // Add some random motion - raindrop slight swaying
        float swayAmount = 0.05f;
        velocity.x += (cos(lifetime * 3.0f + position.z) * swayAmount - velocity.x * 0.1f) * deltaTime;
        velocity.z += (sin(lifetime * 2.5f + position.x) * swayAmount - velocity.z * 0.1f) * deltaTime;
        
        // Velocity increases slightly over time - simulating gravity
        velocity.y -= 0.2f * deltaTime;
        
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
        brightness -= deltaTime * 2.0f;
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

uniform sampler2D skyTexture;
uniform float time;

void main() {
    // 采样天空纹理
    vec4 skyColor = texture(skyTexture, TexCoords);
    
    // 添加一些闪烁的星星
    float stars = 0.0;
    if (fract(sin(TexCoords.x * 100.0) * sin(TexCoords.y * 100.0) * 43758.5453) > 0.997) {
        stars = 0.5 + 0.5 * sin(time * 2.0 + TexCoords.x * 10.0);
    }
    
    // 最终颜色是天空纹理与闪烁星星的混合
    vec3 finalColor = skyColor.rgb + stars * vec3(0.8, 0.8, 1.0);
    
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
    
    // Multi-layer wave effect
    vec3 pos = aPos;
    float wave1 = sin(pos.x * 0.1 + time * waveSpeed) * cos(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    float wave2 = sin(pos.x * 0.2 + time * waveSpeed * 1.2) * cos(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    float wave3 = sin(pos.x * 0.05 + time * waveSpeed * 0.7) * cos(pos.z * 0.06 + time * waveSpeed * 0.9) * waveStrength * 0.3;
    
    pos.y = wave1 + wave2 + wave3;
    
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoords = aTexCoords;
    
    // Wave surface normal calculation (based on wave derivatives)
    float dx1 = 0.1 * cos(pos.x * 0.1 + time * waveSpeed) * cos(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    float dz1 = 0.1 * sin(pos.x * 0.1 + time * waveSpeed) * -sin(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    
    float dx2 = 0.2 * cos(pos.x * 0.2 + time * waveSpeed * 1.2) * cos(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    float dz2 = 0.15 * sin(pos.x * 0.2 + time * waveSpeed * 1.2) * -sin(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    
    vec3 tangent = normalize(vec3(1.0, dx1 + dx2, 0.0));
    vec3 bitangent = normalize(vec3(0.0, dz1 + dz2, 1.0));
    Normal = normalize(cross(tangent, bitangent));
}
)";

    // Fragment shader - water (enhanced)
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
    // Generate distorted texture coordinates
    vec2 distortedTexCoords = vec2(
        TexCoords.x + sin(TexCoords.y * 10.0 + time) * 0.01,
        TexCoords.y + sin(TexCoords.x * 10.0 + time * 0.8) * 0.01
    );
    
    // Generate dynamic normal without normal map
    vec3 normal = normalize(Normal);
    
    // Dynamic changing normal to simulate small water ripples
    normal.x += sin(TexCoords.x * 30.0 + time * 3.0) * sin(TexCoords.y * 20.0 + time * 2.0) * 0.03;
    normal.z += cos(TexCoords.x * 25.0 + time * 2.5) * cos(TexCoords.y * 35.0 + time * 3.5) * 0.03;
    normal = normalize(normal);
    
    // Ambient light
    vec3 ambient = vec3(0.05, 0.1, 0.2);
    
    // Diffuse - use multiple light sources
    vec3 result = ambient;
    
    // Main light source - moonlight
    {
        vec3 lightDir = normalize(vec3(0.3, 1.0, 0.1));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.6, 0.7, 0.9) * 0.3; // Soft blue-white moonlight
        
        // Reflection
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = spec * vec3(0.8, 0.9, 1.0) * 0.5;
        
        result += diffuse + specular;
    }
    
    // Second light source - ambient
    {
        vec3 lightDir = normalize(vec3(-0.5, 0.5, 0.2));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.1, 0.2, 0.4) * 0.1; // Slight blue ambient light
        
        result += diffuse;
    }
    
    // Water color - blend deep and shallow tones
    vec3 waterColorDeep = vec3(0.0, 0.05, 0.15); // Deep water
    vec3 waterColorShallow = vec3(0.1, 0.3, 0.6); // Shallow water
    
    // More vertical view angle shows more of the bottom
    float fresnelFactor = pow(1.0 - max(dot(normal, normalize(viewPos - FragPos)), 0.0), 3.0);
    
    // Dynamically blend deep/shallow water colors based on view and waves
    vec3 waterColor = mix(waterColorDeep, waterColorShallow, 
                          fresnelFactor * 0.5 + 0.2 * sin(time * 0.1) + 0.3);
    
    // Reflection effect - generate simulated reflection without reflection texture
    vec3 reflection = vec3(0.0);
    
    // Generate simulated night sky reflection
    float skyFresnel = pow(1.0 - max(dot(normal, normalize(viewPos - FragPos)), 0.0), 2.0);
    vec3 skyColor = vec3(0.0, 0.02, 0.1); // Deep blue night sky
    
    // Add simulated moonlight reflection and stars
    vec2 moonPos = vec2(0.7, 0.8); // Moon position in reflection
    float moonDist = distance(distortedTexCoords, moonPos);
    vec3 moonColor = vec3(0.8, 0.8, 0.6) * smoothstep(0.15, 0.0, moonDist) * 0.8;
    
    // Random stars
    float stars = 0.0;
    if (fract(sin(distortedTexCoords.x * 100.0) * sin(distortedTexCoords.y * 100.0) * 43758.5453) > 0.996) {
        stars = 0.5 + 0.5 * sin(time * 2.0 + distortedTexCoords.x * 10.0);
    }
    
    reflection = skyColor + moonColor + stars * vec3(0.8, 0.8, 1.0);
    
    // Final blend of all components
    result = mix(result, reflection, skyFresnel * 0.5);
    result = mix(waterColor, result, 0.5);
    
    // Add ripple edge highlight
    float edgeHighlight = pow(1.0 - abs(dot(normal, vec3(0.0, 1.0, 0.0))), 8.0) * 0.5;
    result += vec3(edgeHighlight);
    
    // Semi-transparent water effect
    float alpha = 0.8 + edgeHighlight * 0.2;
    
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

    // Fragment shader - raindrop (enhanced)
    const char* raindropFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 Color;
in float Brightness;

void main() {
    // Create circular point
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist = length(circCoord);
    
    // Smooth circular edge
    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);
    
    // Fade edge and brighten center
    if (dist > 1.0) {
        discard;
    }
    
    // Create raindrop glow effect
    float innerGlow = 1.0 - dist * dist;
    float outerGlow = 0.5 * (1.0 - smoothstep(0.5, 1.0, dist));
    
    // Adjust brightness and transparency based on raindrop size
    vec3 finalColor = Color * Brightness * (0.7 + 0.6 * innerGlow);
    float finalAlpha = alpha * (0.6 + 0.4 * innerGlow);
    
    // Add slight internal structure
    float detail = 0.1 * sin(circCoord.x * 10.0) * sin(circCoord.y * 10.0);
    finalColor += detail * innerGlow * Brightness;
    
    FragColor = vec4(finalColor, finalAlpha);
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

    // Fragment shader - water ripple (enhanced)
    const char* rippleFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 rippleColor;
uniform float opacity;

void main() {
    // Simulate water ripple flash and transparency changes
    vec3 color = rippleColor;
    
    // Use fragment position to calculate ripple's radial position
    vec3 center = vec3(0.0, 0.0, 0.0); // Ripple center in model space
    vec2 fromCenter = vec2(FragPos.x, FragPos.z);
    float dist = length(fromCenter);
    
    // Add texture variation - make ripple more detailed
    float detail = sin(dist * 20.0) * 0.1;
    color += detail * rippleColor;
    
    // Smooth edges
    float edgeFade = smoothstep(0.9, 1.0, dist);
    float innerFade = smoothstep(0.0, 0.2, dist);
    
    // Final color and transparency
    color = color * (1.0 - edgeFade) * innerFade;
    float alpha = opacity * (1.0 - edgeFade) * innerFade;
    
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