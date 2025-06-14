
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
