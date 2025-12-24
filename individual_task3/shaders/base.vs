#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexTangent;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPosition;
out mat3 TBN;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));

    vec3 T = normalize(vec3(matModel * vertexTangent));
    vec3 N = normalize(vec3(matModel * vec4(vertexNormal, 0.0)));
    vec3 B = cross(N, T) * vertexTangent.w;
    TBN = mat3(T, B, N);

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}