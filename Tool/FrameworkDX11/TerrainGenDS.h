#pragma once
#include <random>
#include <cmath>
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>

class TerrainGenDS
{
public:
    int _size;
    int _max;
    std::vector<std::vector<float>> _map;
    float _roughness;

    TerrainGenDS() {};
    TerrainGenDS(int detail)
    {
        _size = 1 + (int)pow(2, detail);
        _max = _size - 1;
        _map = std::vector<std::vector<float>>(_size, std::vector<float>(_size));
    }

    void Generate(float roughness)
    {
        _roughness = roughness;
        SetMap(0, 0, _max);
        SetMap(0, _max, _max / 2);
        SetMap(_max, 0, _max / 2);
        SetMap(_max, _max, 0);
        divide(_max);
        for (int x = 0; x < _size; x++)
        {
            for (int y = 0; y < _size; y++)
            {
                //OutputDebugStringA((std::to_string(x) + " ").c_str());
                //OutputDebugStringA((std::to_string(y) + "\n").c_str());

                //OutputDebugStringA((std::to_string(_map[x][y]) + " ").c_str());
                //_map[x, y] = _map[x, y] - _size / 2;
            }
            //OutputDebugStringA((std::to_string(000) + "\n").c_str());

        }
    }

    void SetMap(int x, int y, float value)
    {
        _map[x][y] = value;
    }

    float GetMap(int x, int y)
    {
        if (x < 0 || x > _max || y < 0 || y > _max) { return -1; }
        //OutputDebugStringA((std::to_string(_map[x][y]) + " ").c_str());
        return _map[x][y];
    }

    void divide(int jumpSize)
    {
        std::random_device rd;  // Seed source
        std::mt19937 gen(rd()); // Mersenne Twister generator
        std::uniform_real_distribution<> dis(0.0, 1.0); // Uniform distribution between 0 and 
        
        int half = jumpSize / 2;
        if (half < 1) { return; }
        float offset = _roughness * jumpSize;

        for (int x = half; x < _max; x += jumpSize) {
            for (int y = half; y < _max; y += jumpSize) {
                Square(x, y, half, dis(gen) * offset * 2 - offset);
            }
        }

        int choiceNumber = 0;
        for (int x = 0; x <= _max; x += half) {
            if (choiceNumber == 0)
            {
                for (int y = half; y < _max; y += jumpSize)
                {
                    Diamond(x, y, half, dis(gen) * offset * 2 - offset);
                }
                choiceNumber = 1;
            }
            else if (choiceNumber == 1)
            {
                for (int y = 0; y <= _max; y += jumpSize)
                {
                    Diamond(x, y, half, dis(gen) * offset * 2 - offset);
                }
                choiceNumber = 2;
            }
            else if (choiceNumber == 2)
            {
                for (int y = half; y <= _max; y += jumpSize)
                {
                    Diamond(x, y, half, dis(gen) * offset * 2 - offset);
                }
                choiceNumber = 1;
            }
        }
        divide(jumpSize / 2);
    }

    void Square(int x, int y, int half, float offset)
    {
        std::vector<float> list = {
                GetMap(x - half, y - half),
                    GetMap(x - half, y + half),
                    GetMap(x + half, y + half),
                    GetMap(x + half, y - half)
        };
        float average = averageList(list);
        //OutputDebugStringA((std::to_string(average) + " ").c_str());
        //OutputDebugStringA((std::to_string(offset) + " ").c_str());
        //OutputDebugStringA((std::to_string(_map[x, y]) + " " ).c_str());
        SetMap(x, y, (average + offset));
        //OutputDebugStringA((std::to_string(_map[x, y]) + " ").c_str());
    }

    void Diamond(int x, int y, int half, float offset)
    {
        std::vector<float> list = {
            GetMap(x, y - half),
                GetMap(x, y + half),
                GetMap(x + half, y),
                GetMap(x - half, y)
        };
        float average = averageList(list);
        SetMap(x, y, (average + offset));
    }

    float averageList(std::vector<float> array)
    {
        float total = 0;
        int length = 0;

        for(int i = 0; i < size(array); i++)
        {
            if (array[i] != -1)
            {
                total += array[i];
                length++;
            }
        }
        return (total / length);
    }
};

