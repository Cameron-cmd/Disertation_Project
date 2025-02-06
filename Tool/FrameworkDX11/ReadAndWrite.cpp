//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"

#include "tiny_gltf.h"
#include "ReadAndWrite.h"

void ReadAndWrite::ExportGLB(SimpleVertex* vertices, DWORD* indices, int verticesCount, int indicesCount, std::string fileLocation, std::string fileName)
{
    tinygltf::Model model;

    std::vector<float> positions(verticesCount*3, 0);
    std::vector<float> colours(verticesCount * 3, 0);
    std::vector<float> normals(verticesCount * 3, 0);

    XMFLOAT3 maxPos = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    XMFLOAT3 minPos = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);

    XMFLOAT3 normalMin = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
    XMFLOAT3 normalMax = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    XMFLOAT3 colorMin = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
    XMFLOAT3 colorMax = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (int i = 0; i < verticesCount; ++i)
    {

        positions[i*3] = vertices[i].Pos.x;
        positions[i*3+1] = vertices[i].Pos.y;
        positions[i*3+2] = vertices[i].Pos.z;

        if (vertices[i].Pos.x < minPos.x) minPos.x = vertices[i].Pos.x;
        if (vertices[i].Pos.x > maxPos.x) maxPos.x = vertices[i].Pos.x;
        if (vertices[i].Pos.y < minPos.y) minPos.y = vertices[i].Pos.y;
        if (vertices[i].Pos.y > maxPos.y) maxPos.y = vertices[i].Pos.y;
        if (vertices[i].Pos.z < minPos.z) minPos.z = vertices[i].Pos.z;
        if (vertices[i].Pos.z > maxPos.z) maxPos.z = vertices[i].Pos.z;

        normals[i*3] = vertices[i].Normal.x;
        normals[i*3+1] = vertices[i].Normal.y;
        normals[i*3+2] = vertices[i].Normal.z;

        if (vertices[i].Normal.x < normalMin.x) normalMin.x = vertices[i].Normal.x;
        if (vertices[i].Normal.x > normalMax.x) normalMax.x = vertices[i].Normal.x;
        if (vertices[i].Normal.y < normalMin.y) normalMin.y = vertices[i].Normal.y;
        if (vertices[i].Normal.y > normalMax.y) normalMax.y = vertices[i].Normal.y;
        if (vertices[i].Normal.z < normalMin.z) normalMin.z = vertices[i].Normal.z;
        if (vertices[i].Normal.z > normalMax.z) normalMax.z = vertices[i].Normal.z;

        colours[i*3] = vertices[i].Colour.x /** 255.0f*/;
        colours[i*3+1] = vertices[i].Colour.y /** 255.0f*/;
        colours[i*3+2] = vertices[i].Colour.z /** 255.0f*/;

        if (vertices[i].Colour.x < colorMin.x) colorMin.x = vertices[i].Colour.x;
        if (vertices[i].Colour.x > colorMax.x) colorMax.x = vertices[i].Colour.x;
        if (vertices[i].Colour.y < colorMin.y) colorMin.y = vertices[i].Colour.y;
        if (vertices[i].Colour.y > colorMax.y) colorMax.y = vertices[i].Colour.y;
        if (vertices[i].Colour.z < colorMin.z) colorMin.z = vertices[i].Colour.z;
        if (vertices[i].Colour.z > colorMax.z) colorMax.z = vertices[i].Colour.z;
    }
    std::vector<unsigned int> indexData(indices, indices + indicesCount);

    // Single buffer for all data
    tinygltf::Buffer buffer;
    buffer.data.insert(buffer.data.end(), reinterpret_cast<uint8_t*>(positions.data()), reinterpret_cast<uint8_t*>(positions.data()) + positions.size() * sizeof(float));
    buffer.data.insert(buffer.data.end(), reinterpret_cast<uint8_t*>(normals.data()), reinterpret_cast<uint8_t*>(normals.data()) + normals.size() * sizeof(float));
    buffer.data.insert(buffer.data.end(), reinterpret_cast<uint8_t*>(colours.data()), reinterpret_cast<uint8_t*>(colours.data()) + colours.size() * sizeof(float));
    buffer.data.insert(buffer.data.end(), reinterpret_cast<uint8_t*>(indexData.data()), reinterpret_cast<uint8_t*>(indexData.data()) + indexData.size() * sizeof(unsigned int));
    model.buffers.push_back(buffer);

    // Buffer views
    size_t offset = 0;
    auto createBufferView = [&](size_t size, int target) {
        tinygltf::BufferView view;
        view.buffer = 0;
        view.byteOffset = offset;
        view.byteLength = size;
        view.target = target;
        model.bufferViews.push_back(view);
        offset += size;
        return model.bufferViews.size() - 1;
        };

    int positionView = createBufferView(positions.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
    int normalView = createBufferView(normals.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
    int colorView = createBufferView(colours.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
    int indexView = createBufferView(indexData.size() * sizeof(unsigned int), TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);

    auto createAccessor = [&](int bufferView, int componentType, int count, int type, const XMFLOAT3& minPos, const XMFLOAT3& maxPos) {
        tinygltf::Accessor accessor;
        accessor.bufferView = bufferView;
        accessor.byteOffset = 0;
        accessor.componentType = componentType;
        accessor.count = count;
        accessor.type = type;

        // Only set min/max for vector types (e.g., TINYGLTF_TYPE_VEC3)
        if (type == TINYGLTF_TYPE_VEC3) {
            accessor.minValues = { minPos.x, minPos.y, minPos.z };
            accessor.maxValues = { maxPos.x, maxPos.y, maxPos.z };
        }

        model.accessors.push_back(accessor);
        return model.accessors.size() - 1;
        };

    // Calling the accessor creation with the proper min/max
    int positionAccessor = createAccessor(positionView, TINYGLTF_COMPONENT_TYPE_FLOAT, verticesCount, TINYGLTF_TYPE_VEC3, minPos, maxPos);
    int normalAccessor = createAccessor(normalView, TINYGLTF_COMPONENT_TYPE_FLOAT, verticesCount, TINYGLTF_TYPE_VEC3, normalMin, normalMax);
    int colorAccessor = createAccessor(colorView, TINYGLTF_COMPONENT_TYPE_FLOAT, verticesCount, TINYGLTF_TYPE_VEC3, colorMin, colorMax);
    int indexAccessor = createAccessor(indexView, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, indexData.size(), TINYGLTF_TYPE_SCALAR, {}, {});

    tinygltf::Material material;
    material.pbrMetallicRoughness.baseColorFactor = { 1.0, 1.0, 1.0, 1.0 }; // White base color
    material.pbrMetallicRoughness.metallicFactor = 0.0;                  // Non-metallic
    material.pbrMetallicRoughness.roughnessFactor = 1.0;                 // Fully rough (non-shiny)
    model.materials.push_back(material);

    // Mesh setup
    tinygltf::Primitive primitive;
    primitive.attributes["POSITION"] = positionAccessor;
    primitive.attributes["NORMAL"] = normalAccessor;
    primitive.attributes["COLOR_0"] = colorAccessor;
    primitive.indices = indexAccessor;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    primitive.material = 0;


    tinygltf::Mesh mesh;
    mesh.primitives.push_back(primitive);
    model.meshes.push_back(mesh);

    tinygltf::Node node;
    node.mesh = 0;
    model.nodes.push_back(node);

    tinygltf::Scene scene;
    scene.nodes.push_back(0);
    model.scenes.push_back(scene);

    model.defaultScene = 0;

    // Write GLB
    tinygltf::TinyGLTF writer;
    if (!writer.WriteGltfSceneToFile(&model, fileLocation + "\\" + fileName + ".glb", true, true, true, true)) {
        std::cerr << "Failed to write GLB file!" << std::endl;
    }
}

void ReadAndWrite::ReadGLB(const std::string& fileName, DrawableGameObject* gameObject) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    // Load the GLB model
    std::string err;
    std::string warn;
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, fileName);

    if (!ret) {
        std::cerr << "Failed to load GLB file: " << err << std::endl;
        return;
    }

    // Check if the model has meshes
    if (model.meshes.empty()) {
        std::cerr << "No meshes found in the GLB file." << std::endl;
        return;
    }

    // Extract the first mesh and its primitive
    const tinygltf::Mesh& mesh = model.meshes[0];
    const tinygltf::Primitive& primitive = mesh.primitives[0];

    // Get the accessors for position, color, and index using find
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) {
        std::cerr << "'POSITION' attribute not found in primitive" << std::endl;
        return;
    }
    int positionAccessorIndex = posIt->second; // Accessor index for "POSITION"

    auto colorIt = primitive.attributes.find("COLOR_0");
    if (colorIt == primitive.attributes.end()) {
        std::cerr << "'COLOR_0' attribute not found in primitive" << std::endl;
        return;
    }
    int colorAccessorIndex = colorIt->second; // Accessor index for "COLOR_0"

    // Get the index accessor (indices for faces or triangles)
    int indexAccessorIndex = primitive.indices; // This is already an index to the accessor

    // Retrieve the accessors for positions, colors, and indices
    const tinygltf::Accessor& positionAccessor = model.accessors[positionAccessorIndex];
    const tinygltf::Accessor& colorAccessor = model.accessors[colorAccessorIndex];
    const tinygltf::Accessor& indexAccessor = model.accessors[indexAccessorIndex];

    // Get the vertex and index count
    size_t vertexCount = positionAccessor.count;
    size_t indexCount = indexAccessor.count;

    std::cout << "Vertex count: " << vertexCount << std::endl;
    std::cout << "Index count: " << indexCount << std::endl;
    int size = sqrt(vertexCount);

    // Retrieve the position and color data (positions are float3, colors are float3)
    std::vector<std::vector<float>> positionData(size, std::vector<float>(size, 0));  // Store y, x, z
    std::vector<std::vector<XMFLOAT3>> colorData(size, std::vector<XMFLOAT3>(size, XMFLOAT3(0.2f, 0.2f, 0.2f)));

    // Reading position data
    const float* positionPtr = reinterpret_cast<const float*>(&model.buffers[positionAccessor.bufferView].data[positionAccessor.byteOffset]);
    for (size_t i = 0; i < vertexCount; ++i) {
        // Position as a vector of float {y, x, z}
        positionData[positionPtr[i * 3]][positionPtr[i * 3 + 2]] = positionPtr[i * 3 + 1];
    };

    if (colorAccessor.bufferView < 0 || colorAccessor.bufferView >= model.bufferViews.size()) {
        std::cerr << "Invalid buffer view index in colorAccessor!" << std::endl;
        return;
    }

    const tinygltf::BufferView& colorBufferView = model.bufferViews[colorAccessor.bufferView];

    if (colorBufferView.buffer < 0 || colorBufferView.buffer >= model.buffers.size()) {
        std::cerr << "Invalid buffer index in colorBufferView!" << std::endl;
        return;
    }

    const tinygltf::Buffer& colorBuffer = model.buffers[colorBufferView.buffer];

    // Check if the byteOffset is valid
    size_t totalOffset = colorBufferView.byteOffset + colorAccessor.byteOffset;
    if (totalOffset < 0 || totalOffset >= colorBuffer.data.size()) {
        std::cerr << "Invalid byteOffset in colorAccessor!" << std::endl;
        return;
    }

    if (colorAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        std::cerr << "Color data is not stored as floats!" << std::endl;
        return;
    }

    if (colorAccessor.type != TINYGLTF_TYPE_VEC3 && colorAccessor.type != TINYGLTF_TYPE_VEC4) {
        std::cerr << "Color data is not stored as VEC3 or VEC4!" << std::endl;
        return;
    }

    // Reading color data (RGB values)
    int x = 0;
    int z = 0;
    const float* colorPtr = reinterpret_cast<const float*>(&colorBuffer.data[totalOffset]);
    for (size_t i = 0; i < vertexCount; ++i) {
        XMFLOAT3 color(
            colorPtr[i * 3],     // R
            colorPtr[i * 3 + 1], // G
            colorPtr[i * 3 + 2]  // B
        );
        colorData[x][z] = color; // Store RGB values as XMFLOAT3
        z++;
        if (z >= size)
        {
            x++;
            z = 0;
        }
    }

    // Now, positionData contains {y, x, z} positions, and colorData contains RGB values.
    std::cout << "Position Data (y, x, z) and Color Data (RGB) loaded from GLB file." << std::endl;

    // Debugging printout
    for (int x = 0; x < size; x++) {
        for (int z = 0; z < size; z++) {
            std::cout << "Position: (y: " << x << ", x: " << positionData[x][z] << ", z: " << z << "), "
                << "Color: (" << colorData[x][z].x << ", " << colorData[x][z].y << ", " << colorData[x][z].z << ")\n";
        }
    }
    gameObject->loadTerrain(&positionData, &colorData, sqrt(vertexCount));
}