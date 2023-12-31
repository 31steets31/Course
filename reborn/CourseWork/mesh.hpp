#pragma once

#include "shader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GLFW/glfw3.h>


class Model;

struct Vertex 
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 Velocity;
};

struct Face
{
    std::pair<glm::uvec3, glm::uvec3> Triangles;
    glm::vec3 Normal;
};

class Mesh 
{
public:
    Mesh(Model& model);

    void Bind();
    void Draw(Shader& shader);
    void Unbind();

    void setupMesh(Model& model);
private:
    unsigned int VAO, VBO, EBO, indicesSize;
};
