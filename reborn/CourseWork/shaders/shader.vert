#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 view;
uniform mat4 proj;

void main()
{
    FragPos = aPos;
    Normal = mat3(transpose(inverse(mat4(1)))) * aNormal;  
    gl_Position = proj * view * vec4(FragPos, 1.0);
}