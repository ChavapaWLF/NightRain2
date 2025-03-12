import numpy as np
from PIL import Image, ImageFilter, ImageDraw
import os
from scipy.ndimage import gaussian_filter

def generate_water_dudv_map(width=512, height=512, filename="textures/waterDuDv.jpg"):
    """
    生成水面UV坐标扰动贴图，用于模拟水波动态效果
    
    DuDv贴图存储纹理坐标偏移量，用于水面波纹效果：
    - 红色通道: 水平(U)扰动
    - 绿色通道: 垂直(V)扰动
    - 蓝色通道: 未使用(保持为0)
    
    值以128为中心(无扰动)，范围为[0, 255]
    """
    # 创建扰动值数组
    du_map = np.zeros((height, width), dtype=np.float32)
    dv_map = np.zeros((height, width), dtype=np.float32)
    
    # 波浪参数，用于增加复杂度
    wave_params = [
        # 振幅, x频率, y频率, 相位, 方向
        (0.03, 0.01, 0.03, 0.0, 0.0),
        (0.02, 0.02, 0.01, 1.5, 0.7),
        (0.01, 0.04, 0.02, 3.0, 1.5),
        (0.02, 0.03, 0.02, 0.5, 2.2),
        (0.01, 0.01, 0.04, 2.0, 3.0),
        (0.02, 0.02, 0.02, 4.0, 3.9),
        (0.01, 0.03, 0.01, 1.0, 4.5),
        (0.02, 0.01, 0.03, 2.5, 5.2),
    ]
    
    # 生成波浪图案
    for y in range(height):
        for x in range(width):
            du_val = 0.0
            dv_val = 0.0
            
            # 计算所有波浪的贡献
            for amp, freq_x, freq_y, phase, direction in wave_params:
                angle = x * freq_x + y * freq_y + phase
                wave_value = amp * np.sin(angle)
                
                # 投影到U/V轴上，考虑方向影响
                du_val += wave_value * np.cos(direction)
                dv_val += wave_value * np.sin(direction)
            
            # 存储值
            du_map[y, x] = du_val
            dv_map[y, x] = dv_val
    
    # 归一化到DuDv贴图的合适范围
    du_max = np.max(np.abs(du_map))
    dv_max = np.max(np.abs(dv_map))
    max_val = max(du_max, dv_max)
    
    du_map = du_map / max_val * 0.5  # 缩放到[-0.5, 0.5]范围
    dv_map = dv_map / max_val * 0.5
    
    # 以0.5为中心(在0-1范围内的中性值)
    du_map += 0.5
    dv_map += 0.5
    
    # 确保值被限制在[0, 1]范围内
    du_map = np.clip(du_map, 0, 1)
    dv_map = np.clip(dv_map, 0, 1)
    
    # 转换为RGB图像
    rgb_image = np.zeros((height, width, 3), dtype=np.uint8)
    rgb_image[:, :, 0] = np.uint8(du_map * 255)  # 红色通道 - U扰动
    rgb_image[:, :, 1] = np.uint8(dv_map * 255)  # 绿色通道 - V扰动
    
    # 创建PIL图像并保存
    image = Image.fromarray(rgb_image)
    image.save(filename, quality=95)
    
    print(f"水面DuDv扰动贴图生成成功: {filename}")
    print(f"分辨率: {width}x{height} 像素")
    print("格式: JPEG (24位色深)")
    print("水平扰动存储在红色通道，垂直扰动存储在绿色通道")
    print("贴图已准备好用于水面着色器中创建动态水效果")

def generate_raindrop_glow(width=128, height=128, filename="textures/raindrop_glow.png"):
    """
    生成雨滴光晕效果贴图
    
    特点:
    - 放射状的柔和白色光晕
    - 中心亮度最高，向外逐渐透明
    - 带有Alpha通道确保边缘平滑过渡
    - 用于雨滴的渲染，使雨滴在夜空中具有微弱的发光效果
    """
    # 确保目录存在
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    # 创建带Alpha通道的图像 (RGBA)
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    
    # 创建绘图对象和数组以进行处理
    data = np.zeros((height, width, 4), dtype=np.uint8)
    
    # 计算中心点
    center_x, center_y = width // 2, height // 2
    max_radius = min(width, height) // 2
    
    # 创建放射状光晕
    for y in range(height):
        for x in range(width):
            # 计算到中心的距离
            dx = x - center_x
            dy = y - center_y
            distance = np.sqrt(dx*dx + dy*dy)
            
            # 归一化距离
            normalized_distance = distance / max_radius
            
            # 计算亮度和透明度
            if normalized_distance < 1.0:
                # 使用平滑的指数衰减函数
                intensity = np.exp(-4.0 * normalized_distance * normalized_distance)
                
                # 设置颜色 (白色光晕)
                brightness = int(255 * intensity)
                alpha = int(255 * intensity)
                
                data[y, x] = [brightness, brightness, brightness, alpha]
    
    # 从数组创建图像
    img = Image.fromarray(data, 'RGBA')
    
    # 添加轻微的高斯模糊以使光晕更加柔和
    img = img.filter(ImageFilter.GaussianBlur(radius=1.0))
    
    # 保存为PNG (保留Alpha通道)
    img.save(filename)
    
    print(f"雨滴光晕贴图生成成功: {filename}")
    print(f"分辨率: {width}x{height} 像素")
    print("格式: PNG (32位带Alpha通道)")
    print("放射状的柔和白色光晕，中心亮度最高，向外逐渐透明")
    print("用于雨滴的渲染，使雨滴在夜空中具有微弱的发光效果")

def generate_water_reflection_map(width=1024, height=1024, filename="textures/waterReflection.jpg"):
    """
    生成水面反射贴图，模拟夜空的反射效果
    
    特点:
    - 深蓝色调的夜空纹理
    - 模糊的星光点
    - 云层效果
    - 中央区域较亮，边缘渐暗（模拟月光）
    """
    # 确保目录存在
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    # 创建深蓝色背景
    base_color = (5, 10, 30)  # 深蓝色调
    img = Image.new('RGB', (width, height), base_color)
    
    # 创建绘图对象和数组以进行处理
    draw = ImageDraw.Draw(img)
    data = np.array(img)
    
    # 生成星星
    num_stars = 2000
    stars = []
    np.random.seed(42)  # 保持结果一致
    
    # 生成不同大小和亮度的星星
    for _ in range(num_stars):
        x = np.random.randint(0, width)
        y = np.random.randint(0, height)
        size = np.random.choice([1, 1, 1, 2, 2, 3])  # 大多是小星星
        brightness = np.random.randint(150, 256)
        stars.append((x, y, size, brightness))
    
    # 绘制星星
    for x, y, size, brightness in stars:
        color = (brightness, brightness, brightness)
        if size == 1:
            draw.point((x, y), fill=color)
        else:
            draw.ellipse((x-size//2, y-size//2, x+size//2, y+size//2), fill=color)
    
    # 将图像转换为数组进行处理
    data = np.array(img)
    
    # 给星星添加模糊效果
    data_blurred = gaussian_filter(data, sigma=1.0)
    
    # 生成云层效果
    cloud_layer = np.zeros((height, width), dtype=np.float32)
    
    # 使用多层Perlin噪声的近似方法生成云效果
    scale = 8
    octaves = 6
    persistence = 0.5
    lacunarity = 2.0
    
    for octave in range(octaves):
        octave_scale = scale * (lacunarity ** octave)
        amplitude = persistence ** octave
        
        for y in range(height):
            for x in range(width):
                # 简化的噪声生成
                nx = x / width * octave_scale
                ny = y / height * octave_scale
                # 使用正弦函数组合模拟Perlin噪声
                noise_val = np.sin(nx) * np.sin(ny) * 0.5 + 0.5
                noise_val += np.sin(nx*1.7 + 1.3) * np.sin(ny*2.1 + 0.7) * 0.25 + 0.25
                cloud_layer[y, x] += noise_val * amplitude
    
    # 归一化云层值
    cloud_layer = (cloud_layer - np.min(cloud_layer)) / (np.max(cloud_layer) - np.min(cloud_layer))
    
    # 创建月光效果（中心更亮）
    center_x, center_y = width // 2, height // 2
    vignette = np.zeros((height, width), dtype=np.float32)
    
    max_dist = np.sqrt(center_x**2 + center_y**2)
    for y in range(height):
        for x in range(width):
            # 计算到中心的距离
            dist = np.sqrt((x - center_x)**2 + (y - center_y)**2)
            # 创建渐变效果
            vignette[y, x] = 1.0 - (dist / max_dist) ** 1.5  # 指数控制渐变强度
    
    # 应用云层和月光效果到图像
    for y in range(height):
        for x in range(width):
            # 从基础图像获取颜色
            r, g, b = data_blurred[y, x]
            
            # 应用云层（增加蓝白色调）
            cloud_value = cloud_layer[y, x] * 0.5  # 降低云层强度
            r = int(np.clip(r + cloud_value * 80, 0, 255))
            g = int(np.clip(g + cloud_value * 100, 0, 255))
            b = int(np.clip(b + cloud_value * 130, 0, 255))
            
            # 应用月光效果（增加亮度）
            moonlight = vignette[y, x] * 0.7  # 月光强度
            r = int(np.clip(r + moonlight * 30, 0, 255))
            g = int(np.clip(g + moonlight * 40, 0, 255))
            b = int(np.clip(b + moonlight * 40, 0, 255))
            
            data_blurred[y, x] = [r, g, b]
    
    # 从处理后的数组创建最终图像
    final_img = Image.fromarray(data_blurred.astype(np.uint8))
    
    # 添加轻微的模糊效果使反射更自然
    final_img = final_img.filter(ImageFilter.GaussianBlur(radius=1.5))
    
    # 保存图像
    final_img.save(filename, quality=95)
    
    print(f"水面反射贴图生成成功: {filename}")
    print(f"分辨率: {width}x{height} 像素")
    print("格式: JPEG (24位色深)")
    print("深蓝色调夜空纹理，包含星光和云层效果")
    print("贴图中央区域较亮，边缘渐暗，模拟月光照射下的夜空")

# 生成DuDv贴图
generate_water_dudv_map(512, 512, "textures/waterDuDv.jpg")

# 生成水面反射贴图
generate_water_reflection_map(1024, 1024, "textures/waterReflection.jpg")

# 生成雨滴光晕贴图
generate_raindrop_glow(128, 128, "textures/raindrop_glow.png")