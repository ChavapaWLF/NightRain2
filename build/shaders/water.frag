
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
    // Generate distorted texture coordinates
    vec2 distortedTexCoords = vec2(
        TexCoords.x + sin(TexCoords.y * 10.0 + time) * 0.01,
        TexCoords.y + sin(TexCoords.x * 10.0 + time * 0.8) * 0.01
    );
    
    // Generate dynamic normal without normal map
    vec3 normal = normalize(Normal);
    
    // Dynamic changing normal to simulate small water ripples
    normal.x += sin(TexCoords.x * 30.0 + time * 3.0) * sin(TexCoords.y * 20.0 + time * 2.0) * 0.03;
    normal.z += cos(TexCoords.x * 25.0 + time * 2.5) * cos(TexCoords.y * 35.0 + time * 3.5) * 0.03;
    normal = normalize(normal);
    
    // Ambient light
    vec3 ambient = vec3(0.05, 0.1, 0.2);
    
    // Diffuse - use multiple light sources
    vec3 result = ambient;
    
    // Main light source - moonlight
    {
        vec3 lightDir = normalize(vec3(0.3, 1.0, 0.1));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.6, 0.7, 0.9) * 0.3; // Soft blue-white moonlight
        
        // Reflection
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = spec * vec3(0.8, 0.9, 1.0) * 0.5;
        
        result += diffuse + specular;
    }
    
    // Second light source - ambient
    {
        vec3 lightDir = normalize(vec3(-0.5, 0.5, 0.2));
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(0.1, 0.2, 0.4) * 0.1; // Slight blue ambient light
        
        result += diffuse;
    }
    
    // Water color - blend deep and shallow tones
    vec3 waterColorDeep = vec3(0.0, 0.05, 0.15); // Deep water
    vec3 waterColorShallow = vec3(0.1, 0.3, 0.6); // Shallow water
    
    // More vertical view angle shows more of the bottom
    float fresnelFactor = pow(1.0 - max(dot(normal, normalize(viewPos - FragPos)), 0.0), 3.0);
    
    // Dynamically blend deep/shallow water colors based on view and waves
    vec3 waterColor = mix(waterColorDeep, waterColorShallow, 
                          fresnelFactor * 0.5 + 0.2 * sin(time * 0.1) + 0.3);
    
    // Reflection effect - generate simulated reflection without reflection texture
    vec3 reflection = vec3(0.0);
    
    // Generate simulated night sky reflection
    float skyFresnel = pow(1.0 - max(dot(normal, normalize(viewPos - FragPos)), 0.0), 2.0);
    vec3 skyColor = vec3(0.0, 0.02, 0.1); // Deep blue night sky
    
    // Add simulated moonlight reflection and stars
    vec2 moonPos = vec2(0.7, 0.8); // Moon position in reflection
    float moonDist = distance(distortedTexCoords, moonPos);
    vec3 moonColor = vec3(0.8, 0.8, 0.6) * smoothstep(0.15, 0.0, moonDist) * 0.8;
    
    // Random stars
    float stars = 0.0;
    if (fract(sin(distortedTexCoords.x * 100.0) * sin(distortedTexCoords.y * 100.0) * 43758.5453) > 0.996) {
        stars = 0.5 + 0.5 * sin(time * 2.0 + distortedTexCoords.x * 10.0);
    }
    
    reflection = skyColor + moonColor + stars * vec3(0.8, 0.8, 1.0);
    
    // Final blend of all components
    result = mix(result, reflection, skyFresnel * 0.5);
    result = mix(waterColor, result, 0.5);
    
    // Add ripple edge highlight
    float edgeHighlight = pow(1.0 - abs(dot(normal, vec3(0.0, 1.0, 0.0))), 8.0) * 0.5;
    result += vec3(edgeHighlight);
    
    // Semi-transparent water effect
    float alpha = 0.8 + edgeHighlight * 0.2;
    
    FragColor = vec4(result, alpha);
}
