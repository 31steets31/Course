#include "model.hpp"


void Model::Draw(Shader& shader, bool updateVel)
{
    glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::vec3(2, 2, 2));

    for (unsigned int i = 0; i < meshes.size(); ++i)
    {
        meshes[i].Bind();

        if (updateVel)
            updateVelocity();

        meshes[i].Draw(shader);
        meshes[i].Unbind();
    }
}

void Model::loadModel(std::string path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "Failed to import file: " << importer.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        processNode(node->mChildren[i], scene);
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    unsigned int i;

    for (i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;
        glm::vec3 pos, velocity; 

        pos.x = mesh->mVertices[i].x;
        pos.y = mesh->mVertices[i].y;
        pos.z = mesh->mVertices[i].z;

        velocity.x = mesh->mNormals[i].x / 8000;
        velocity.y = mesh->mNormals[i].y / 8000;
        velocity.z = mesh->mNormals[i].z / 8000;

        vertex.Position = pos;
        vertex.Velocity = velocity;

        vertices.push_back(vertex);
    }
    for (i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    vertexCount = mesh->mNumVertices;

    return Mesh(vertices, indices);
}

bool Model::isInsideRoom(glm::vec4& point)
{
    return
        point.x < maxRoomVert.x &&
        point.x > minRoomVert.x &&
        point.y < maxRoomVert.y &&
        point.y > minRoomVert.y &&
        point.z < maxRoomVert.z &&
        point.z > minRoomVert.z;
}

glm::vec3 Model::getNormal(glm::vec3& point)
{
    int x = int(point.x), y = int(point.y), z = int(point.z);

    if (x == 1)
        return glm::vec3(-1, 0, 0);
    else if (x == -1)
        return glm::vec3(1, 0, 0);
    else if (y == 1)
        return glm::vec3(0, -1, 0);
    else if (y == -1)
        return glm::vec3(0, 1, 0);
    else if (z == -1)
        return glm::vec3(0, 0, 1);
    else if (z == 1)
        return glm::vec3(0, 0, -1);

    return glm::vec3(0, 0, 0);
}

void Model::updateVelocity()
{
    glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::vec3(0.2, 0.2, 0.2));
    m = glm::translate(m, glm::vec3(2, 2, -2));
    Vertex* pData = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);

    float glTime = glfwGetTime();

    for (size_t i = 0; i < vertexCount; ++i)
    {
        glm::vec3 curVel = vertices[i].Velocity;
        glm::vec4 tPos = glm::vec4(vertices[i].Position + curVel * glTime, 1);
        glm::vec4 point = m * tPos;

        //std::cout << point.x << point.y << point.z << " | " << point.w << std::endl;
        if (!isInsideRoom(point))
        {
            //std::cout << point.x << point.y << point.z << " | " << point.w << std::endl;
            if (point.x < minRoomVert.x || point.x > maxRoomVert.x) {
                curVel.x = -curVel.x;
            }
            if (point.y < minRoomVert.y || point.y > maxRoomVert.y) {
                curVel.y = -curVel.y;
            }
            if (point.z < minRoomVert.z || point.z > maxRoomVert.z) {
                curVel.z = -curVel.z;
            }

            // Calculate the normal vector of the obstacle
            double nx = 0.0, ny = 0.0, nz = 0.0;

            //for (std::vector<Vertex>& object : objects)
            //{
            //    get

            //    // Determine which side of the obstacle was hit and set the corresponding normal
            //    if (point.x < obstacle.x1 || point.x > obstacle.x2) {
            //        nx = (point.x < obstacle.x1) ? -1.0 : 1.0;
            //    }
            //    if (point.y < obstacle.y1 || point.y > obstacle.y2) {
            //        ny = (point.y < obstacle.y1) ? -1.0 : 1.0;
            //    }
            //    if (point.z < obstacle.z1 || point.z > obstacle.z2) {
            //        nz = (point.z < obstacle.z1) ? -1.0 : 1.0;
            //    }

            //    // Calculate the reflection of velocity vector
            //    double dotProduct = 2 * (point.vx * nx + point.vy * ny + point.vz * nz);
            //    point.vx -= dotProduct * nx;
            //    point.vy -= dotProduct * ny;
            //    point.vz -= dotProduct * nz;
            //}

            pData[i].Velocity = curVel;
            vertices[i].Velocity = curVel;
        }

        pData[i].Position = tPos;
        vertices[i].Position = tPos;
        if (i == -1000)
            std::cout << vertices[i].Position.x << vertices[i].Position.y << vertices[i].Position.z << " | " << point.w << std::endl;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}
