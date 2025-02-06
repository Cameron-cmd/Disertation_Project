#pragma once
#include <DirectXMath.h>
#include <Windows.h>
#include <DirectXCollision.h>
#include <cmath>
#include <random>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>

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
        // Simplified for brevity
        return XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    struct Particle
    {
        Particle(XMFLOAT2 _pos) { pos = _pos; }
        XMFLOAT2 pos;
        XMFLOAT2 speed = XMFLOAT2(0, 0);
        float volume = 1.0f;
        float sediment = 0.0f;
    };

    // Using unique_ptr to manage mutexes properly.
    std::vector<std::mutex> mutexMap;
    inline int GetMutexIndex(int i, int j) const
    {
        return i * (int)dim.x + j;
    }


public:
    double scale = 60.0;
    float dt = 1.2f;
    float density = 1.0f;
    float evapRate = 0.001f;
    float depositionRate = 0.1f;
    float minVol = 0.01f;
    float friction = 0.05f;

    HydraulicErosion() {}

    // Lock surrounding cells
    void LockSurrounding(XMINT2 pos)
    {
        static std::mutex lockMutex;
        lockMutex.lock();
        std::lock(
            mutexMap[GetMutexIndex(pos.x, pos.y)],
            mutexMap[GetMutexIndex(pos.x - 1, pos.y)],
            mutexMap[GetMutexIndex(pos.x - 1, pos.y - 1)],
            mutexMap[GetMutexIndex(pos.x - 1, pos.y + 1)],
            mutexMap[GetMutexIndex(pos.x + 1, pos.y)],
            mutexMap[GetMutexIndex(pos.x + 1, pos.y - 1)],
            mutexMap[GetMutexIndex(pos.x + 1, pos.y + 1)],
            mutexMap[GetMutexIndex(pos.x, pos.y - 1)],
            mutexMap[GetMutexIndex(pos.x, pos.y + 1)]
        );
        lockMutex.unlock();
    }

    void UnlockSurrounding(XMINT2 pos)
    {
        static std::mutex unlockMutex;
        unlockMutex.lock();
        // Unlock in the reverse order to ensure consistency
        mutexMap[GetMutexIndex(pos.x, pos.y)].unlock();
        mutexMap[GetMutexIndex(pos.x - 1, pos.y)].unlock();
        mutexMap[GetMutexIndex(pos.x - 1, pos.y - 1)].unlock();
        mutexMap[GetMutexIndex(pos.x - 1, pos.y + 1)].unlock();
        mutexMap[GetMutexIndex(pos.x + 1, pos.y)].unlock();
        mutexMap[GetMutexIndex(pos.x + 1, pos.y - 1)].unlock();
        mutexMap[GetMutexIndex(pos.x + 1, pos.y + 1)].unlock();
        mutexMap[GetMutexIndex(pos.x, pos.y - 1)].unlock();
        mutexMap[GetMutexIndex(pos.x, pos.y + 1)].unlock();
        unlockMutex.unlock();

    }

    void ErodeThread(int _hydroCycles, std::mt19937 gen)
    {
        std::uniform_real_distribution<> dis(0.0, dim.x); // Uniform distribution between 0 and 

        for (int i = 0; i < _hydroCycles; i++)
        {
            XMFLOAT2 newpos = XMFLOAT2(dis(gen), dis(gen));
            Particle* drop = new Particle(newpos);

            while (drop->volume > minVol)
            {
                XMFLOAT2 ipos = drop->pos;
                LockSurrounding(XMINT2(ipos.x, ipos.y));
                if (ipos.x >= dim.x - 1 || ipos.y >= dim.y - 1 || ipos.y <= 1 || ipos.x <= 1) { break; }
                XMFLOAT3 n = surfaceNormal((int)ipos.x, (int)ipos.y);

                XMFLOAT2 temp = XMFLOAT2((dt * n.x) / (drop->volume * density), (dt * n.z) / (drop->volume * density));
                drop->speed = XMFLOAT2(drop->speed.x + temp.x, drop->speed.y + temp.y);
                drop->pos = XMFLOAT2(drop->pos.x + (dt * drop->speed.x), drop->pos.y + (dt * drop->speed.y));
                drop->speed = XMFLOAT2(drop->speed.x * (1.0F - dt * friction), drop->speed.y * (1.0F - dt * friction));

                if (!(drop->pos.x > 0 && drop->pos.y > 0) || !(drop->pos.x < dim.x && drop->pos.y < dim.y))
                {
                    UnlockSurrounding(XMINT2(ipos.x, ipos.y));
                    break;
                }

                float maxsediment = drop->volume * sqrt(drop->speed.x * drop->speed.x * drop->speed.y * drop->speed.y) * (erosionMap[(int)ipos.x][(int)ipos.y] - erosionMap[(int)drop->pos.x][(int)drop->pos.y]);
                if (maxsediment < 0.0) { maxsediment = 0.0F; }
                float sdiff = maxsediment - drop->sediment;

                drop->sediment += dt * depositionRate * sdiff;
                erosionMap[(int)ipos.x][(int)ipos.y] -= dt * drop->volume * depositionRate * sdiff;

                drop->volume *= (1.0F - dt * evapRate);
                UnlockSurrounding(XMINT2(ipos.x, ipos.y));
            }
            delete drop;
        };
    }

    std::vector<std::vector<float>> Erode(std::vector<std::vector<float>> erosionmap, int _hydroCycles, int size)
    {
        mutexMap = std::vector<std::mutex>(size * size);
        dim = XMFLOAT2(size, size);
        std::vector<std::thread> threads;
        erosionMap = erosionmap;

        std::random_device rd;  // Seed source
        std::mt19937 gen(rd()); // Mersenne Twister generator

        int numThreads = std::thread::hardware_concurrency() / 2;
        int cyclesPerThread = _hydroCycles / numThreads;
        HydraulicErosion erosionInstance;

        for (int i = 0; i < numThreads; i++)
        {
            threads.push_back(std::thread(&HydraulicErosion::ErodeThread, &erosionInstance, cyclesPerThread, gen));
        }

        for (auto& t : threads)
        {
            t.join();
        }

        return erosionMap;
    }
};
