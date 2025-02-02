    #pragma once
    #include <iostream>
    #include <vector>
    #include <DirectXCollision.h>
    #include <string>
    #include <Windows.h>
#include "DrawableGameObject.h"
    using namespace DirectX;



    class ReadAndWrite
    {
    public:
        void ExportGLB(SimpleVertex* vertices, DWORD* indices, int verticesCount, int indicesCount, std::string fileLocation, std::string fileName);

        void ReadGLB(const std::string& fileName, DrawableGameObject* gameObject);
    private:

    };

