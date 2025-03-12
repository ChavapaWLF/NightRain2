
#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 rippleColor;
uniform float opacity;

void main() {
    // Simulate water ripple flash and transparency changes
    vec3 color = rippleColor;
    
    // Use fragment position to calculate ripple's radial position
    vec3 center = vec3(0.0, 0.0, 0.0); // Ripple center in model space
    vec2 fromCenter = vec2(FragPos.x, FragPos.z);
    float dist = length(fromCenter);
    
    // Add texture variation - make ripple more detailed
    float detail = sin(dist * 20.0) * 0.1;
    color += detail * rippleColor;
    
    // Smooth edges
    float edgeFade = smoothstep(0.9, 1.0, dist);
    float innerFade = smoothstep(0.0, 0.2, dist);
    
    // Final color and transparency
    color = color * (1.0 - edgeFade) * innerFade;
    float alpha = opacity * (1.0 - edgeFade) * innerFade;
    
    FragColor = vec4(color, alpha);
}
