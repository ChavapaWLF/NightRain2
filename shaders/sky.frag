
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
