#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec4 ambientColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(ambientColor);

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPosition);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(colDiffuse);

    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * vec3(1.0, 1.0, 1.0); 

    vec3 result = (ambient + diffuse + specular) * vec3(texelColor);
    finalColor = vec4(result, texelColor.a);
}