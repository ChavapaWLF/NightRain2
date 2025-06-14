
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
