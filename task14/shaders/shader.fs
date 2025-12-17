#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;

#define LIGHT_POINT 0
#define LIGHT_DIRECTIONAL 1
#define LIGHT_SPOT 2

#define PHONG 0
#define TOON 1
#define OREN_NAYAR 2

struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};

#define MAX_LIGHTS 4
uniform int lightsCount;
uniform Light lights[MAX_LIGHTS];

uniform vec3 ambient;
uniform float shininess;
uniform float specularStrength;
uniform int lightingModel;
uniform float roughness;

vec3 phong(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 lightColor) {
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 diffuse = lightColor * diff;
    vec3 specular = lightColor * spec * specularStrength;
    return diffuse + specular;
}

vec3 toon(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 lightColor) {
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    float toonDiff = floor(diff * 5.0) / 5.0;
    float toonSpec = floor(spec * 3.0) / 3.0;
    return lightColor * toonDiff + lightColor * toonSpec * specularStrength;
}

vec3 orenNayar(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 lightColor) {
    float LdotV = dot(lightDir, viewDir);
    float NdotL = max(0.0, dot(normal, lightDir));
    float NdotV = max(0.0, dot(normal, viewDir));
    
    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
    
    float sigma2 = roughness * roughness;
    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * (sigma2 / (sigma2 + 0.09));
    
    return lightColor * NdotL * (A + B * s / t);
}

vec3 calcLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir;
    float attenuation = 1.0;
    float spotIntensity = 1.0;

    if (light.type == LIGHT_POINT) {
        lightDir = normalize(light.position - fragPos);
        float distance = length(light.position - fragPos);
        attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    } else if (light.type == LIGHT_DIRECTIONAL) {
        lightDir = normalize(-light.direction);
    } else if (light.type == LIGHT_SPOT) {
        lightDir = normalize(light.position - fragPos);
        float theta = dot(lightDir, normalize(-light.direction));
        float epsilon = light.cutOff - light.outerCutOff;
        spotIntensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
        float distance = length(light.position - fragPos);
        attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    }

    vec3 result = vec3(0.0);
    switch (lightingModel) {
        case PHONG:
            result = phong(lightDir, viewDir, normal, light.color);
            break;
        case TOON:
            result = toon(lightDir, viewDir, normal, light.color);
            break;
        case OREN_NAYAR:
            result = orenNayar(lightDir, viewDir, normal, light.color);
            break;
    }

    return result * light.intensity * attenuation * spotIntensity;
}


void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 result = ambient;
    
    for(int i = 0; i < lightsCount && i < MAX_LIGHTS; i++)
    {
        result += calcLight(lights[i], normal, fragPosition, viewDir);
    }
    
    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = vec4(result, 1.0) * texelColor * colDiffuse;
}
