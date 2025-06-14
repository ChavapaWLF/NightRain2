
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D normalMap;
uniform sampler2D dudvMap;
uniform sampler2D reflectionMap;
uniform vec3 viewPos;
uniform float time;
uniform float waterDepth;
uniform float waveStrength;

void main() {
    // Enhanced distorted texture coordinates for better wave effects
    vec2 distortedTexCoords = vec2(
        TexCoords.x + sin(TexCoords.y * 15.0 + time * 1.2) * 0.015,
        TexCoords.y + sin(TexCoords.x * 12.0 + time * 0.8) * 0.012
    );
    
    // Generate enhanced dynamic normal
    vec3 normal = normalize(Normal);
    
    // Multi-layer normal disturbance for more realistic water surface
    normal.x += sin(TexCoords.x * 40.0 + time * 4.0) * sin(TexCoords.y * 30.0 + time * 3.0) * 0.04;
    normal.z += cos(TexCoords.x * 35.0 + time * 3.5) * cos(TexCoords.y * 45.0 + time * 4.5) * 0.04;
    
    // Add fine detail ripples
    normal.x += sin(TexCoords.x * 80.0 + time * 8.0) * sin(TexCoords.y * 70.0 + time * 7.0) * 0.01;
    normal.z += cos(TexCoords.x * 75.0 + time * 7.5) * cos(TexCoords.y * 85.0 + time * 8.5) * 0.01;
    
    normal = normalize(normal);
    
    // Enhanced ambient lighting
    vec3 ambient = vec3(0.08, 0.12, 0.25);
    
    vec3 result = ambient;
    
    // Main moonlight source with enhanced intensity
    {
        vec3 lightDir = normalize(vec3(0.4, 1.0, 0.2));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.7, 0.8, 1.0) * 0.4;
        
        // Enhanced specular reflection
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
        vec3 specular = spec * vec3(1.0, 1.0, 1.0) * 0.8;
        
        result += diffuse + specular;
    }
    
    // Secondary light sources for more complex lighting
    {
        vec3 lightDir = normalize(vec3(-0.6, 0.8, 0.3));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.2, 0.3, 0.5) * 0.2;
        result += diffuse;
    }
    
    // Enhanced water color system
    vec3 waterColorDeep = vec3(0.02, 0.08, 0.18); // Deeper blue
    vec3 waterColorShallow = vec3(0.15, 0.4, 0.7); // Brighter shallow water
    
    // More sophisticated fresnel calculation
    vec3 viewDir = normalize(viewPos - FragPos);
    float fresnelFactor = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.5);
    
    // Dynamic water color blending
    vec3 waterColor = mix(waterColorDeep, waterColorShallow, 
                          fresnelFactor * 0.6 + 0.3 * sin(time * 0.2) + 0.2);
    
    // Enhanced reflection system
    float skyFresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 1.8);
    vec3 skyColor = vec3(0.02, 0.05, 0.12);
    
    // Enhanced moonlight reflection
    vec2 moonPos = vec2(0.75, 0.82);
    float moonDist = distance(distortedTexCoords, moonPos);
    vec3 moonColor = vec3(0.9, 0.9, 0.7) * smoothstep(0.2, 0.0, moonDist) * 1.2;
    
    // Enhanced star reflections
    float stars = 0.0;
    float starNoise = fract(sin(distortedTexCoords.x * 150.0) * sin(distortedTexCoords.y * 150.0) * 43758.5453);
    if (starNoise > 0.995) {
        stars = 0.6 + 0.4 * sin(time * 3.0 + distortedTexCoords.x * 15.0);
    }
    
    vec3 reflection = skyColor + moonColor + stars * vec3(0.9, 0.9, 1.0);
    
    // More sophisticated blending
    result = mix(result, reflection, skyFresnel * 0.6);
    result = mix(waterColor, result, 0.7);
    
    // Enhanced edge highlighting for wave crests
    float edgeHighlight = pow(1.0 - abs(dot(normal, vec3(0.0, 1.0, 0.0))), 12.0) * 0.8;
    result += vec3(edgeHighlight * 0.5, edgeHighlight * 0.7, edgeHighlight);
    
    // Add foam effect on wave peaks
    float waveHeight = sin(TexCoords.x * 20.0 + time * 2.0) + cos(TexCoords.y * 18.0 + time * 1.8);
    if (waveHeight > 1.5) {
        result += vec3(0.3, 0.4, 0.5) * (waveHeight - 1.5) * 0.5;
    }
    
    // Dynamic transparency
    float alpha = 0.85 + edgeHighlight * 0.15;
    
    FragColor = vec4(result, alpha);
}
