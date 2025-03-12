
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
