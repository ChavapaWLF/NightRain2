# 🌙池塘夜降彩色雨(NightRain2)

本项目是针对NightRain项目的3D改进版，基于OpenGL实现。

由于期末时间匆忙，本项目仍处于开发阶段，存在较多细节问题，欢迎指正。

## ✨ 主要功能

- **雨滴效果**: 多层次彩色雨滴，包含拖尾效果和动态大小
- **水面波纹系统**: 雨滴落水时产生的动态涟漪效果（待优化）
- **闪电系统**: 自动和手动触发的闪电效果
- **动态天空背景**: 夜空渐变、星星闪烁、月亮光晕（待优化）
- **音效系统**: 雨滴声、环境雨声、涟漪声音效果
- **实时参数调节**: 通过ImGui界面实时调整各种效果参数
- **流畅的相机控制**: 支持自由移动和旋转视角（待优化）

## 🎮 控制方式

### 相机控制
- **WASD**: 前后左右移动相机
- **空格键**: 相机上升
- **左Ctrl**: 相机下降
- **方向键**: 旋转视角
- **L键**: 手动触发闪电效果

### 界面控制
- **ESC**: 退出程序
- **鼠标**: 在ImGui控制面板中调节参数

## 🛠️ 编译依赖

### 必需库
- **OpenGL 3.3+**
- **GLFW 3.x**: 窗口管理
- **GLEW**: OpenGL扩展加载
- **GLM**: 数学库
- **SDL2**: 音频处理
- **SDL2_mixer**: 音频混合
- **ImGui**: 用户界面
- **stb_image**: 图像加载
- **stb_image_write**: 图像保存

### 编译指令

#### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

#### Windows (MinGW)
```bash
# 直接在根目录下编译
mingw32-make
```

#### Linux
```bash
mkdir build
cd build
cmake ..
make
```

#### macOS
```bash
mkdir build
cd build
cmake ..
make
```

## 📁 项目结构

```
ColorfulRainSimulation/
├── main.cpp                       # 主程序文件
├── CMakeLists.txt                 # CMake配置文件
├── README.md                      # 项目说明文档
├── LICENSE                        # 许可证文件
├── 材质包需求.txt                  # 材质包需求说明
├── Makefile                       # 编译生成的Makefile
├── ColorfulRainSimulation.exe     # 编译生成的可执行文件
├── SDL2.dll                       # SDL2动态链接库
├── SDL2_mixer.dll                 # SDL2音频混合库
├── download_dependencies.bat      # 依赖下载脚本
├── get_stb_headers.bat           # STB头文件获取脚本
├── imgui.ini                     # ImGui配置文件
├── include/                      # 头文件目录
│   ├── filesystem_compat.h      # 文件系统兼容层
│   ├── GLFW/                    # GLFW头文件
│   ├── GL/                      # OpenGL头文件
│   ├── glm/                     # GLM数学库头文件
│   ├── imgui/                   # ImGui头文件
│   ├── SDL2/                    # SDL2头文件
│   └── stb/                     # STB库头文件
├── lib/                          # 静态库文件目录
├── shaders/                      # 着色器文件夹
│   ├── water.vert              # 水面顶点着色器
│   ├── water.frag              # 水面片段着色器
│   ├── raindrop.vert           # 雨滴顶点着色器
│   ├── raindrop.frag           # 雨滴片段着色器
│   ├── ripple.vert             # 涟漪顶点着色器
│   ├── ripple.frag             # 涟漪片段着色器
│   ├── sky.vert                # 天空顶点着色器
│   ├── sky.frag                # 天空片段着色器
│   ├── lightning.vert          # 闪电顶点着色器
│   └── lightning.frag          # 闪电片段着色器
├── textures/                     # 纹理文件夹
│   ├── waternormal.jpeg        # 水面法线贴图（小）
│   ├── waternormal.jpg         # 水面法线贴图（中）
│   ├── waternormal_origin.jpeg # 水面法线贴图（原始）
│   ├── waternormal_mini.jpeg   # 水面法线贴图（迷你）
│   ├── waterDuDv.jpg           # 水面扭曲贴图
│   ├── waterReflection.jpg     # 水面反射贴图
│   ├── waterReflection_origin.jpg # 水面反射贴图（原始）
│   ├── raindrop_glow.png       # 雨滴发光贴图
│   ├── night_sky.jpg           # 夜空背景贴图
│   ├── night_sky1.jpg          # 夜空背景贴图1
│   ├── night_sky2.jpg          # 夜空背景贴图2
│   ├── night_sky3.jpeg         # 夜空背景贴图3
│   ├── night_sky4.jpg          # 夜空背景贴图4
│   └── night_sky_fullsize.jpg  # 夜空背景贴图（全尺寸）
├── audio/                        # 音频文件夹
│   ├── raindrop_splash.wav     # 雨滴落水音效
│   ├── ambient_rain.mp3        # 环境雨声
│   └── water_ripple.wav        # 涟漪音效
├── screenshots/                  # 截图目录
├── image_generator/              # 图像生成工具目录
├── build/                        # CMake构建目录
├── _deps/                        # CMake依赖目录
└── .vscode/                      # VS Code配置目录
```

## 🎨 参数调节

程序运行时，左侧会显示控制面板，可以实时调节以下参数：

### 雨滴设置
- **雨滴密度**: 控制雨滴生成数量
- **雨滴大小**: 最小/最大雨滴尺寸
- **雨滴速度**: 最小/最大下落速度
- **雨滴颜色**: 5种可调节的彩色配置

### 水面设置
- **波浪强度**: 水面波动幅度
- **最大涟漪大小**: 雨滴落水时涟漪扩散范围
- **涟漪可见度**: 涟漪的透明度和亮度
- **涟漪环数**: 每个涟漪的同心环数量
- **涟漪颜色**: 5种可调节的涟漪颜色

### 闪电设置
- **启用闪电**: 开关闪电效果
- **闪电频率**: 自动闪电间隔时间
- **闪电强度**: 闪电亮度和可见度
- **手动触发**: 按钮或L键手动生成闪电

### 相机设置
- **相机速度**: 移动和旋转速度

### 音频设置
- **启用声音**: 总开关
- **主音量**: 整体音量控制
- **雨滴音量**: 雨滴落水音效音量
- **环境音量**: 背景雨声音量
- **涟漪音量**: 涟漪音效音量

## 🔧 性能优化

### 已实施的优化
- **内存预分配**: 为雨滴轨迹和闪电路径预分配内存
- **视锥裁剪**: 只渲染可见范围内的对象
- **LOD系统**: 远距离对象使用简化渲染
- **批量渲染**: 减少GPU状态切换
- **智能更新**: 只在需要时更新对象状态

### 建议设置（低端设备）
- 雨滴密度: 100-150
- 最大涟漪大小: 40-60
- 涟漪环数: 3-4
- 星星数量: 减少到80个

## 🎵 音频文件

项目已包含必要的音频文件：

- **raindrop_splash.wav**: 水滴落水音效
- **ambient_rain.mp3**: 循环播放的雨声背景音
- **water_ripple.wav**: 涟漪音效

如需替换音频文件，建议格式和长度：
- **raindrop_splash.wav**: 0.5-1秒的水滴声
- **ambient_rain.mp3**: 循环播放的雨声背景音
- **water_ripple.wav**: 0.3-0.8秒的涟漪声

## 🐛 故障排除

### 常见问题

1. **编译错误 - 找不到库文件**
   - 确保安装了所有依赖库
   - 检查CMake是否正确找到库路径

2. **运行时黑屏**
   - 检查显卡驱动是否支持OpenGL 3.3
   - 尝试更新显卡驱动

3. **无声音**
   - 检查audio文件夹是否存在音频文件
   - 确认SDL2_mixer正确安装

4. **性能较低**
   - 降低雨滴密度和涟漪大小
   - 减少星星数量
   - 关闭音效

5. **着色器编译失败**
   - 确保shaders文件夹存在
   - 程序会自动生成默认着色器

## 🤝 贡献

欢迎提交Bug报告、功能请求或代码贡献！

## 📄 许可证

本项目仅供学习和研究使用。