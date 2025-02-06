#pragma once
#include <DirectXMath.h>
#include <Windows.h>
#include <DirectXCollision.h>
#include <cmath>
#include <random>

using namespace DirectX;

class HydraulicErosion
{
private:
    XMFLOAT2 dim;
    std::vector<std::vector<float>> erosionMap;

    XMVECTOR surNormCalc(float a, float x, float y, float z)
    {
        XMFLOAT3 vec1(a, a, a);
        XMFLOAT3 vec2(x, y, z);
        return XMLoadFloat3(&vec1) * XMVector3Normalize(XMLoadFloat3(&vec2));
    }

    XMFLOAT3 surfaceNormal(int i, int j)
    {
        XMVECTOR n = surNormCalc(0.15F, (float)scale * (erosionMap[i - 1][j] - erosionMap[i][j]), 1.0f, 0.0f);
        n += surNormCalc(0.15F, (float)scale * (erosionMap[i - 1][j] - erosionMap[i][j]), 1.0F, 0.0F);
        n += surNormCalc(0.15F, 0.0F, 1.0F, (float)scale * (erosionMap[i][j] - erosionMap[i][j + 1]));
        n += surNormCalc(0.15F, 0.0F, 1.0F, (float)scale * (erosionMap[i][j - 1] - erosionMap[i][j]));

        n += surNormCalc(0.1F, (float)scale * (erosionMap[i][j] - erosionMap[i + 1][j + 1]) / sqrt(2.0F), sqrt(2.0f), (float)scale * (erosionMap[i][j] - erosionMap[i + 1][j + 1]) / sqrt(2.0F));
        n += surNormCalc(0.1F, (float)scale * (erosionMap[i][j] - erosionMap[i + 1][j - 1]) / sqrt(2.0F), sqrt(2.0F), (float)scale * (erosionMap[i][j] - erosionMap[i + 1][j - 1]) / sqrt(2.0F));
        n += surNormCalc(0.1F, (float)scale * (erosionMap[i][j] - erosionMap[i - 1][j + 1]) / sqrt(2.0F), sqrt(2.0F), (float)scale * (erosionMap[i][j] - erosionMap[i - 1][j + 1]) / sqrt(2.0F));
        n += surNormCalc(0.1F, (float)scale * (erosionMap[i][j] - erosionMap[i - 1][j - 1]) / sqrt(2.0F), sqrt(2.0F), (float)scale * (erosionMap[i][j] - erosionMap[i - 1][j - 1]) / sqrt(2.0F));

        return XMFLOAT3(XMVectorGetX(n), XMVectorGetY(n), XMVectorGetZ(n));
    }

    struct Particle
    {
        Particle(XMFLOAT2 _pos) { pos = _pos; }

        XMFLOAT2 pos;
        XMFLOAT2 speed = XMFLOAT2(0, 0);

        float volume = 1.0f;
        float sediment = 0.0f;
    };
public:
    double scale = 60.0;
    float dt = 1.2f;
    float density = 1.0f;
    float evapRate = 0.001f;
    float depositionRate = 0.1f;
    float minVol = 0.01f;
    float friction = 0.05f;
    HydraulicErosion() {};
    std::vector<std::vector<float>> Erode(std::vector<std::vector<float>> erosionmap, int _hydroCycles, int size)
    {
        dim = XMFLOAT2(size, size);

        std::random_device rd;  // Seed source
        std::mt19937 gen(rd()); // Mersenne Twister generator
        std::uniform_real_distribution<> dis(0.0, dim.x); // Uniform distribution between 0 and 

        erosionMap = erosionmap;

        for (int i = 0; i < _hydroCycles; i++)
        {
            XMFLOAT2 newpos = XMFLOAT2(dis(gen), dis(gen));
            Particle* drop = new Particle(newpos);

            while (drop->volume > minVol)
            {
                XMFLOAT2 ipos = drop->pos;
                if (ipos.x >= dim.x - 1 || ipos.y >= dim.y - 1 || ipos.y <= 1 || ipos.x <= 1) { break; }
                XMFLOAT3 n = surfaceNormal((int)ipos.x, (int)ipos.y);

                XMFLOAT2 temp = XMFLOAT2((dt * n.x) / (drop->volume * density), (dt * n.z) / (drop->volume * density));
                drop->speed = XMFLOAT2(drop->speed.x + temp.x, drop->speed.y + temp.y);
                drop->pos = XMFLOAT2(drop->pos.x + (dt * drop->speed.x), drop->pos.y + (dt * drop->speed.y));
                drop->speed = XMFLOAT2(drop->speed.x * (1.0F - dt * friction), drop->speed.y * (1.0F - dt * friction));

                if (!(drop->pos.x > 0 && drop->pos.y > 0) || !(drop->pos.x < dim.x && drop->pos.y < dim.y))
                {
                    break;
                }

                float maxsediment = drop->volume * sqrt(drop->speed.x * drop->speed.x * drop->speed.y * drop->speed.y) * (erosionMap[(int)ipos.x][ (int)ipos.y] - erosionMap[(int)drop->pos.x][ (int)drop->pos.y]);
                if (maxsediment < 0.0) { maxsediment = 0.0F; }
                float sdiff = maxsediment - drop->sediment;

                drop->sediment += dt * depositionRate * sdiff;
                erosionMap[(int)ipos.x][ (int)ipos.y] -= dt * drop->volume * depositionRate * sdiff;

                drop->volume *= (1.0F - dt * evapRate);
            }
            delete drop;
        };
        
        return erosionMap;

    };
};