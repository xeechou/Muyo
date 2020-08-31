#include "Geometry.h"
#include <tiny_obj_loader.h>
#include <tiny_gltf.h>
std::unique_ptr<Geometry> loadObj(const std::string& path, glm::mat4 mTransformation)
{
    struct TinyObjInfo
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
    };
    TinyObjInfo objInfo;
    std::string err;
    bool ret = tinyobj::LoadObj(&(objInfo.attrib), &(objInfo.shapes),
                                &(objInfo.materials), &err, path.c_str());
    if (!ret)
    {
        std::cerr << err << std::endl;
    }
    else
    {
        std::cout << "Num verts :" << objInfo.attrib.vertices.size() / 3
                  << std::endl;
        std::cout << "Num Normals :" << objInfo.attrib.normals.size() / 3
                  << std::endl;
        std::cout << "Num TexCoords :" << objInfo.attrib.texcoords.size() / 2
                  << std::endl;

        // Print out mesh summery
        for (size_t i = 0; i < objInfo.shapes.size(); i++)
        {
            std::cout << "Shape " << i << ": " << std::endl;
            tinyobj::shape_t &shape = objInfo.shapes[i];
            std::cout << "\t" << shape.mesh.indices.size() << " indices\n";

        }
    }

    std::vector<std::unique_ptr<Primitive>> primitives;
    const glm::mat4 mNormalTransformation = glm::transpose(glm::inverse(mTransformation));
    for (size_t i = 0; i < objInfo.shapes.size(); i++)
    {
        const auto &vIndices = objInfo.shapes[i].mesh.indices;
        size_t numVert = vIndices.size();

        std::vector<Vertex> vertices;
        vertices.reserve(numVert);
        for (const auto &meshIdx : vIndices)
        {
            glm::vec4 pos(objInfo.attrib.vertices[3 * meshIdx.vertex_index],
                          objInfo.attrib.vertices[3 * meshIdx.vertex_index + 1],
                          objInfo.attrib.vertices[3 * meshIdx.vertex_index + 2],
                          1.0);
            pos = pos * mTransformation;

            glm::vec4 normal(
                objInfo.attrib.normals[3 * meshIdx.normal_index],
                objInfo.attrib.normals[3 * meshIdx.normal_index + 1],
                objInfo.attrib.normals[3 * meshIdx.normal_index + 2], 0.0);

            normal = normal * mNormalTransformation;

            vertices.emplace_back(Vertex(
                {{pos.x, pos.y, pos.z},
                 {normal.x, normal.y, normal.z},
                 {objInfo.attrib.texcoords[2 * meshIdx.texcoord_index],
                  objInfo.attrib.texcoords[2 * meshIdx.texcoord_index + 1],
                  0}}));
        }
        std::vector<Index> indices;
        indices.reserve(objInfo.shapes[i].mesh.indices.size());
        for (size_t index = 0; index < objInfo.shapes[i].mesh.indices.size(); index++)
        {
            indices.push_back(index);
        }

        primitives.emplace_back(std::make_unique<Primitive>(vertices, indices));
    }
    return std::make_unique<Geometry>(primitives);
}
std::unique_ptr<Geometry> getQuad()
{
    static const std::vector<Vertex> VERTICES = {
        {{-1.0f, -1.0f, 0.0}, {0.0, 0.0, 1.0}, {0.0f, 0.0f, 0.0f}},
        {{1.0f, -1.0f, 0.0}, {0.0, 0.0, 1.0}, {1.0f, 0.0f, 0.0f}},
        {{1.0f, 1.0f, 0.0}, {0.0, 0.0, 1.0}, {1.0f, 1.0f, 1.0f}},
        {{-1.0f, 1.0f, 0.0}, {0.0, 0.0, 1.0}, {0.0f, 1.0f, 1.0f}}};

    static const std::vector<uint32_t> INDICES = {0, 1, 2, 2, 3, 0};
    return std::make_unique<Geometry>(
        std::make_unique<Primitive>(VERTICES, INDICES));
}

std::unique_ptr<Geometry> getSkybox()
{
    return loadObj("assets/cube.obj");
}

std::unique_ptr<Geometry> loadGLTF(const std::string& path, glm::mat4 mTransformation)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
    std::cout<<"Loading GLTF===\n";
    std::cout<<"Num meshes: "<<model.meshes.size()<<std::endl;
    const std::vector<std::string> READ_VALUES = {
        "NORMAL", "POSITION", "TEXCOORD_0"
    };
    for (const auto &mesh : model.meshes)
    {
        std::cout << "Name: " << mesh.name << std::endl;
        // List attributes
        std::cout << "Num primitives: " << mesh.primitives.size() << std::endl;
        for (const auto &primitive : mesh.primitives)
        {
            for (const std::string &attribKey : READ_VALUES)
            {
                std::cout << primitive.attributes.at(attribKey)<<std::endl;
            }
        }
    }

    // bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); //
    // for binary glTF(.glb)

    if (!warn.empty())
    {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty())
    {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret)
    {
        printf("Failed to parse glTF\n");
        //return -1;
    }
    return nullptr;
}
