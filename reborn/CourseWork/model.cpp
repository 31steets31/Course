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
        std::cout << "Failed to import model file: " << importer.GetErrorString() << std::endl;
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
        glm::vec3 pos, normal, velocity; 

        pos.x = mesh->mVertices[i].x;
        pos.y = mesh->mVertices[i].y;
        pos.z = mesh->mVertices[i].z;

        normal.x = mesh->mNormals[i].x;
        normal.y = mesh->mNormals[i].y;
        normal.z = mesh->mNormals[i].z;

        velocity.x = mesh->mNormals[i].x / 1900;
        velocity.y = mesh->mNormals[i].y / 1900;
        velocity.z = mesh->mNormals[i].z / 1900;

        vertex.Position = pos;
        vertex.Normal = normal;
        vertex.Velocity = velocity;

        vertices.push_back(vertex);
    }
    for (i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }
    for (i = 0; i < mesh->mNumFaces; i += 2)
    {
        Face face;

        face.Triangles.first = glm::uvec3(
            mesh->mFaces[i].mIndices[0],
            mesh->mFaces[i].mIndices[1],
            mesh->mFaces[i].mIndices[2]
        );

        face.Triangles.second = glm::uvec3(
            mesh->mFaces[i + 1].mIndices[0],
            mesh->mFaces[i + 1].mIndices[1],
            mesh->mFaces[i + 1].mIndices[2]
        );

        glm::vec3 faceVec1 = glm::vec3(
            vertices[face.Triangles.second.y].Position.x - vertices[face.Triangles.first.x].Position.x,
            vertices[face.Triangles.second.y].Position.y - vertices[face.Triangles.first.x].Position.y,
            vertices[face.Triangles.second.y].Position.z - vertices[face.Triangles.first.x].Position.z
        );

        glm::vec3 faceVec2 = glm::vec3(
            vertices[face.Triangles.second.z].Position.x - vertices[face.Triangles.first.x].Position.x,
            vertices[face.Triangles.second.z].Position.y - vertices[face.Triangles.first.x].Position.y,
            vertices[face.Triangles.second.z].Position.z - vertices[face.Triangles.first.x].Position.z
        );

        face.Normal = glm::normalize(glm::cross(faceVec1, faceVec2));

        faces.push_back(face);
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

bool isIntersecting(const glm::vec3& orig,
      const glm::vec3& dir,
      const glm::vec3& tri0,
      const glm::vec3& tri1,
      const glm::vec3& tri2) {
    const float EPSILON = 1e-4f;

    glm::vec3 edge1 = tri1 - tri0;
    glm::vec3 edge2 = tri2 - tri0;

    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON)
        return false; // The ray is parallel to the triangle

    float f = 1.0f / a;
    glm::vec3 s = orig - tri0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = f * glm::dot(edge2, q);

    return t > EPSILON;
}

void Model::updateVelocity()
{
    glm::mat4 mm = glm::translate(glm::mat4(1), glm::vec3(0, 0, -6.5));
    Vertex* pData = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);

    glm::mat4 m = glm::rotate(glm::mat4(1), glm::radians(20.0f), glm::vec3(0, 1, 0));

    float glTime = glfwGetTime();

    for (size_t i = 0; i < vertexCount; ++i)
    {
        glm::vec3 curVel = vertices[i].Velocity;
        glm::vec4 prevPos = mm * glm::vec4(vertices[i].Position, 1);
        glm::vec4 tPos = glm::vec4(vertices[i].Position, 1);
        glm::vec4 point = mm * tPos + glm::vec4(curVel * glTime, 0);

        //std::cout << point.x << point.y << point.z << " | " << point.w << std::endl;
        if (!isInsideRoom(point))
        {
            if (point.x < minRoomVert.x || point.x > maxRoomVert.x)
                curVel.x = -curVel.x;
            if (point.y < minRoomVert.y || point.y > maxRoomVert.y)
                curVel.y = -curVel.y;
            if (point.z < minRoomVert.z || point.z > maxRoomVert.z)
                curVel.z = -curVel.z;
        }
        else
            for (unsigned int i = 0; i < objects.size(); ++i)
            {
                std::vector<Face> objFaces = objects[i].faces;
                std::vector<Vertex> objVertices = objects[i].vertices;

                glm::vec3 pointToVertex0 = point - m * glm::vec4(objVertices[objFaces[0].Triangles.first.z].Position, 1);
                glm::vec3 pointToVertex1 = point - m * glm::vec4(objVertices[objFaces[1].Triangles.first.z].Position, 1);
                glm::vec3 pointToVertex2 = point - m * glm::vec4(objVertices[objFaces[2].Triangles.first.z].Position, 1);
                glm::vec3 pointToVertex3 = point - m * glm::vec4(objVertices[objFaces[3].Triangles.first.z].Position, 1);
                glm::vec3 pointToVertex4 = point - m * glm::vec4(objVertices[objFaces[4].Triangles.first.z].Position, 1);
                glm::vec3 pointToVertex5 = point - m * glm::vec4(objVertices[objFaces[5].Triangles.first.z].Position, 1);

                float pointPlane0 = glm::dot(pointToVertex0, glm::vec3(m * glm::vec4(objFaces[0].Normal, 1)));
                float pointPlane1 = glm::dot(pointToVertex1, glm::vec3(m * glm::vec4(objFaces[1].Normal, 1)));
                float pointPlane2 = glm::dot(pointToVertex2, glm::vec3(m * glm::vec4(objFaces[2].Normal, 1)));
                float pointPlane3 = glm::dot(pointToVertex3, glm::vec3(m * glm::vec4(objFaces[3].Normal, 1)));
                float pointPlane4 = glm::dot(pointToVertex4, glm::vec3(m * glm::vec4(objFaces[4].Normal, 1)));
                float pointPlane5 = glm::dot(pointToVertex5, glm::vec3(m * glm::vec4(objFaces[5].Normal, 1)));

                if (pointPlane0 < 0 && pointPlane1 < 0 && pointPlane2 < 0 && pointPlane3 < 0 && pointPlane4 < 0 && pointPlane5 < 0)
                {
                  glm::vec3 normal;
                  bool found = false;
                  for(uint32_t i = 0; i < 6; ++i) {
                    if(isIntersecting(
                            glm::vec3(prevPos),
                            glm::vec3(point),
                            glm::vec3(m * glm::vec4(objVertices[objFaces[i].Triangles.first.x].Position, 1)),
                            glm::vec3(m * glm::vec4(objVertices[objFaces[i].Triangles.first.y].Position, 1)),
                            glm::vec3(m * glm::vec4(objVertices[objFaces[i].Triangles.first.z].Position, 1))) || 
                        isIntersecting(
                            glm::vec3(prevPos),
                            glm::vec3(point),
                            glm::vec3(m * glm::vec4(objVertices[objFaces[i].Triangles.second.x].Position, 1)),
                            glm::vec3(m * glm::vec4(objVertices[objFaces[i].Triangles.second.y].Position, 1)),
                            glm::vec3(m * glm::vec4(objVertices[objFaces[i].Triangles.second.z].Position, 1))))
                    {
                      normal = objFaces[i].Normal;
                      found = true;
                      break;
                    }
                  }
                  if(found) {
                    normal = glm::normalize(glm::vec3(m * glm::vec4(normal, 1)));

                    glm::vec3 vel = glm::vec4(curVel, 1);

                    float dotProduct = 2 * glm::dot(vel, normal);
                    curVel -= dotProduct * normal;
                  }
                }


                /* if (!prevNormalDots[i].first) */
                /*     prevNormalDots[i].first = true; */
                /* //else if (pointPlane0 < 0 || pointPlane1 < 0 || pointPlane2 < 0 || pointPlane3 < 0 || pointPlane4 < 0 || pointPlane5 < 0) */
                /* //    continue; */
                /* else if (pointPlane0 < 0 && pointPlane1 < 0 && pointPlane2 < 0 && pointPlane3 < 0 && pointPlane4 < 0 && pointPlane5 < 0) */
                /* { */
                /*     glm::vec3 normal; */

                /*     if (prevNormalDots[i].second[0] > 0) */
                /*         normal = objFaces[0].Normal; */
                /*     else if (prevNormalDots[i].second[1] > 0) */
                /*         normal = objFaces[1].Normal; */
                /*     else if (prevNormalDots[i].second[2] > 0) */
                /*         normal = objFaces[2].Normal; */
                /*     else if (prevNormalDots[i].second[3] > 0) */
                /*         normal = objFaces[3].Normal; */
                /*     else if (prevNormalDots[i].second[4] > 0) */
                /*         normal = objFaces[4].Normal; */
                /*     else if (prevNormalDots[i].second[5] > 0) */
                /*         normal = objFaces[5].Normal; */
     /* /1*               else */
                /*         continue;*/

                /*     //normal = objFaces[0].Normal; */
                /*     normal = glm::normalize(glm::vec3(m * glm::vec4(normal, 1))); */

                /*     float dotProduct = 2 * (curVel.x * normal.x + curVel.y * normal.y + curVel.z * normal.z); */

                /*     curVel -= dotProduct * normal; */
                /* } */

                /* prevNormalDots[i].second[0] = pointPlane0; */
                /* prevNormalDots[i].second[1] = pointPlane1; */
                /* prevNormalDots[i].second[2] = pointPlane2; */
                /* prevNormalDots[i].second[3] = pointPlane3; */
                /* prevNormalDots[i].second[4] = pointPlane4; */
                /* prevNormalDots[i].second[5] = pointPlane5; */
            }


        pData[i].Velocity = curVel;
        vertices[i].Velocity = curVel;
        pData[i].Position = glm::vec4(glm::vec3(tPos) + curVel * glTime, 1);
        vertices[i].Position = glm::vec4(glm::vec3(tPos) + curVel * glTime, 1);

        if (i == -1000)
            std::cout << vertices[i].Position.x << vertices[i].Position.y << vertices[i].Position.z << " | " << point.w << std::endl;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}
