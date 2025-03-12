
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
