
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
