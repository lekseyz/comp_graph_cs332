#version 330

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;
in mat3 TBN;

out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec4 colDiffuse;

const vec3 lightPos = vec3(50.0, 100.0, 50.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float ambientStrength = 0.4;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    
    vec3 normal = texture(texture1, fragTexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(TBN * normal);

    vec3 lightDir = normalize(lightPos - fragPosition);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 result = (ambient + diffuse) * texColor.rgb * colDiffuse.rgb;
    
    finalColor = vec4(result, texColor.a * colDiffuse.a);
}