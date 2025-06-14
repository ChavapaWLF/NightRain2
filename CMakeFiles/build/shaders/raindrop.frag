#version 330 core
out vec4 FragColor

in vec3 Color
uniform sampler2D glowTexture

void main() {
    // 创建圆形点
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0
    if (dot(circCoord, circCoord) > 1.0) {
        discard
    }
    
    // 添加一些亮度变化
    float brightness = 0.7 + 0.3 * (1.0 - length(circCoord))
    
    // 使用光晕贴图（如果有）
    // 这里我们假设没有贴图，直接使用计算的亮度
    
    FragColor = vec4(Color * brightness, 0.7)
}