#version 330 core
out vec4 FragColor

in vec2 TexCoords
in vec3 FragPos
in vec3 Normal

uniform sampler2D normalMap
uniform sampler2D dudvMap
uniform sampler2D reflectionMap
uniform vec3 viewPos
uniform float time

void main() {
    // 扰动纹理坐标
    vec2 distortedTexCoords = texture(dudvMap, vec2(TexCoords.x + time * 0.05, TexCoords.y)).rg * 0.1
    distortedTexCoords = TexCoords + vec2(distortedTexCoords.x, distortedTexCoords.y)
    
    // 从法线贴图获取法线
    vec3 normal = texture(normalMap, distortedTexCoords).rgb
    normal = normalize(normal * 2.0 - 1.0)
    
    // 环境光
    vec3 ambient = vec3(0.1, 0.1, 0.3)
    
    // 漫反射
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5))
    float diff = max(dot(normal, lightDir), 0.0)
    vec3 diffuse = diff * vec3(0.3, 0.5, 0.7)
    
    // 反射
    vec3 viewDir = normalize(viewPos - FragPos)
    vec3 reflectDir = reflect(-lightDir, normal)
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0)
    vec3 specular = spec * vec3(0.5)
    
    // 反射贴图
    float ratio = 0.6 + 0.4 * clamp(1.0 - dot(Normal, viewDir), 0.0, 1.0)
    vec2 reflectionCoord = vec2(TexCoords.x + normal.x * 0.05, TexCoords.y + normal.z * 0.05)
    vec3 reflection = texture(reflectionMap, reflectionCoord).rgb
    
    // 最终颜色
    vec3 result = ambient + diffuse + specular
    result = mix(result, reflection, ratio * 0.5)
    result = mix(vec3(0.0, 0.1, 0.3), result, 0.8) // 混合深蓝色水底
    
    FragColor = vec4(result, 0.8) // 水面半透明
}