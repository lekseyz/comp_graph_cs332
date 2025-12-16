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

vec3 calcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 diffuse = light.color * diff * light.intensity;
    vec3 specular = light.color * spec * specularStrength * light.intensity;
    diffuse *= attenuation;
    specular *= attenuation;
    return (diffuse + specular);
}

vec3 calcDirectionalLight(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 diffuse = light.color * diff * light.intensity;
    vec3 specular = light.color * spec * specularStrength * light.intensity;
    return (diffuse + specular);
}

vec3 calcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float spotIntensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 diffuse = light.color * diff * light.intensity;
    vec3 specular = light.color * spec * specularStrength * light.intensity;
    diffuse *= attenuation * spotIntensity;
    specular *= attenuation * spotIntensity;
    return (diffuse + specular);
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 result = ambient;
    
    for(int i = 0; i < lightsCount && i < MAX_LIGHTS; i++)
    {
        if(lights[i].type == LIGHT_POINT)
        {
            result += calcPointLight(lights[i], normal, fragPosition, viewDir);
        }
        else if(lights[i].type == LIGHT_DIRECTIONAL)
        {
            result += calcDirectionalLight(lights[i], normal, viewDir);
        }
        else if(lights[i].type == LIGHT_SPOT)
        {
            result += calcSpotLight(lights[i], normal, fragPosition, viewDir);
        }
    }
    
    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = vec4(result, 1.0) * texelColor * colDiffuse;
}
