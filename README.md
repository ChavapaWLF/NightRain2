# 池塘夜雨彩色雨滴模拟

一个使用C++和OpenGL实现的高性能3D图形应用，模拟彩色雨滴落入水中的视觉和听觉效果。

## 项目特点

- 真实的3D雨滴下落物理模拟
- 逼真的水面渲染和水波纹扩散效果
- 彩色雨滴和水波效果，营造梦幻夜雨氛围
- 立体声音效，增强沉浸感
- 完整的用户界面，支持调整各种参数
- 高性能设计，支持大量粒子同时渲染

## 系统要求

### 最低配置
- 支持OpenGL 3.3或更高版本的显卡
- 2GB显存
- 4GB系统内存
- 支持C++17的编译器
- CMake 3.10或更高版本

### 推荐配置
- 支持OpenGL 4.5的中高端显卡
- 4GB或更高显存
- 8GB或更高系统内存
- 多核CPU

## 依赖库

本项目依赖以下第三方库：
- GLFW (窗口和输入处理)
- GLEW (OpenGL扩展加载)
- GLM (数学库)
- ImGui (用户界面)
- stb_image.h (图像加载)
- stb_image_write.h (图像写入)
- irrKlang (音频处理)

## 构建说明

### Windows
1. 安装必要的依赖库：
   ```
   vcpkg install glfw3 glew glm
   ```

2. 下载并安装ImGui和irrKlang

3. 使用CMake生成项目：
   ```
   mkdir build
   cd build
   cmake ..
   ```

4. 打开生成的Visual Studio项目并构建

### Linux
1. 安装必要的依赖库：
   ```
   sudo apt install libglfw3-dev libglew-dev libglm-dev
   ```

2. 下载并安装ImGui和irrKlang

3. 构建项目：
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

### macOS
1. 使用Homebrew安装依赖：
   ```
   brew install glfw glew glm
   ```

2. 下载并安装ImGui和irrKlang

3. 构建项目：
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

## 文件结构

```
/
├── main.cpp              # 主程序代码
├── CMakeLists.txt        # CMake构建配置
├── include/              # 头文件
│   ├── stb/              # STB图像库
│   ├── imgui/            # ImGui库
│   └── irrKlang/         # irrKlang音频库
├── lib/                  # 库文件
├── shaders/              # 着色器文件
│   ├── water.vert        # 水面顶点着色器
│   ├── water.frag        # 水面片段着色器
│   ├── raindrop.vert     # 雨滴顶点着色器
│   ├── raindrop.frag     # 雨滴片段着色器
│   ├── ripple.vert       # 水波顶点着色器
│   └── ripple.frag       # 水波片段着色器
├── textures/             # 纹理文件
│   ├── waternormal.jpg   # 水面法线贴图
│   ├── waterDuDv.jpg     # 水面扰动贴图
│   ├── waterReflection.jpg # 水面反射贴图
│   ├── raindrop_glow.png # 雨滴光晕贴图
│   └── night_sky.jpg     # 夜空背景贴图
└── audio/                # 音频文件
    ├── raindrop_splash.wav # 雨滴入水声音
    ├── ambient_rain.mp3  # 背景雨声
    └── water_ripple.wav  # 水波声音
```

## 资源文件

项目需要以下资源文件才能正常运行：

### 纹理文件
1. `textures/waternormal.jpg` - 水面法线贴图
2. `textures/waterDuDv.jpg` - 水面扰动贴图
3. `textures/waterReflection.jpg` - 水面反射贴图
4. `textures/raindrop_glow.png` - 雨滴光晕贴图
5. `textures/night_sky.jpg` - 夜空背景贴图

### 音频文件
1. `audio/raindrop_splash.wav` - 雨滴入水声音
2. `audio/ambient_rain.mp3` - 背景雨声
3. `audio/water_ripple.wav` - 水波声音

如果无法找到这些资源文件，程序会生成简单的默认资源作为替代，但为了获得最佳体验，建议使用高质量资源文件。

## 控制说明

- **WASD键** - 移动摄像机
- **鼠标** - 旋转视角
- **空格键** - 上升
- **左Shift键** - 下降
- **Esc键** - 退出程序

## 参数调整

通过界面可以调整以下参数：
- 雨点密度
- 最大水波大小
- 更新间隔
- 音频设置（主音量、雨滴音量、环境音量、水波音量）

## 性能调优

如果在低配置系统上遇到性能问题，可以：
1. 降低雨滴密度
2. 减小最大水波大小
3. 增加更新间隔
4. 关闭音频效果

## 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 致谢

- OpenGL和GLFW社区
- ImGui库
- STB库
- irrKlang音频引擎
- 所有测试和提供反馈的用户
