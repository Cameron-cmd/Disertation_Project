﻿//--------------------------------------------------------------------------------------
// File: main.cpp
//
// This application demonstrates animation using matrix transformations
//
// http://msdn.microsoft.com/en-us/library/windows/apps/ff729722.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define _XM_NO_INTRINSICS_

#include "main.h"
#include "constants.h"
#include "Camera.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "FastNoiseLite.h"
#include <random>
#include <thread>
#include <sstream>
#include "DirectXTex.h"
#include <filesystem>
#include <wincodec.h>

#include "nfd.h"

#include "ReadAndWrite.h"

ReadAndWrite GLB;
Camera* g_pCamera;

#define SliderWidth 200
#define UIWidthL 385
#define UIWidthR 300


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT		InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT		InitDevice();
HRESULT		InitRunTimeParameters();
HRESULT		InitWorld(int width, int height);
void		CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void		Render();



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11Device1*          g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;
ID3D11DeviceContext1*   g_pImmediateContext1 = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
IDXGISwapChain1*        g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D*        g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11VertexShader*     g_pVertexShader = nullptr;

ID3D11PixelShader*      g_pPixelShader = nullptr;

ID3D11InputLayout*      g_pVertexLayout = nullptr;

ID3D11Buffer*           g_pConstantBuffer = nullptr;

ID3D11Buffer*           g_pLightConstantBuffer = nullptr;


XMMATRIX                g_Projection;

int						g_viewWidth;
int						g_viewHeight;

DrawableGameObject		g_GameObject;

ImVec4                  light_colour = ImVec4(0.9f, 0.4f, 1.0f, 0.2f);
XMMATRIX                obj_rotation = XMMatrixRotationX(0);

// Terrain Values
// DiamondSquare
int                     t_detail = 9;
float                   t_roughness = 0.5;
int                     h_cycles = 200000;

float                   n_Exponent = 1.0f;

bool                    n_PerlinOn = true;
int                     n_PerlinOctaves = 1;
float                   n_PerlinWeight = 1.0f;
float                   n_PerlinFrequency = 0.01f;
bool                    n_SimplexOn = false;
int                     n_SimplexOctaves = 1;
float                   n_SimplexWeight = 1.0f;
float                   n_SimplexFrequency = 0.01f;
bool                    n_CellularOn = false;
int                     n_CellularOctaves = 1;
float                   n_CellularWeight = 1.0f;
float                   n_CellularFrequency = 0.01f;
int                     n_resolution = 256;
float                   n_height = 256;
float                   n_floor = 0;



uint32_t*               n_pixels;

FastNoiseLite           n_Perlin = FastNoiseLite();
FastNoiseLite           n_Simplex = FastNoiseLite();
FastNoiseLite           n_Cellular = FastNoiseLite();

ID3D11ShaderResourceView* n_texture = nullptr;
ID3D11Texture2D* n_frontTexture = nullptr;
ID3D11Texture2D* n_backTexture = nullptr;

int n_imageSize = 150;
string FileName;

float ScaleX;
float ScaleY;

bool normalDraw = true;
bool wireDraw = true;
bool wireframeEnabled = false;

int screenWidth;
int screenHeight;

ID3D11RasterizerState* rasterizerState;

float height = 0;
float width = 0;

bool advHydro = false;

float movement = 50.0f;

bool b_FlatBool = false;
bool b_FlattenDown = false;
bool b_FlattenUp = false;
int b_FlatSize = 5;
float b_MaxFlatDifference = 5.0f;
float b_SmoothnessFactor = 0.1f;

bool b_RaiseBool = false;
int b_RaiseSize = 5;
float b_RaiseRate = 0.1f;
float b_RaiseStrength = 1.0f;

bool b_LowerBool = false;
int b_LowerSize = 5;
float b_LowerRate = 0.1f;
float b_LowerStrength = 1.0f;

bool b_SmoothBool = false;
float b_SmoothRate = 0.1;
int b_SmoothSize = 5;

bool b_ColourBool = false;
int b_ColourSize = 5;
XMFLOAT3 b_Colour = XMFLOAT3(0.2, 0.4, 0.3);

bool FillTool = false;

float c_sensitivity = 1.0f;
float c_zoomSens = 0.35f;

string SaveLocation;
string LoadObjLocation;
string LoadImageLocation;

int TerrainGenerationMode = 1;

bool loadedModel = false;

POINTS mousePos;
RECT rect;
bool mouseDownR = false;
bool mouseDownL = false;

bool wasEdited = false;

int UndoRedoIndex = 0;
int maxURLength = 30;
std::vector<std::vector<XMFLOAT4>> UndoRedo(maxURLength, std::vector<XMFLOAT4>(0));
std::vector<std::vector<XMFLOAT3>> Colour(maxURLength, std::vector<XMFLOAT3>(0));

bool ColourPicker = false;

void RegenerateHeightMap();
void ClearAllUR()
{
    for (int i = 0; i < maxURLength - 1; i++)
    {
        UndoRedo[i].clear();
        Colour[i].clear();
    }
    UndoRedoIndex = 0;
}
void CheckVectorSpace()
{
    if (UndoRedoIndex == maxURLength)
    {
        Colour.erase(Colour.begin());
        UndoRedo.erase(UndoRedo.begin());
        Colour.push_back(std::vector<XMFLOAT3>(0));
        UndoRedo.push_back(std::vector<XMFLOAT4>(0));
        UndoRedoIndex = maxURLength-1;
    }
}
void ClearAfterVector()
{
    for (int i = UndoRedoIndex; i < maxURLength; i++)
    {
        UndoRedo[i].clear();
        Colour[i].clear();
    }
}
void AddPositionToUndo(XMFLOAT3 newPos, bool isColour, XMFLOAT3 colour)
{
    bool isContained = false;
    for (XMFLOAT4 pos : UndoRedo[UndoRedoIndex])
    {
        if (newPos.x == pos.x && newPos.z == pos.z)
        {
            isContained = true;
            break;
        }
    }
    if (!isContained && !isColour)
    {
        UndoRedo[UndoRedoIndex].push_back(XMFLOAT4(newPos.x, newPos.y, newPos.z, 0));
    }
    else if (!isContained && isColour)
    {
        UndoRedo[UndoRedoIndex].push_back(XMFLOAT4(newPos.x, 0, newPos.z, 1));
        Colour[UndoRedoIndex].push_back(colour);
    }
    wasEdited = true;
}
void IncrementUndoRedo(int increment)
{
    UndoRedoIndex += increment;
}
void Undo()
{
    if (UndoRedoIndex <= 0) {
        UndoRedoIndex = 0;
        return;
    }
    int ColourIndex = 0;
    UndoRedo[UndoRedoIndex].clear();
    Colour[UndoRedoIndex].clear();
    for (XMFLOAT4 pos : UndoRedo[UndoRedoIndex - 1])
    {
        if (pos.w == 0)
        {
            UndoRedo[UndoRedoIndex].push_back(XMFLOAT4(pos.x, g_GameObject.GetHeight(pos.x, pos.z), pos.z, 0));
            g_GameObject.SetHeight(pos.x, pos.z, pos.y);
        }
        else
        {
            UndoRedo[UndoRedoIndex].push_back(XMFLOAT4(pos.x, 0, pos.z, 1));
            Colour[UndoRedoIndex].push_back(g_GameObject.GetColour(pos.x, pos.z));
            g_GameObject.SetColour(pos.x, pos.z, Colour[UndoRedoIndex - 1][ColourIndex]);
            ColourIndex++;
        }
    }
    UndoRedoIndex--;
    RegenerateHeightMap();

}
void Redo()
{
    if (UndoRedoIndex >= maxURLength-1) {
        UndoRedoIndex = maxURLength - 1;
        return;
    }
    if (UndoRedo[UndoRedoIndex + 1].size() == 0) { return; }
    UndoRedo[UndoRedoIndex].clear();
    Colour[UndoRedoIndex].clear();

    int ColourIndex = 0;
    for (XMFLOAT4 pos : UndoRedo[UndoRedoIndex + 1])
    {
        if (pos.w == 0)
        {
            UndoRedo[UndoRedoIndex].push_back(XMFLOAT4(pos.x, g_GameObject.GetHeight(pos.x, pos.z), pos.z, 0));
            g_GameObject.SetHeight(pos.x, pos.z, pos.y);
        }
        else {
            UndoRedo[UndoRedoIndex].push_back(XMFLOAT4(pos.x, 0, pos.z, 1));
            Colour[UndoRedoIndex].push_back(g_GameObject.GetColour(pos.x, pos.z));
            g_GameObject.SetColour(pos.x, pos.z, Colour[UndoRedoIndex+1][ColourIndex]);
            ColourIndex++;
        }
    }
    UndoRedoIndex++;
    RegenerateHeightMap();
}
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"lWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);
    screenWidth = desktop.right;
    screenHeight = desktop.bottom;

    float aspectRatio = (float)screenWidth / (float)screenHeight;
    const float targetAspectRatio = 16.0f / 9.0f;

    if (aspectRatio > targetAspectRatio)
    {
        // Screen is too wide → Adjust width to maintain 16:9
        screenWidth = (int)(screenHeight * targetAspectRatio);
    }
    else if (aspectRatio < targetAspectRatio)
    {
        // Screen is too tall → Adjust height to maintain 16:9
        screenHeight = (int)(screenWidth / targetAspectRatio);
    }

    // Apply clamped resolution
    g_hInst = hInstance;
    RECT rc = { 0, 0, screenWidth, screenHeight };

    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow(L"lWindowClass", L"Terrain Generation Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, SW_MAXIMIZE);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile( szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob );
    if( FAILED(hr) )
    {
        if( pErrorBlob )
        {
            OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
            pErrorBlob->Release();
        }
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}

float RatioValueConverter(float old_min, float old_max, float new_min, float new_max, float value)
{
    return (((value - old_min) / (old_max - old_min)) * (new_max - new_min) + new_min);
}

int RandomSeed()
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dis(1, 10000);
    return dis(rng);
}

float GenerateNoiseValue(FastNoiseLite* noise, int octaves, int x, int y)
{
    float colour = 0;
    float weight = 1.0f;
    int scale = 1.0f;
    float totalDivide = 0;
    for (int i = 0; i < octaves; i++) {
        totalDivide += weight;
        weight *= 0.5f;
    }
    weight = 1.0f;
    for (int i = 0; i < octaves; i++) {
        colour += weight * noise->GetNoise((float)x * (float)scale , (float)y * (float)scale);
        weight = weight * 0.5;
        scale = scale * 2;
    }
    return colour / totalDivide;
}

void ProcessChunk(int startX, int endX, std::vector<std::vector<float>>* map, bool Image, bool Terrain, float normalizationFactor, int floorColour, bool smaller, int smallerResolution)
{
    int resolution;
    if (smaller) {
        resolution = smallerResolution;
    }
    else { resolution = n_resolution; }
    for (int x = 0; x < resolution; x++) {
        for (int y = 0; y < resolution; y++) {
            float colour = 0;
            if (n_PerlinOn) { colour += GenerateNoiseValue(&n_Perlin, n_PerlinOctaves, x, y) * n_PerlinWeight; }
            if (n_SimplexOn) { colour += GenerateNoiseValue(&n_Simplex, n_SimplexOctaves, x, y) * n_SimplexWeight; }
            if (n_CellularOn) { colour += GenerateNoiseValue(&n_Cellular, n_CellularOctaves, x, y) * n_CellularWeight; }

            float normalizedColour = colour * normalizationFactor + 0.5f;  // Normalize to [0, 1]
            if (Terrain) {
                float mapVal = pow(normalizedColour, n_Exponent);
                if (smaller) { mapVal = (mapVal * (n_height/4)); mapVal = max(n_floor/4, min(n_height/4, mapVal));
                }
                else{ mapVal = (mapVal * n_height); mapVal = max(n_floor, min(n_height, mapVal));
                }
                
                (*map)[x][y] = mapVal;
            }
            if (Image) {
                int texture = (int)(normalizedColour * 255);
                texture = max(0, min(255, texture)); // Clamp for safety
                n_pixels[x + y * resolution] = texture | texture << 8 | texture << 16 | 255 << 24;
            }
        }
    }
}
void RegenerateHeightMap();
void GenerateNoiseImageAndTerrain(bool Image, bool Terrain, bool SmallerTerrain)
{
    int resolution;
    int height;
    if (SmallerTerrain) { resolution = n_resolution / 4; height = n_height / 4; }
    else { resolution = n_resolution; height = n_height; }
    ClearAllUR();

    n_pixels = new uint32_t[resolution * resolution]; memset(n_pixels, 0, sizeof(uint32_t) * resolution * resolution);
    
    std::vector<std::vector<float>> map;
    map = std::vector<std::vector<float>>(resolution, std::vector<float>(resolution));

    static int numThreads = std::thread::hardware_concurrency()/2;
    int chunkSize = resolution / numThreads;

    float maxWeightSum = (n_PerlinOn * n_PerlinWeight) +
        (n_SimplexOn * n_SimplexWeight) +
        (n_CellularOn * n_CellularWeight);

    float normalizationFactor = 0.5f / maxWeightSum;

    std::vector<std::thread> threads;
    if (SmallerTerrain) {
        n_Perlin.SetFrequency(n_PerlinFrequency * 4); n_Simplex.SetFrequency(n_SimplexFrequency * 4); n_Cellular.SetFrequency(n_CellularFrequency * 4);
    }
    for (int i = 0; i < numThreads; ++i) {
        int startX = i * chunkSize;
        int endX = (i == numThreads - 1) ? resolution : (startX + chunkSize);
        if (SmallerTerrain) { threads.push_back(thread(ProcessChunk, startX, endX, &map, Image, Terrain, normalizationFactor, RatioValueConverter(0, height, 0, 255, n_floor/4), true, resolution)); }
        else { threads.push_back(thread(ProcessChunk, startX, endX, &map, Image, Terrain, normalizationFactor, RatioValueConverter(0, height, 0, 255, n_floor), false, resolution)); }
    }

    for (auto& thread : threads) {
        thread.join();
    }
    threads.clear();
    threads.shrink_to_fit();

    if (SmallerTerrain) {
        n_Perlin.SetFrequency(n_PerlinFrequency); n_Simplex.SetFrequency(n_SimplexFrequency); n_Cellular.SetFrequency(n_CellularFrequency);
    }

    if (Terrain) {
        g_pCamera->SetTarget(XMFLOAT3(resolution / 2, height / 2, resolution / 2));
        g_pCamera->SetDistance(resolution);
        g_GameObject.noiseGenerateTerrain(&map, resolution);
        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    }


    if (Image) {
        if (n_backTexture) {
            n_backTexture->Release();
            n_backTexture = nullptr;
        }
        D3D11_SUBRESOURCE_DATA subrecData = {};
        subrecData.pSysMem = n_pixels;
        subrecData.SysMemPitch = resolution * sizeof(uint32_t);

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = resolution;
        desc.Height = resolution;
        desc.MipLevels = desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subrecData, &n_backTexture);
        if (FAILED(hr)) {
            OutputDebugStringA("Failed to create texture\n");
            return;
        }
        std::swap(n_frontTexture, n_backTexture);
    }
    delete[] n_pixels;
    n_pixels = nullptr;
    RegenerateHeightMap();
    map.clear();
    map.shrink_to_fit();
}

void RegenerateHeightMap()
{
    int size = g_GameObject.GetSize();
    n_pixels = new uint32_t[size * size];
    std::memset(n_pixels, 0, sizeof(uint32_t) * size * size);
    float low = 300;
    float high = 0;
    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++)
        {
            if (g_GameObject.GetHeight(x,y) > high) { high = g_GameObject.GetHeight(x,y); }
            else if (g_GameObject.GetHeight(x, y) < low) { low = g_GameObject.GetHeight(x, y); }
        }
    }

    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++)
        {
            int colour = RatioValueConverter(low, high, 0, 255, g_GameObject.GetHeight(x, y));
            n_pixels[x + y * size] = colour | colour << 8 | colour << 16 | 255 << 24;
        }
    }

    D3D11_SUBRESOURCE_DATA subrecData = {};
    subrecData.pSysMem = n_pixels;
    subrecData.SysMemPitch = size * sizeof(uint32_t);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = size;
    desc.Height = size;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (n_backTexture) {
        n_backTexture->Release();
        n_backTexture = nullptr;
    }
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subrecData, &n_backTexture);
    std::swap(n_frontTexture, n_backTexture);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create texture\n");
        delete[] n_pixels;
        n_pixels = nullptr;
        return;
    }
    delete[] n_pixels;
    n_pixels = nullptr;
}
//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

        if (hr == E_INVALIDARG)
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        }

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create device.", L"Error", MB_OK);
        return hr;
    }

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2)
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
        if (SUCCEEDED(hr))
        {
            (void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;//  DXGI_FORMAT_R16G16B16A16_FLOAT;////DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create swapchain.", L"Error", MB_OK);
        return hr;
    }

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a back buffer.", L"Error", MB_OK);
        return hr;
    }

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a render target.", L"Error", MB_OK);
        return hr;
    }

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a depth / stencil texture.", L"Error", MB_OK);
        return hr;
    }

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
    {
        MessageBox(nullptr,
            L"Failed to create a depth / stencil view.", L"Error", MB_OK);
        return hr;
    }

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

	hr = InitRunTimeParameters();
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to initialise mesh.", L"Error", MB_OK);
		return hr;
	}

	hr = InitWorld(width, height);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"Failed to initialise world.", L"Error", MB_OK);
		return hr;
	}
	if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to init mesh in game object.", L"Error", MB_OK);
        return hr;
    }

    return S_OK;
}

// ***************************************************************************************
// InitMesh
// ***************************************************************************************

HRESULT		InitRunTimeParameters()
{
    n_Perlin.SetNoiseType(n_Perlin.NoiseType_Perlin);
    n_Perlin.SetFrequency(n_PerlinFrequency);
    n_Simplex.SetNoiseType(n_Simplex.NoiseType_OpenSimplex2);
    n_Simplex.SetFrequency(n_SimplexFrequency);
    n_Cellular.SetNoiseType(n_Cellular.NoiseType_Cellular);
    n_Cellular.SetFrequency(n_CellularFrequency);

    g_GameObject.setDetailRoughness(t_detail, t_roughness);
    g_GameObject.generateTerrain(n_pixels, n_backTexture, g_pd3dDevice);
    std::swap(n_frontTexture, n_backTexture);
    HRESULT hr = g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);


	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;


	// Create the constant buffer
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
	if (FAILED(hr))
		return hr;



	// Create the light constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pLightConstantBuffer);
	if (FAILED(hr))
		return hr;


	return hr;
}

// ***************************************************************************************
// InitWorld
// ***************************************************************************************
HRESULT		InitWorld(int width, int height)
{
    float val = (pow(2, t_detail) + 1) /2;
    g_pCamera = new Camera(XMFLOAT3(val, val, val), val, XMFLOAT3(0.0f, 1.0f, 0.0f));
    int temp = pow(2, t_detail) / 2;
    g_pCamera->SetTarget(XMFLOAT3(temp, temp, temp));
    g_pCamera->SetDistance(temp * 5);

	// Initialize the projection matrix
    constexpr float fovAngleY = XMConvertToRadians(60.0f);
	g_Projection = XMMatrixPerspectiveFovLH(fovAngleY, width / (FLOAT)height, 0.01f, 10000.0f);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_GameObject.cleanup();

    // Remove any bound render target or depth/stencil buffer
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    g_pImmediateContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);

    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
    // Flush the immediate context to force cleanup
    if (g_pImmediateContext1) g_pImmediateContext1->Flush();
    g_pImmediateContext->Flush();

    if (g_pLightConstantBuffer)
        g_pLightConstantBuffer->Release();
    if (g_pVertexLayout) g_pVertexLayout->Release();
    if( g_pConstantBuffer ) g_pConstantBuffer->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if (n_pixels ) delete[] n_pixels;
        

    FastNoiseLite           n_Perlin = FastNoiseLite();
    FastNoiseLite           n_Simplex = FastNoiseLite();
    FastNoiseLite           n_Cellular = FastNoiseLite();

    ID3D11ShaderResourceView* n_texture = nullptr;

    ID3D11Debug* debugDevice = nullptr;
    g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDevice));

    if (g_pd3dDevice1) g_pd3dDevice1->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    // handy for finding dx memory leaks
    //debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL); // add back later for debugging

    if (debugDevice)
        debugDevice->Release();
}

// Function to center the mouse in the window
void CenterMouseInWindow(HWND hWnd)
{
    // Get the dimensions of the window
    RECT rect;
    GetClientRect(hWnd, &rect);

    // Calculate the center position
    POINT center;
    center.x = (rect.right - rect.left) / 2;
    center.y = (rect.bottom - rect.top) / 2;

    // Convert the client area point to screen coordinates
    ClientToScreen(hWnd, &center);

    // Move the cursor to the center of the screen
    SetCursorPos(center.x, center.y);
}

struct brushReturn {
    DirectX::XMVECTOR StructPositon;
    XMFLOAT3 StructPickedPosition;

    XMVECTOR RayDirection;

    brushReturn(DirectX::XMVECTOR Pos, XMFLOAT3 F3, XMVECTOR V) { StructPositon = Pos; StructPickedPosition = F3; RayDirection = V; }
};

brushReturn RaycastBrush()
{
    XMFLOAT3 cameraPosition = g_pCamera->GetPosition();
    XMVECTOR rayOrigin = XMLoadFloat3(&cameraPosition);
    XMFLOAT2 temp = XMFLOAT2(mousePos.x, mousePos.y);
    XMVECTOR cursorPos = XMLoadFloat2(&temp);
    XMMATRIX mGO = XMLoadFloat4x4(g_GameObject.getTransform());
    XMMATRIX World = XMMatrixTranspose(mGO);
    float viewportWidth = (float)(rect.right - rect.left);
    float viewportHeight = (float)(rect.bottom - rect.top);
    XMVECTOR rayDirection = XMVector3Normalize(XMVector3Unproject(cursorPos, 0, 0,
        (viewportWidth), (viewportHeight),
        0.0f, 1.0f, g_Projection, g_pCamera->GetViewMatrix(), World) - rayOrigin);

    float stepSize = 0.5f;
    float currentDistance = 0.0f;
    int maxDistance = 5000;
    XMVECTOR Position = rayOrigin;

    while (currentDistance < maxDistance)
    {
        float x = XMVectorGetX(Position);
        float z = XMVectorGetZ(Position);
        float size = g_GameObject.GetSize()-1;

        if (x <= size && x >= 0 && z <= size && z >= 0) {
            // Find the height of the picked vertex
            int pickedX = (int)x;
            int pickedZ = (int)z;
            float pickedHeight = g_GameObject.GetHeight(pickedX, pickedZ);
            if (abs(XMVectorGetY(Position) - pickedHeight) <= 2)
            {
                return brushReturn(Position, XMFLOAT3(pickedX, pickedHeight, pickedZ), rayDirection);
            }
        }
        Position += rayDirection * stepSize;
        currentDistance += stepSize;
    }
    return brushReturn(Position, XMFLOAT3(-1, -1, -1), rayDirection);
}

void BrushFunc(bool held, int BrushNumber, int BrushSize)
{
    brushReturn info = RaycastBrush();
    XMVECTOR Position = info.StructPositon;
    XMFLOAT3 F3 = info.StructPickedPosition;
    if (F3.x == -1 && F3.y == -1 && F3.z)
    {
        return;
    }
    int pickedX = F3.x;
    int pickedZ = F3.z;
    int pickedHeight = F3.y;
    float size = g_GameObject.GetSize();
    for (int dx = -BrushSize; dx <= BrushSize; dx++) {
        for (int dz = -BrushSize; dz <= BrushSize; dz++) {
            int nx = pickedX + dx;
            int nz = pickedZ + dz;

            // Check bounds
            if (nx >= 0 && nx < size && nz >= 0 && nz < size) {
                if (dx * dx + dz * dz <= BrushSize * BrushSize) {
                    float curheight = g_GameObject.GetHeight(nx, nz);
                    //Raise brush
                    if (BrushNumber == 1)
                    {
                        float distanceFactor = sqrt(pow(dx, 2) + pow(dz, 2)) / BrushSize;

                        float weight = pow(1 - distanceFactor, b_RaiseStrength);

                        float newHeight = weight * b_RaiseRate + curheight;

                        AddPositionToUndo(XMFLOAT3(nx, curheight, nz), 0, XMFLOAT3(0, 0, 0));

                        g_GameObject.SetHeight(nx, nz, newHeight);
                    }
                    //Lower / Depression brush
                    else if (BrushNumber == 2)
                    {
                        float distanceFactor = sqrt(pow(dx, 2) + pow(dz, 2)) / BrushSize;

                        float weight = pow(1 - distanceFactor, b_LowerStrength);

                        float newHeight = curheight - weight * b_LowerRate ;
                        AddPositionToUndo(XMFLOAT3(nx, curheight, nz), 0, XMFLOAT3(0,0,0));
                        g_GameObject.SetHeight(nx, nz, newHeight);
                    }
                    //Flatten brush
                    else if (BrushNumber == 3)
                    {
                        if (abs(pickedHeight - g_GameObject.GetHeight(nx, nz)) <= b_MaxFlatDifference)
                        {
                            float newHeight = (1.0f - b_SmoothnessFactor) * curheight + b_SmoothnessFactor * pickedHeight;
                            AddPositionToUndo(XMFLOAT3(nx, curheight, nz), 0, XMFLOAT3(0, 0, 0));
                            g_GameObject.SetHeight(nx, nz, newHeight);
                        }
                    }
                }
            }
        }
    }
    if (held)
    {
        static int frame = 0;
        if (frame % 5 == 0)
        {
            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
            RegenerateHeightMap();
        }
        frame++;
    }
    else {
        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
        RegenerateHeightMap();
    }

}

void SmoothBrush(bool held, int BrushSize)
{
    brushReturn info = RaycastBrush();
    XMVECTOR Position = info.StructPositon;
    XMFLOAT3 F3 = info.StructPickedPosition;
    XMVECTOR rayDirection = info.RayDirection;
    XMVECTOR rightDirection = XMVector3Cross(rayDirection, XMVectorSet(0, 1, 0, 0)); // Correct cross product

    if (F3.x == -1 && F3.y == -1 && F3.z)
    {
        return;
    }

    int pickedX = F3.x;
    int pickedZ = F3.z;
    float size = g_GameObject.GetSize();

    std::vector<XMFLOAT3> addVector;

    for (int dx = -BrushSize; dx <= BrushSize; dx++) {
        for (int dz = -BrushSize; dz <= BrushSize; dz++) {
            int nx = pickedX + dx;
            int nz = pickedZ + dz;

            // Check bounds
            if (nx >= 0 && nx < size && nz >= 0 && nz < size) {
                XMVECTOR vertexPos = XMVectorSet(nx, g_GameObject.GetHeight(nx, nz), nz, 1);

                // Project onto the brush plane
                XMVECTOR offset = vertexPos - XMLoadFloat3(&F3);
                float vx = XMVectorGetX(XMVector3Dot(offset, rightDirection));
                float vz = XMVectorGetX(XMVector3Dot(offset, rayDirection));

                // Check if within the brush circle
                if (vx * vx + vz * vz <= BrushSize * BrushSize) {
                    // Compute average height (with bounds check)
                    float avgHeight = g_GameObject.GetHeight(nx, nz);
                    int neighborCount = 1;

                    if (nx + 1 < size) {
                        avgHeight += g_GameObject.GetHeight(nx + 1, nz);
                        neighborCount++;
                    }
                    if (nx - 1 >= 0) {
                        avgHeight += g_GameObject.GetHeight(nx - 1, nz);
                        neighborCount++;
                    }
                    if (nz + 1 < size) {
                        avgHeight += g_GameObject.GetHeight(nx, nz + 1);
                        neighborCount++;
                    }
                    if (nz - 1 >= 0) {
                        avgHeight += g_GameObject.GetHeight(nx, nz - 1);
                        neighborCount++;
                    }
                    avgHeight /= neighborCount;
                    float newHeight = b_SmoothRate * avgHeight + (1 - b_SmoothRate) * g_GameObject.GetHeight(nx,nz);
                    AddPositionToUndo(XMFLOAT3(nx, g_GameObject.GetHeight(nx, nz), nz), 0, XMFLOAT3(0,0,0));

                    // Add smoothed height to update vector
                    addVector.push_back(XMFLOAT3(nx, newHeight, nz));
                }
            }
        }
    }

    // Apply smoothed heights
    for (const auto& val : addVector) {
        g_GameObject.SetHeight(val.x, val.z, val.y); // Adjust SetHeight argument order
    }

    // Clean up and rebuild the mesh
    if (held) {
        static int frame = 0;
        if (frame % 5 == 0) {
            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
            RegenerateHeightMap();
        }
        frame++;
    }
    else {
        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
        RegenerateHeightMap();
    }
}

void ColourBrush(bool held, int BrushSize)
{
    brushReturn info = RaycastBrush();
    XMVECTOR Position = info.StructPositon;
    XMFLOAT3 F3 = info.StructPickedPosition;
    XMVECTOR rayDirection = info.RayDirection;

    if (F3.x == -1)
    {
        return;
    }

    int pickedX = F3.x;
    int pickedZ = F3.z;
    float size = g_GameObject.GetSize();

    std::vector<XMFLOAT3> addVector;

    // Get the surface normal at the picked position
    XMVECTOR surfaceNormal;
    XMFLOAT3 temp = g_GameObject.GetNormal(pickedX, pickedZ);
    surfaceNormal = XMLoadFloat3(&temp);

    // Calculate the right direction relative to the surface normal
    XMVECTOR rightDirection = XMVector3Cross(rayDirection, surfaceNormal);
    rightDirection = XMVector3Normalize(rightDirection);

    for (int dx = -BrushSize; dx <= BrushSize; dx++) {
        for (int dz = -BrushSize; dz <= BrushSize; dz++) {
            int nx = pickedX + dx;
            int nz = pickedZ + dz;

            // Check bounds
            if (nx >= 0 && nx < size && nz >= 0 && nz < size) {
                XMVECTOR vertexPos = XMVectorSet(nx, g_GameObject.GetHeight(nx, nz), nz, 1);

                // Calculate the offset from the center of the brush
                XMVECTOR offset = vertexPos - XMLoadFloat3(&F3);

                // Project the offset onto the surface plane
                XMVECTOR projectedOffset = offset - XMVector3Dot(offset, surfaceNormal) * surfaceNormal;

                // Calculate the distance squared from the center of the brush
                float distanceSq = XMVectorGetX(XMVector3LengthSq(projectedOffset));

                // Check if within the brush circle
                if (distanceSq <= BrushSize * BrushSize) {
                    AddPositionToUndo(XMFLOAT3(nx, 0, nz), 1, g_GameObject.GetColour(nx, nz));
                    g_GameObject.SetColour(nx, nz, b_Colour);
                }
            }
        }
    }

    // Clean up and rebuild the mesh
    if (held) {
        static int frame = 0;
        if (frame % 5 == 0) {
            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
            RegenerateHeightMap();
        }
        frame++;
    }
    else {
        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
        RegenerateHeightMap();
    }
}
//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    mousePos = MAKEPOINTS(lParam);
    GetClientRect(hWnd, &rect);

    // Calculate the center position of the window
    POINT windowCenter;
    windowCenter.x = (rect.right - rect.left) / 2;
    windowCenter.y = (rect.bottom - rect.top) / 2;

    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch( message )
    {

    case WM_KEYDOWN:
        switch (wParam)
        {
        case 27:
            PostQuitMessage(0);
            break;
        case 0x5A:
        {
            if (GetKeyState(VK_LCONTROL) & 0x8000) {
                if (GetKeyState(VK_LSHIFT) & 0x8000)
                {
                    Redo();
                }
                else {
                    Undo();
                }
                g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
            }
            break;
        }
        case 49:
        {
            if (b_FlatBool && !ImGui::IsAnyItemActive())
            {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            else if (!ImGui::IsAnyItemActive()) {
                b_FlatBool = true;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            break;
        }
        case 50:
        {
            if (b_RaiseBool && !ImGui::IsAnyItemActive())
            {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            else if (!ImGui::IsAnyItemActive() ){
                b_FlatBool = false;
                b_RaiseBool = true;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            break;
        }
        case 51:
        {
            if (b_LowerBool && !ImGui::IsAnyItemActive())
            {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            else if ( !ImGui::IsAnyItemActive() ){
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = true;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            break;
        }
        case 52:
        {
            if (b_SmoothBool && !ImGui::IsAnyItemActive())
            {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            else if ( !ImGui::IsAnyItemActive() ){
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = true;
                b_ColourBool = false;
                ColourPicker = false;
            }
            break;
        }
        case 53:
        {
            if (b_ColourBool && !ImGui::IsAnyItemActive())
            {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            else if (!ImGui::IsAnyItemActive()) {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = true;
                TerrainGenerationMode = 3;
                ColourPicker = false;
            }
            break;
        }
        case 80:
        {
            if (ColourPicker)
            {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = false;
            }
            else
            {
                b_FlatBool = false;
                b_RaiseBool = false;
                b_LowerBool = false;
                b_SmoothBool = false;
                b_ColourBool = false;
                ColourPicker = true;
            }

            break;
        }
        case 69:
        {
            g_GameObject.hydraulicErosion(h_cycles);
            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
            break;
        }
        case 83:
        {
            float size = g_GameObject.GetSize();

            std::vector<std::vector<float>> smoothedHeights(size, std::vector<float>(size, 0.0f));

            for (int x = 0; x < size; x++)
            {
                for (int y = 0; y < size; y++)
                {
                    float avgHeight = 0;
                    int neighborCount = 0;

                    // Calculate the average height of neighbors
                    if (x + 1 < size) {
                        avgHeight += g_GameObject.GetHeight(x + 1, y);
                        neighborCount++;
                    }
                    if (x - 1 >= 0) {
                        avgHeight += g_GameObject.GetHeight(x - 1, y);
                        neighborCount++;
                    }
                    if (y + 1 < size) {
                        avgHeight += g_GameObject.GetHeight(x, y + 1);
                        neighborCount++;
                    }
                    if (y - 1 >= 0) {
                        avgHeight += g_GameObject.GetHeight(x, y - 1);
                        neighborCount++;
                    }

                    if (neighborCount > 0) {
                        avgHeight /= neighborCount;

                        // Blend the averaged height with the original height
                        float originalHeight = g_GameObject.GetHeight(x, y);
                        float newHeight = 0.1 * avgHeight + (1 - 0.1) * originalHeight;

                        // Store the smoothed height in the temporary buffer
                        smoothedHeights[x][y] = newHeight;
                    }
                    else {
                        // If no neighbors, keep the original height
                        smoothedHeights[x][y] = g_GameObject.GetHeight(x, y);
                    }
                }
            }

            // Apply the smoothed heights to the terrain
            for (int x = 0; x < size; x++)
            {
                for (int y = 0; y < size; y++)
                {
                    g_GameObject.SetHeight(x, y, smoothedHeights[x][y]);
                }
            }
            smoothedHeights.clear();
            smoothedHeights.shrink_to_fit();
            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
            RegenerateHeightMap();
            break;
        }
        }
        break;
    
    case WM_MBUTTONDOWN:
        {
        brushReturn info = RaycastBrush();
        g_pCamera->SetTarget(info.StructPickedPosition);
        break;
        }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);

        g_pCamera->Zoom(-delta * c_zoomSens);
        break;
    }
    case WM_RBUTTONDOWN:
    {
        mouseDownR = true;
        SetCursorPos(windowCenter.x, windowCenter.y);
        break;
    }
    case WM_RBUTTONUP:
        mouseDownR = false;
        break;
    case WM_LBUTTONDOWN:
    {
        if (!ImGui::IsAnyItemActive()) {
            if (ColourPicker) {
                brushReturn b = RaycastBrush();
                if (b.StructPickedPosition.x != -1)
                {
                    ColourPicker = false;
                    b_Colour = g_GameObject.GetColour(b.StructPickedPosition.x, b.StructPickedPosition.z);
                    break;
                }
            }
            mouseDownL = true;
            }
        break;
    }
    case WM_LBUTTONUP:
        if ((b_FlatBool || b_LowerBool || b_RaiseBool || b_SmoothBool || b_ColourBool) && wasEdited)
        {
            IncrementUndoRedo(1);
            CheckVectorSpace();
            ClearAfterVector();
            wasEdited = false;
        }
        mouseDownL = false;
        break;
    case WM_MOUSEMOVE:
    {
        static bool isFirstClick = true;
        if (mouseDownL) {
            static POINTS lastMousePos = {};
            if (isFirstClick) {
                lastMousePos = mousePos;
                isFirstClick = false;
            }
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {

                POINTS delta = { mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y };


                XMFLOAT3 cameraPos = g_pCamera->GetPosition();
                XMFLOAT3 targetPos = g_pCamera->GetTarget();

                XMVECTOR camPosVec = XMLoadFloat3(&cameraPos);
                XMVECTOR targetPosVec = XMLoadFloat3(&targetPos);
                XMVECTOR viewDirection = XMVector3Normalize(targetPosVec - camPosVec); 

                XMVECTOR upVec = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); 
                XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(upVec, viewDirection));
                upVec = XMVector3Normalize(XMVector3Cross(viewDirection, rightVec));

                XMVECTOR deltaMovement = XMVectorAdd(
                    XMVectorScale(rightVec, -delta.x * c_sensitivity),
                    XMVectorScale(upVec, delta.y * c_sensitivity)
                );

                XMVECTOR newTargetVec = targetPosVec + deltaMovement;

                XMVECTOR targetDelta = newTargetVec - targetPosVec;

                XMFLOAT3 newTargetPos;
                DirectX::XMStoreFloat3(&newTargetPos, newTargetVec);

                g_pCamera->SetOnlyTarget(newTargetPos);
                g_pCamera->UpdatePositionWithTargetMove(XMFLOAT3(
                    XMVectorGetX(targetDelta),
                    XMVectorGetY(targetDelta),
                    XMVectorGetZ(targetDelta)
                ));

                lastMousePos = mousePos;

                break;
            }
        }
        else { isFirstClick = true; }
        if (mouseDownR)
        {

            // Convert the client area point to screen coordinates
            ClientToScreen(hWnd, &windowCenter);

            // Get the current cursor position
            POINT cursorPos = { mousePos.x, mousePos.y };
            ClientToScreen(hWnd, &cursorPos);

            // Calculate the delta from the window center
            POINT delta;
            delta.x = cursorPos.x - windowCenter.x;
            delta.y = -(cursorPos.y - windowCenter.y);

            // Sensitivity factor
            const float sensitivity = 0.001f;

            // Update the camera with the delta
            g_pCamera->Rotate(delta.x * sensitivity, -delta.y * sensitivity);

            // Recenter the cursor
            SetCursorPos(windowCenter.x, windowCenter.y);
        }
        break;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
            CenterMouseInWindow(hWnd);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

        // Note that this tutorial does not handle resizing (WM_SIZE) requests,
        // so we created the window without the resize border.
    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

bool OpenFileDialog(bool imgObj) {
    nfdchar_t* outPath = NULL;
    const nfdchar_t* filterList;
    if (imgObj) {
        filterList = "png,jpg,jpeg";
    }
    else if (!imgObj) {
        filterList = "glb";
    }
    nfdresult_t result = NFD_OpenDialog(filterList, NULL, &outPath);

    if (result == NFD_OKAY) {
        std::cout << "Selected file: " << outPath << std::endl;
        LoadObjLocation = (string)outPath;
        free(outPath);
        return true;
    }
    else if (result == NFD_CANCEL) {
        std::cout << "User pressed cancel." << std::endl;
        return false;
    }
    else {
        std::cout << "Error: " << NFD_GetError() << std::endl;
        return false;
    }
    return false;
}

bool OpenFolderDialog() {
    nfdchar_t* outPath = NULL;
    nfdresult_t result = NFD_SaveDialog(NULL, NULL, &outPath);

    if (result == NFD_OKAY) {
        std::cout << "Selected file: " << outPath << std::endl;
        filesystem::path filePath = outPath;
        filesystem::path folder_location = filePath.parent_path();
        filesystem::path file_name = filePath.filename();
        SaveLocation = folder_location.string();
        FileName = file_name.string();
        return true;
        free(outPath);
    }
    else if (result == NFD_CANCEL) {
        std::cout << "User pressed cancel." << std::endl;
        return false;
    }
    else {
        std::cout << "Error: " << NFD_GetError() << std::endl;
        return false;
    }
    return false;
}

void LoadTerrain() {
    GLB.ReadGLB(LoadObjLocation, &g_GameObject);
    g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    loadedModel = true;
    //ifstream myfile;
    //myfile.open(SaveLocation, ios::in);
    //if (!myfile.fail())
    //{
    //    std::vector<std::vector<float>> map;
    //    std::vector<std::vector<XMFLOAT3>> colourMap;
    //	string line;
    //    bool start = false;
    //    bool vertex = false;
    //    bool index = false;
    //    int vertexCount;
    //    int vertexIndex = 0;
    //    int squareNum;

    //    while (getline(myfile, line)) {
    //        if (!start && !line.empty() && line == "end_header")
    //        {
    //            start = true;
    //        }
    //        if (!start && !line.empty() && line[0] == 'e' && line[8] == 'v')
    //        {
    //            sscanf(line.c_str(), "element vertex %i", &vertexCount);
    //            squareNum = sqrt(vertexCount);
    //            map = std::vector<std::vector<float>>(squareNum, std::vector<float>(squareNum, 0.0f));
    //            colourMap = std::vector<std::vector<XMFLOAT3>>(squareNum, std::vector<XMFLOAT3>(squareNum, XMFLOAT3(0, 0, 0)));
    //        }
    //        if (start && vertexIndex <= vertexCount && !line.empty() && line != "end_header") {
    //            XMFLOAT3 vertex;
    //            XMFLOAT3 normal;
    //            XMINT3 colour;
    //            sscanf(line.c_str(), "%f %f %f %f %f %f %i %i %i %i", &vertex.x, &vertex.y, &vertex.z, &normal.x, &normal.y, &normal.z, &colour.x, &colour.y, &colour.z, &normal.x);
    //            map[vertex.x][vertex.z] = vertex.y;
    //            colourMap[vertex.x][vertex.z] = XMFLOAT3(((float)colour.x) / 255, ((float)colour.y) / 255, ((float)colour.z) / 255);
    //            //OutputDebugStringA((to_string(vertices[vertIndex].Pos.x) + " ").c_str());
    //            //OutputDebugStringA((to_string(vertices[vertIndex].Pos.y) + " ").c_str());
    //            //OutputDebugStringA((to_string(vertices[vertIndex].Pos.z)+"\n").c_str());
    //            vertexIndex++;
    //            continue;
    //        }
    //        if (start && vertexIndex > vertexCount) {
    //            g_GameObject.loadTerrain(&map, &colourMap, squareNum);
    //            g_pCamera->SetDistance(squareNum);
    //            g_pCamera->SetTarget(XMFLOAT3(squareNum / 2, squareNum / 2, squareNum / 2));
    //            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    //            g_GameObject.draw(g_pImmediateContext);
    //            RegenerateHeightMap();
    //            loadedModel = true;
    //            TerrainGenerationMode = 2;
    //            return;
    //        }
    //    }
    //}
}

void LoadPNG() {
    std::wstring wSaveLocation = std::wstring(LoadObjLocation.begin(), LoadObjLocation.end());
    DirectX::ScratchImage image;
    HRESULT hr = DirectX::LoadFromWICFile(wSaveLocation.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(hr))
    {
        // Handle the error (e.g., log it)
        std::wcerr << L"Failed to load image from file: " << wSaveLocation << std::endl;
        return;
    }

    const DirectX::TexMetadata metadata = image.GetMetadata();
    const DirectX::Image* img = image.GetImage(0, 0, 0);

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = metadata.width;
    texDesc.Height = metadata.height;
    texDesc.MipLevels = static_cast<UINT>(metadata.mipLevels);
    texDesc.ArraySize = static_cast<UINT>(metadata.arraySize);
    texDesc.Format = img->format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_STAGING;
    texDesc.BindFlags = 0;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    // Create the texture
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = img->pixels;
    initData.SysMemPitch = static_cast<UINT>(img->rowPitch);
    initData.SysMemSlicePitch = static_cast<UINT>(img->slicePitch);

    ID3D11Texture2D* stagingTexture;
    if (n_backTexture)
    {
        n_backTexture->Release();
        n_backTexture = nullptr;
    }

    hr = g_pd3dDevice->CreateTexture2D(&texDesc, &initData, &stagingTexture);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create texture. HRESULT: " << hr << std::endl;
        return;
    }

    //D3D11_TEXTURE2D_DESC texDescReal = texDesc;
    //texDescReal.Usage = D3D11_USAGE_DEFAULT; // Usage must be default for rendering
    //texDescReal.CPUAccessFlags = 0; // No CPU access required for rendering
    //texDescReal.BindFlags = D3D11_BIND_SHADER_RESOURCE; // Binding for shader resource

    //hr = g_pd3dDevice->CreateTexture2D(&texDescReal, &initData, &n_backTexture);
    //if (FAILED(hr)) {
    //    std::wcerr << L"Failed to create texture. HRESULT: " << hr << std::endl;
    //    return;
    //}

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = g_pImmediateContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to map texture for reading. HRESULT: " << hr << std::endl;
        return;
    }


    void* pixelData = mappedResource.pData;
    int width = texDesc.Width;
    int height = texDesc.Height;

   /* std::swap(n_frontTexture, n_backTexture);*/

    n_height = 512;
    std::vector<std::vector<float>> map(height, std::vector<float>(width));
    if (img->format == DXGI_FORMAT_R16_UNORM)
    {
        for (int x = 0; x < height; x++) {
            unsigned char* rowStart = static_cast<unsigned char*>(pixelData) + x * mappedResource.RowPitch;
            for (int y = 0; y < width; y++) {
                unsigned short r = *reinterpret_cast<unsigned short*>(&rowStart[y * 2]);

                int colour = (r); // Average RGB
                map[x][y] = RatioValueConverter(0, 65535, n_floor, n_height, colour);
            }
        }
    }
    else if (img->format == DXGI_FORMAT_R8_UNORM)
    {
        for (int x = 0; x < height; x++) {
            unsigned char* rowStart = static_cast<unsigned char*>(pixelData) + x * mappedResource.RowPitch;
            for (int y = 0; y < width; y++) {
                unsigned char r = rowStart[y];     // Red channel (assumes RGBA 8-bit format)

                int colour = (r); // Average RGB
                map[x][y] = RatioValueConverter(0, 255, n_floor, n_height, colour);
            }
        }
    }
    else {
        for (int x = 0; x < height; x++) {
            unsigned char* rowStart = static_cast<unsigned char*>(pixelData) + x * mappedResource.RowPitch;
            for (int y = 0; y < width; y++) {
                unsigned char r = rowStart[y * 4];     // Red channel (assumes RGBA 8-bit format)
                unsigned char g = rowStart[y * 4 + 1]; // Green channel
                unsigned char b = rowStart[y * 4 + 2]; // Blue channel

                int colour = (r + g + b) / 3; // Average RGB
                map[x][y] = RatioValueConverter(0, 255, n_floor, n_height, colour);
            }
        }
    }
    g_GameObject.noiseGenerateTerrain(&map, height);
    g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    RegenerateHeightMap();
    if (stagingTexture) { stagingTexture->Release(); stagingTexture = nullptr; }
    map.clear();
    map.shrink_to_fit();
    loadedModel = true;
    n_PerlinOn = false;
    n_SimplexOn = false;
    n_CellularOn = false;
    g_pCamera->SetTarget(XMFLOAT3(g_GameObject.GetSize()/2, n_height / 2, g_GameObject.GetSize()/2));
    TerrainGenerationMode = 2;
}

void EditLoadedModel(int option) {
    int size = g_GameObject.GetSize();
    float low = 0;
    float high = 0;
    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++)
        {
            if (g_GameObject.GetHeight(x, y) > high) { high = g_GameObject.GetHeight(x, y); }
            else if (g_GameObject.GetHeight(x, y) < low) { low = g_GameObject.GetHeight(x, y); }
        }
    }
    
    if (option == 2)
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++)
            {
                float height = RatioValueConverter(low, high, 0, n_height, g_GameObject.GetHeight(x, y));
                g_GameObject.SetHeight(x, y, height);
                if (g_GameObject.GetHeight(x, y) <= n_floor) {
                    g_GameObject.SetHeight(x, y, n_floor);
                }
            }
        }
    }
    if (option == 3)
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++)
            {
                if (g_GameObject.GetHeight(x, y) <= n_floor) {
                    g_GameObject.SetHeight(x, y, n_floor);
                }
            }
        }
    }
    g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    g_GameObject.draw(g_pImmediateContext);
    RegenerateHeightMap();
}
void setupLightForRender()
{
    Light light;
    light.Enabled = static_cast<int>(true);
    light.LightType = PointLight;
    light.Color = XMFLOAT4(Colors::White);
    //light.SpotAngle = XMConvertToRadians(360.0f);
    light.ConstantAttenuation = 1.0f;
    light.LinearAttenuation = 0.05f;
    light.QuadraticAttenuation = 0.01f;

    // set up the light
    static XMFLOAT4 LightPosition(-g_pCamera->GetPosition().x, -g_pCamera->GetPosition().y, -g_pCamera->GetPosition().z, 1);
    light.Position = LightPosition;
    //XMVECTOR LightDirection = XMVectorSet(-LightPosition.x, -LightPosition.y, -LightPosition.z, 0.0f);
    //LightDirection = XMVector3Normalize(LightDirection);
    //XMStoreFloat4(&light.Direction, LightDirection);

    LightPropertiesConstantBuffer lightProperties;
    lightProperties.EyePosition = LightPosition;
    lightProperties.Lights[0] = light;
    g_pImmediateContext->UpdateSubresource(g_pLightConstantBuffer, 0, nullptr, &lightProperties, 0, 0);
}

float calculateDeltaTime()
{
    // Update our time
    static float deltaTime = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
        timeStart = timeCur;
    deltaTime = (timeCur - timeStart) / 1000.0f;
    timeStart = timeCur;

    float FPS60 = 1.0f / 60.0f;
    static float cummulativeTime = 0;

    // cap the framerate at 60 fps 
    cummulativeTime += deltaTime;
    if (cummulativeTime >= FPS60) {
        cummulativeTime = cummulativeTime - FPS60;
    }
    else {
        return 0;
    }

    return deltaTime;
}

void saveTerrain()
{
    GLB.ExportGLB(g_GameObject.GetVertices(), g_GameObject.GetIndices(), g_GameObject.GetVertexCount(), g_GameObject.GetIndexCount(), SaveLocation, FileName);
    //std::string fileName = FileName + ".ply";
    //ofstream myfile(SaveLocation+"\\"+fileName);
    //int CountVertices = 0;
    //int CountIndices = 0;
    //int IndexCount = g_GameObject.GetIndexCount();
    //int VertexCount = g_GameObject.GetVertexCount();
    //std::vector<std::vector<XMFLOAT3>>* CM = g_GameObject.GetColourMap();
    //SimpleVertex* SV = g_GameObject.GetVertices();
    //DWORD* Faces = g_GameObject.GetIndices();
    //int size = g_GameObject.GetSize();
    //std::ostringstream output;
    //myfile << "ply\n";
    //myfile << "format ascii 1.0\n";
    //myfile << "element vertex " << VertexCount << "\n";
    //myfile << "property float x\n";
    //myfile << "property float y\n";
    //myfile << "property float z\n";
    //myfile << "property float nx\n";
    //myfile << "property float ny\n";
    //myfile << "property float nz\n";
    //myfile << "property uchar red\n";
    //myfile << "property uchar blue\n";
    //myfile << "property uchar green\n";
    //myfile << "property uchar alpha\n";
    //myfile << "element face " << IndexCount/3 << "\n";
    //myfile << "property list uchar int vertex_indices\n";
    //myfile << "end_header\n";

    //if (myfile)
    //{
    //    int x = 0;
    //    int y = 0;
    //    for (int i = 0; i < VertexCount; ++i) {

    //        output << SV[i].Pos.x << " " << SV[i].Pos.y << " " << SV[i].Pos.z << " " << SV[i].Normal.x << " " << SV[i].Normal.y << " " << SV[i].Normal.z << " " << static_cast<int>((*CM)[x][y].x * 255) << " " << static_cast<int>((*CM)[x][y].y * 255) << " " << static_cast<int>((*CM)[x][y].z * 255) << " " << 255 << "\n";

    //        y++;
    //        if (y >= size)
    //        {
    //            y = 0;
    //            x++;
    //        }
    //        if ((i + 1) % 1000 == 0) {  // Flush every 1,000 vertices
    //            myfile << output.str();
    //            output.str("");
    //            output.clear();
    //        }
    //    }
    //    if (!output.str().empty())
    //    {
    //        myfile << output.str();
    //        output.str("");
    //        output.clear();
    //    }
    //    for (int i = 0; i < IndexCount; i += 3) {
    //        output << 3 << " " << Faces[i] << " " << Faces[i + 1] << " " << Faces[i + 2] << "\n";

    //        if ((i + 3) % 1000 == 0) {  // Flush every 1,000 faces
    //            myfile << output.str();
    //            output.str("");
    //            output.clear();
    //        }
    //    }
    //    if (!output.str().empty())
    //    {
    //        myfile << output.str();
    //        output.str("");
    //        output.clear();
    //    }
    //}
}

void saveImage()
{
    std::string fileName = SaveLocation + "\\" + FileName + ".png";
    std::wstring wFileName(fileName.begin(), fileName.end());
    int size = g_GameObject.GetSize();
    uint16_t* pixels = new uint16_t[size * size];
    std::memset(pixels, 0, sizeof(uint16_t) * size * size);
    float low = 300;
    float high = 0;
    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++)
        {
            if (g_GameObject.GetHeight(x, y) > high) { high = g_GameObject.GetHeight(x, y); }
            else if (g_GameObject.GetHeight(x, y) < low) { low = g_GameObject.GetHeight(x, y); }
        }
    }

    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++)
        {
            int colour = RatioValueConverter(low, high, 0, 65535, g_GameObject.GetHeight(x, y));
            pixels[x + y * size] = colour;
        }
    }

    D3D11_SUBRESOURCE_DATA subrecData = {};
    subrecData.pSysMem = pixels;
    subrecData.SysMemPitch = size * sizeof(uint16_t);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = size;
    desc.Height = size;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R16_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* saveTexture;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subrecData, &saveTexture);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create texture\n");
        delete[] pixels;
        pixels = nullptr;
        return;
    }
    DirectX::ScratchImage image;
    DirectX::CaptureTexture(g_pd3dDevice, g_pImmediateContext, saveTexture, image);
    DirectX::SaveToWICFile(*image.GetImages(), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, wFileName.c_str());
    if (saveTexture)
    {
        saveTexture->Release();
        saveTexture = nullptr;
    }
    if (pixels)
    {
        delete[] pixels;
        pixels = nullptr;
    }
}

static void HelpMarker(const char* desc)
{ 
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
void HydroErosionGUI() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthL * 0.9 * ScaleX, 80 * ScaleY), ImVec2(UIWidthL * ScaleX, 1500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(0, 0 + height));
    ImGui::Begin("Hydraulic Erosion", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    ImGui::SliderInt("Cycles", &h_cycles, 2000, 200000);
    HelpMarker("The amount of 'droplets' created and simulated, depending on the size of the terrain you want more.");
    ImGui::Checkbox("Advanced hydraulic erosion controls", &advHydro);
    if (advHydro) {
        ImGui::SetNextItemWidth(SliderWidth / 2 * ScaleX);
        ImGui::InputFloat("Timestep", &g_GameObject.hydraulicErosionClass.dt);
        HelpMarker("Higher = More erosion, simulated time between each erosion calculation");
        ImGui::SetNextItemWidth(SliderWidth / 2 * ScaleX);
        ImGui::InputFloat("Density", &g_GameObject.hydraulicErosionClass.density);
        HelpMarker("Density of droplet, to give their inertia");
        ImGui::SetNextItemWidth(SliderWidth / 2 * ScaleX);
        ImGui::InputFloat("Evaporation Rate", &g_GameObject.hydraulicErosionClass.evapRate);
        HelpMarker("Volume loss factor every timestep");
        ImGui::SetNextItemWidth(SliderWidth / 2 * ScaleX);
        ImGui::InputFloat("Deposition Rate", &g_GameObject.hydraulicErosionClass.depositionRate);
        HelpMarker("Rate of at which mass is deposited to reach equilibrium");
        ImGui::SetNextItemWidth(SliderWidth / 2 * ScaleX);
        ImGui::InputFloat("Minimum Volume", &g_GameObject.hydraulicErosionClass.minVol);
        HelpMarker("Volume below which a droplet is killed");
        ImGui::SetNextItemWidth(SliderWidth / 2 * ScaleX);
        ImGui::InputFloat("Friction", &g_GameObject.hydraulicErosionClass.friction);
        HelpMarker("Speed loss factor every timestep");
    }
    if (ImGui::Button("Hydraulic Errosion"))
    {
        g_GameObject.hydraulicErosion(h_cycles);
        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    }
    ImGui::SameLine();
    ImGui::Text("Key E");
    if (ImGui::Button("Smooth Terrain"))
    {
        float size = g_GameObject.GetSize();

        std::vector<std::vector<float>> smoothedHeights(size, std::vector<float>(size, 0.0f));

        for (int x = 0; x < size; x++)
        {
            for (int y = 0; y < size; y++)
            {
                float avgHeight = 0;
                int neighborCount = 0;

                // Calculate the average height of neighbors
                if (x + 1 < size) {
                    avgHeight += g_GameObject.GetHeight(x + 1, y);
                    neighborCount++;
                }
                if (x - 1 >= 0) {
                    avgHeight += g_GameObject.GetHeight(x - 1, y);
                    neighborCount++;
                }
                if (y + 1 < size) {
                    avgHeight += g_GameObject.GetHeight(x, y + 1);
                    neighborCount++;
                }
                if (y - 1 >= 0) {
                    avgHeight += g_GameObject.GetHeight(x, y - 1);
                    neighborCount++;
                }

                if (neighborCount > 0) {
                    avgHeight /= neighborCount;

                    // Blend the averaged height with the original height
                    float originalHeight = g_GameObject.GetHeight(x, y);
                    float newHeight = 0.1 * avgHeight + (1 - 0.1) * originalHeight;

                    // Store the smoothed height in the temporary buffer
                    smoothedHeights[x][y] = newHeight;
                }
                else {
                    // If no neighbors, keep the original height
                    smoothedHeights[x][y] = g_GameObject.GetHeight(x, y);
                }
            }
        }

        // Apply the smoothed heights to the terrain
        for (int x = 0; x < size; x++)
        {
            for (int y = 0; y < size; y++)
            {
                g_GameObject.SetHeight(x, y, smoothedHeights[x][y]);
            }
        }
        smoothedHeights.clear();
        smoothedHeights.shrink_to_fit();
        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
        RegenerateHeightMap();
    }
    ImGui::SameLine();
    ImGui::Text("Key S");
    HelpMarker("Smooths out the jagged terrain left from hydraulic erosion, you may have to press it multiple times, can be activated via the S key");
    height += ImGui::GetWindowHeight();
    ImGui::End();
}

void HeightMapImage() {
    ImGui::SetNextWindowPos(ImVec2(width, height));
    ImGui::Begin("Height Map Image", 0, ImGuiWindowFlags_AlwaysAutoResize);
    HelpMarker("Height values represented as an image");
    ImGui::SetNextItemWidth(50 * ScaleX);
    ImGui::SliderInt("Image Size", &n_imageSize, 50, 500);
    if (n_frontTexture) {
        if (n_texture)
        {
            n_texture->Release();
            n_texture = nullptr;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
        srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = 1;

        HRESULT hr = g_pd3dDevice->CreateShaderResourceView(n_frontTexture, &srvd, &n_texture);
        if (FAILED(hr))
        {
            std::wcout << L"Shader Resource View is null!" << std::endl;
        }

        ImGui::Image((ImTextureID)(intptr_t)n_texture, ImVec2(n_imageSize * ScaleX, n_imageSize * ScaleY));

    }

    ImGui::End();
}

void File() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthR*0.85 * ScaleX, 80 * ScaleY), ImVec2(UIWidthR*0.85 * ScaleX, 200 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(screenWidth - (UIWidthR*0.85) * ScaleX, 0));
    ImGui::Begin("File", 0, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button("Load GLB file"))
    {
        if (OpenFileDialog(false)) { LoadTerrain(); }
    }
    HelpMarker("Loads a .glb file into the tool, as of now only ones that are made using this tool");
    if (ImGui::Button("Load Height Map Image"))
    {
        if(OpenFileDialog(true)) { LoadPNG(); }
    }
    HelpMarker("Loads a height map into the tool and creates a terrain from it. Adjust the height with the height slider in the noise function");
    if (ImGui::Button("Export Height Map"))
    {
        if (OpenFolderDialog()) { saveImage(); } 
    }
    HelpMarker("Exports height map as R16 png, type in the name in file explorer");
    if (ImGui::Button("Export Terrain"))
    {
        if(OpenFolderDialog()) { saveTerrain(); }
        
    }
    HelpMarker("Exports terrain as glb file, type in the name in file explorer");
    height = ImGui::GetWindowHeight();
    width = ImGui::GetWindowWidth();
    ImGui::End();
}

void Intro() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthL * 0.9 * ScaleX, 80 * ScaleY), ImVec2(UIWidthL * ScaleX, 1500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::Begin("Information and Controls", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextWrapped("This is a terrain generation tool. The two terrain generation modes Diamond Square and Noise can be accessed via the buttons along with the Colour.\n\nCTRL + Z to undo.\nCTRL+SHIFT+Z to redo.\n\nCamera movement:\nRight click will rotate the camera around the centre point\nControl + Left Click to move the centre point\nTo reset there is a button in view on the right hand side\nMiddle mouse button allows you to center on the location you clicked on\nScroll wheel to zoom in and out.\n\nTo directly edit slider values you can Control + Left Click on the sliders to access the value entry mode\n\n You can load in a heightmap(any) and glb model (only ones created from this tool for now) through the menu on the right along with exporting them. When loading into the tool you can adjust the model with the height slider but to start generating new models you will need to turn off the loaded model checkbox.\n\nYou can use the keys to access the brushes, \n1. Flatten Brush\n2. Raise Brush\n3. Lower Brush\n4. SmoothBrush\n5. Colour brush\nE. Erosion\nS. Model Wide Smooth\nP. Picker\n\nClick the arrow at the top of this window to minimize this window.");
    if (ImGui::Button("Showcase terrain values")) {
        n_PerlinOctaves = 5;
        n_Exponent = 4.861f;
        n_resolution = 565;
        n_PerlinFrequency = 0.007f;
        GenerateNoiseImageAndTerrain(true, true, false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Minimise Window"))
    {
        ImGui::SetWindowCollapsed("Information and Controls", true);
    }
    height = ImGui::GetWindowHeight();
    width = ImGui::GetWindowWidth();
    ImGui::End();
}

void View() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthR *0.85* ScaleX, 80 * ScaleY), ImVec2(UIWidthR*0.85 * ScaleX, 600 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(screenWidth - (0.85*UIWidthR) * ScaleX, height));
    ImGui::Begin("View", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Camera:");
    ImGui::Text("Position: %.2f, %.2f, %.2f", g_pCamera->GetPosition().x, g_pCamera->GetPosition().y, g_pCamera->GetPosition().z);
    ImGui::Text("Rotation P: %.2f, %.2f, %.2f", g_pCamera->GetTarget().x, g_pCamera->GetTarget().y, g_pCamera->GetTarget().z);
    ImGui::SetNextItemWidth(100);
    if (ImGui::Button("Reset Camera Rotation Position"))
    {
        float temp = g_GameObject.GetSize()/2;
        g_pCamera->SetTarget(XMFLOAT3(temp, n_height/2, temp));
        g_pCamera->SetDistance(temp * 3);
    }
    HelpMarker("Resets Camera Position to center of terrain.");
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("CTRL+LCLICK Sens", &c_sensitivity, 0.05f, 4.0f);
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Zoom Sens", &c_zoomSens, 0.01f, 4.0f);
    ImGui::Text("Model Data:");
    ImGui::Text("Vertex Count: %i", g_GameObject.GetVertexCount());
    ImGui::Text("Face Count: %i", g_GameObject.GetIndexCount()/3);
    ImGui::Checkbox("wireFrame", &wireframeEnabled);
    height += ImGui::GetWindowHeight();
    ImGui::End();
}

void Brush()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthR*0.88 *5*ScaleX, 80 * ScaleY), ImVec2(UIWidthR * 30 * ScaleX, 500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(width,0));
    ImGui::Begin("Terrain Brush", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Columns(4, "Columns", true);
    ImGui::Checkbox("Flatten Brush Enabled", &b_FlatBool);
    HelpMarker("Enables the flatten brush so it flattens on left click and drag left click");
    ImGui::SameLine();
    ImGui::Text("Key 1");
    if (b_FlatBool)
    {
        b_RaiseBool = false;
        b_SmoothBool = false;
        b_ColourBool = false;
        b_LowerBool = false;
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::InputInt("Flatten Size", &b_FlatSize);
        HelpMarker("Changes the size of the brush so the bigger the value the bigger the area it flattens");
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::SliderFloat("Max Height Diff", &b_MaxFlatDifference, 0.1f, 200.0f);
        HelpMarker("The maximum difference in the picked height and the surrounding height that will get flattened");
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::SliderFloat("Smoothness", &b_SmoothnessFactor, 0.0f, 1.0f);
        HelpMarker("Adds some variance to flatten so its not completely flat. 1 is completely flat");
    }
    ImGui::NextColumn();
    ImGui::Checkbox("Raise Brush Enabled", &b_RaiseBool);
    HelpMarker("Enables the Raise brush so it Raises/Extrudes on left click and drag left click");
    ImGui::SameLine();
    ImGui::Text("Key 2");
    if (b_RaiseBool)
    {
        b_FlatBool = false;
        b_SmoothBool = false;
        b_ColourBool = false;
        b_LowerBool = false;
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::InputInt("Raise Size", &b_RaiseSize);
        HelpMarker("Size of brush");
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::SliderFloat("Raise Rate", &b_RaiseRate, 0.0f, 10.0f);
        HelpMarker("The rate of added height");
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::SliderFloat("Raise Strength", &b_RaiseStrength, 0.01f, 1.0f);
        HelpMarker("How spread out the raise brush is, 0 raise less further from the center, 1.0 adds to all points equally.");
    }
    ImGui::NextColumn();
    ImGui::Checkbox("Lower Brush Enabled", &b_LowerBool);
    HelpMarker("Enables the Lower brush so it Lowers on left click and drag left click");
    ImGui::SameLine();
    ImGui::Text("Key 3");
    if (b_LowerBool)
    {
        b_RaiseBool = false;
        b_SmoothBool = false;
        b_ColourBool = false;
        b_FlatBool = false;
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::InputInt("Lower Size", &b_RaiseSize);
        HelpMarker("Size of brush");
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::SliderFloat("Lower Rate", &b_LowerRate, 0.0f, 10.0f);
        HelpMarker("The rate of detracted height");
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::SliderFloat("Lower Strength", &b_LowerStrength, 0.01f, 1.0f);
        HelpMarker("How spread out the lower brush is, 0 lowers less further from the center, 1.0 lowers all points equally.");
    }
    ImGui::NextColumn();
    ImGui::Checkbox("Smooth Brush Enabled", &b_SmoothBool);
    HelpMarker("Enables the Smooth brush so it Smooths on left click and drag left click");
    ImGui::SameLine();
    ImGui::Text("Key 4");
    if (b_SmoothBool)
    {
        b_RaiseBool = false;
        b_FlatBool = false;
        b_ColourBool = false;
        b_LowerBool = false;
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::InputInt("Smooth Size", &b_SmoothSize);
        HelpMarker("Size of brush");
        ImGui::SetNextItemWidth(120 * ScaleX);
        ImGui::SliderFloat("Smooth Rate", &b_SmoothRate, 0.01f, 1.0f);
        HelpMarker("How it averages heights, 0 doesnt take account any neighbors, and 1 averages all neighbors equally");
    }
    int tempwidth = width + ImGui::GetWindowWidth();
    height = ImGui::GetWindowHeight();
    ImGui::End();
}

void TerrainGeneration()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 80 * ScaleY), ImVec2(width, 1500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(0 ,height ));
    ImGui::Begin("Terrain", 0, ImGuiWindowFlags_AlwaysAutoResize);
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    int size = ImGui::GetContentRegionAvail().x;
    float buttonWidth = (size - 2 * spacing) / 3; 

    if (ImGui::Button("Diamond Square", ImVec2(buttonWidth, 0.0f)))
    {
        TerrainGenerationMode = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Noise", ImVec2(buttonWidth, 0.0f)))
    {
        TerrainGenerationMode = 2;
    }
    ImGui::SameLine();
    if (ImGui::Button("Colour", ImVec2(buttonWidth, 0.0f)))
    {
        TerrainGenerationMode = 3;
        b_ColourBool = true;
        b_RaiseBool = false;
        b_SmoothBool = false;
        b_FlatBool = false;
        b_LowerBool = false;
    }

    if (TerrainGenerationMode == 1)
    {
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        ImGui::SliderInt("Detail", &t_detail, 4, 11);
        HelpMarker("The size of the terrain, so a detail of 9 is 513x513 and 10 would be 1025x1025 and 11 is 2049x2049.");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        ImGui::SliderFloat("Roughness", &t_roughness, 0, 1);
        HelpMarker("The maximum difference between vertices, 0 is flat and 1 is very jagged.");
        if (ImGui::Button("Generate"))
        {
            ClearAllUR();
            g_GameObject.setDetailRoughness(t_detail, t_roughness);
            float temp = (pow(2, t_detail) + 1) / 2;
            g_pCamera->SetTarget(XMFLOAT3(temp, temp, temp));
            g_pCamera->SetDistance(temp * 3);
            g_GameObject.generateTerrain(n_pixels, n_backTexture, g_pd3dDevice);
            std::swap(n_frontTexture, n_backTexture);
            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
        }
        HelpMarker("Generates the terrain with a random seed using the detail and roughness value.");

        width = ImGui::GetWindowWidth();
        height = ImGui::GetWindowHeight();
    }

    else if (TerrainGenerationMode == 2)
    {

        ImGui::Checkbox("Loaded Model", &loadedModel);
        ImGui::Text("General Noise Parameters");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);

        if (ImGui::SliderInt("Size", &n_resolution, 32, 2048))
        {
            if (!loadedModel)
            {
                if (n_resolution < 32) { n_resolution = 32; }

                GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
            }
        }
        if (ImGui::IsItemDeactivated())
        {
            GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
        }
        HelpMarker("Size of the noise generated, the number is the width and depth of the terrain.");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::SliderFloat("Redistribution", &n_Exponent, 0.01f, 10.0f))
        {
            GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
        }
        if (ImGui::IsItemDeactivated())
        {
            GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
        }
        HelpMarker("Redistributes the terrain");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::SliderFloat("Height", &n_height, 1, 2048))
        {
            if (!loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
            }
            else if (loadedModel) {
                EditLoadedModel(2);
            }
        }
        if (ImGui::IsItemDeactivated() && !loadedModel)
        {
            GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
        }
        HelpMarker("The height of the noise terrain");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::SliderFloat("Floor Height", &n_floor, 0, 1023))
        {
            bool isHeld = ImGui::IsItemActive();
            bool isReleased = !ImGui::IsItemActive() && ImGui::IsItemDeactivatedAfterEdit();

            if (!loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
            }
        }
        if (ImGui::IsItemDeactivated() && !loadedModel)
        {
            GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
        }
        HelpMarker("The height value for the floor of the terrain");
        ImGui::Text("\nNoise Specific Options");
        ImGui::Text("\nPerlin Noise Parameters");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::Checkbox("P Enabled", &n_PerlinOn))
        {
            if (!loadedModel)
            {
                GenerateNoiseImageAndTerrain(FALSE, TRUE, false);
            }
        };

        HelpMarker("Enables perlin noise");
        if (n_PerlinOn && !loadedModel) {
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);
            if (ImGui::SliderInt("P Octaves", &n_PerlinOctaves, 1, 5)) {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            }
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("Layers noise at different scales over eachother, more octaves more detailed looking terrain");
            if (ImGui::Button("New P Seed"))
            {
                n_Perlin.SetSeed(RandomSeed());
                GenerateNoiseImageAndTerrain(TRUE, TRUE, false);
            }
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("Generates a new noise seed, changes how the noise looks");
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);
            if (ImGui::SliderFloat("P Intensity", &n_PerlinWeight, 0.01f, 1.0f))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            }
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("Controls how much this noise affects height when other noise is enabled");
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);

            if (ImGui::SliderFloat("P Scale", &n_PerlinFrequency, 0.001f, 0.1f))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            }
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("The resolution of the noise, lower is a smaller map, higher is a bigger map");
        }

        ImGui::Text("");
        ImGui::Text("Simplex Noise Parameters");
        if (ImGui::Checkbox("S Enabled", &n_SimplexOn))
        {
            if (!loadedModel) { GenerateNoiseImageAndTerrain(TRUE, TRUE, false); }
        };
        HelpMarker("Enables simplex noise");

        if (n_SimplexOn && !loadedModel) {
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);
            if (ImGui::SliderInt("S Octaves", &n_SimplexOctaves, 1, 5))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            };
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("Layers noise at different scales over eachother, more octaves more detailed looking terrain");
            if (ImGui::Button("New S Seed"))
            {
                n_Simplex.SetSeed(RandomSeed());
                GenerateNoiseImageAndTerrain(TRUE, TRUE, false);
            }
            HelpMarker("Generates a new noise seed, changes how the noise looks");
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);

            if (ImGui::SliderFloat("S Intensity", &n_SimplexWeight, 0.01f, 1.0f))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            }
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("Controls how much this noise affects height when other noise is enabled");
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);

            if (ImGui::SliderFloat("S Scale", &n_SimplexFrequency, 0.001f, 0.1f))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            }
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("The resolution of the noise, lower is a smaller map, higher is a bigger map");
        }

        ImGui::Text("");
        ImGui::Text("Cellular Noise Parameters");
        if (ImGui::Checkbox("C Enabled", &n_CellularOn))
        {
            if (!loadedModel) { GenerateNoiseImageAndTerrain(TRUE, TRUE, false); }
        };
        HelpMarker("Enables cellular noise");
        if (n_CellularOn && !loadedModel) {
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);
            if (ImGui::SliderInt("C Octaves", &n_CellularOctaves, 1, 5))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            };
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("Layers noise at different scales over eachother, more octaves more detailed looking terrain");
            if (ImGui::Button("New C Seed"))
            {
                n_Cellular.SetSeed(RandomSeed());
                GenerateNoiseImageAndTerrain(TRUE, TRUE, false);
            }
            HelpMarker("Generates a new noise seed, changes how the noise looks");
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);

            if (ImGui::SliderFloat("C Intensity", &n_CellularWeight, 0.01f, 1.0f))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            }
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("Controls how much this noise affects height when other noise is enabled");
            ImGui::SetNextItemWidth(SliderWidth * ScaleX);

            if (ImGui::SliderFloat("C Scale", &n_CellularFrequency, 0.001f, 0.1f))
            {
                if (!loadedModel)
                {
                    GenerateNoiseImageAndTerrain(TRUE, TRUE, TRUE);
                }
            };
            if (ImGui::IsItemDeactivated() && !loadedModel)
            {
                GenerateNoiseImageAndTerrain(TRUE, TRUE, FALSE);
            }
            HelpMarker("The resolution of the noise, lower is a smaller map, higher is a bigger map");
        }
        ImGui::Text("");
        if (ImGui::Button("Regenerate Noise"))
        {
            if (!loadedModel) { GenerateNoiseImageAndTerrain(TRUE, TRUE, false); }
        }

        HelpMarker("Incase of mistakes this will reset the noise to how it was before you changed it with the brushes or hydraulic erosion.");
    }

    else if (TerrainGenerationMode == 3)
    {
        if (ImGui::Checkbox("Colour brush", &b_ColourBool))
        {
            b_RaiseBool = false;
            b_SmoothBool = false;
            b_FlatBool = false;
            b_LowerBool = false;
            ColourPicker = false;
        }
        ImGui::SameLine();
        ImGui::Text("Key 5");
        if (ImGui::Checkbox("Colour Picker", &ColourPicker))
        {
            b_FlatBool = false;
            b_RaiseBool = false;
            b_LowerBool = false;
            b_SmoothBool = false;
            b_ColourBool = false;
        }
        ImGui::SameLine();
        ImGui::Text("Key P");
        HelpMarker("Click when the box is ticked and it will select a colour");
        ImGui::ColorPicker3("Paint Colour", &b_Colour.x);
        ImGui::InputInt("Colour Size", &b_ColourSize);
        HelpMarker("Size of brush");
        ImGui::NewLine();
        if (ImGui::Button("Set whole map colour"))
        {
            for (int x = 0; x < g_GameObject.GetSize(); x++) {
                for (int y = 0; y < g_GameObject.GetSize(); y++) {
                    g_GameObject.SetColour(x, y, b_Colour);
                }
            }
            g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
        }
    }
    width = ImGui::GetWindowWidth();
    height = ImGui::GetWindowHeight();
    ImGui::End();
}

void RenderDebugWindow(float deltaTime) {
    Intro();
    HydroErosionGUI();
    TerrainGeneration();
    Brush();
    HeightMapImage();
    File();
    View();
    
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
    if (mouseDownL) {
        if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000) && !ImGui::IsAnyItemActive())
        {
            if (b_FlatBool)
            {
                BrushFunc(true, 3, b_FlatSize);
            }
            else if (b_RaiseBool)
            {
                BrushFunc(true, 1, b_RaiseSize);
            }
            else if (b_LowerBool)
            {
                BrushFunc(true, 2, b_LowerSize);

            }
            else if (b_SmoothBool)
            {
                SmoothBrush(true, b_SmoothSize);
            }
            else if (b_ColourBool)
            {
                ColourBrush(true, b_ColourSize);
            }
        }
    }

    float t = calculateDeltaTime(); // capped at 60 fps
    if (t == 0.0f)
        return;


    if (normalDraw && !wireframeEnabled) {
        if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_BACK;
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthClipEnable = TRUE;

        g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
        g_pImmediateContext->RSSetState(rasterizerState);

        normalDraw = false;
        wireDraw = true;
    }

    if (wireDraw && wireframeEnabled) {
        if (rasterizerState) { rasterizerState->Release(); rasterizerState = nullptr; }
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.AntialiasedLineEnable = TRUE;
        rasterizerDesc.MultisampleEnable = TRUE;

        g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
        g_pImmediateContext->RSSetState(rasterizerState);

        wireDraw = false;
        normalDraw = true;
    }

    // Start the Dear ImGui frame
    ScaleX = (float)screenHeight / 1080.0f;
    ScaleY = (float)screenHeight / 1080.0f;




    static bool fontLoaded = false;
    if (!fontLoaded) {

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("calibri.ttf", 16.0f * ScaleX);
        ImGui::GetIO().Fonts->Build();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(ScaleX);
        fontLoaded = true;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // YOU will want to modify this for your own debug, controls etc - comment it out to hide the window

    RenderDebugWindow(t);


    // Clear the back buffer
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, Colors::MidnightBlue );

    // Clear the depth buffer to 1.0 (max depth)
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	// Update the cube transform, material etc. 
	g_GameObject.update(t, g_pImmediateContext, obj_rotation);

    // get the game object world transform
	XMMATRIX mGO = XMLoadFloat4x4(g_GameObject.getTransform());

    // store world and the view / projection in a constant buffer for the vertex shader to use
    ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose( mGO);
	cb1.mView = XMMatrixTranspose( g_pCamera->GetViewMatrix() );
	cb1.mProjection = XMMatrixTranspose( g_Projection );
	cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);
	g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, nullptr, &cb1, 0, 0 );

    
    setupLightForRender();

    // Render a cube
	g_pImmediateContext->VSSetShader( g_pVertexShader, nullptr, 0 );
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pConstantBuffer );

    g_pImmediateContext->PSSetShader( g_pPixelShader, nullptr, 0 );
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pLightConstantBuffer);

    ID3D11Buffer* materialCB = g_GameObject.getMaterialConstantBuffer();
    g_pImmediateContext->PSSetConstantBuffers(1, 1, &materialCB);

    g_GameObject.draw(g_pImmediateContext);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present our back buffer to our front buffer
    g_pSwapChain->Present( 0, 0 );
}



