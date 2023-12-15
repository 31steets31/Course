#include "loader.hpp"
#include "model.hpp"


void Loader::loadModel(const std::string& path, Model& model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | 
        aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "Failed to import model file: " << importer.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene, model);
}

void Loader::processNode(aiNode* node, const aiScene* scene, Model& model)
{
    unsigned int i;

    for (i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.pushMesh(processMesh(mesh, scene, model));
    }
    for (i = 0; i < node->mNumChildren; ++i)
        processNode(node->mChildren[i], scene, model);
}

Mesh Loader::processMesh(aiMesh* mesh, const aiScene* scene, Model& model)
{
    std::vector<Vertex>& vertices = model.getVertices();

    Vertex vertex;
    Face face;
    glm::vec3 pos, normal, velocity;

    aiFace aiFace;

    unsigned int i, j;

    for (i = 0; i < mesh->mNumVertices; ++i)
    {
        pos.x = mesh->mVertices[i].x;
        pos.y = mesh->mVertices[i].y;
        pos.z = mesh->mVertices[i].z;

        normal.x = mesh->mNormals[i].x;
        normal.y = mesh->mNormals[i].y;
        normal.z = mesh->mNormals[i].z;

        velocity.x = mesh->mNormals[i].x / 10000;
        velocity.y = mesh->mNormals[i].y / 10000;
        velocity.z = mesh->mNormals[i].z / 10000;

        vertex.Position = pos;
        vertex.Normal = normal;
        vertex.Velocity = velocity;

        model.pushVertex(vertex);
    }
    for (i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace = mesh->mFaces[i];
        for (j = 0; j < aiFace.mNumIndices; ++j)
            model.pushIndex(aiFace.mIndices[j]);
    }
    for (i = 0; i < mesh->mNumFaces; i += 2)
    {
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

        model.pushFace(face);
    }

    return Mesh(model);
}