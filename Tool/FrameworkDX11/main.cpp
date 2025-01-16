//--------------------------------------------------------------------------------------
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

Camera* g_pCamera;

#define SliderWidth 250
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

char                   s_fileName[32] = { "MeshFileName" };

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
int b_FlatSize = 2;
float b_MaxFlatDifference = 2.0f;
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
    g_hInst = hInstance;
    RECT rc = { 0, 0, screenWidth, screenHeight};

	g_viewWidth = screenWidth;
	g_viewHeight = screenHeight;

    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow(L"lWindowClass", L"DirectX 11",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, SW_MAXIMIZE );

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

float GenerateNoiseThread(FastNoiseLite* noise, int* octaves, int* x, int* y)
{
    float colour = 0;
    float weight = 1.0f;
    float scale = 1.0f;
    float totalDivide = 0;
    for (int i = 0; i < *octaves; i++) {
        colour += weight * noise->GetNoise((float)*x * scale , (float)*y * scale);
        totalDivide += weight;
        weight = weight * 0.5;
        scale = scale * 2;
    }
    return colour / totalDivide;
}

void GenerateNoise()
{
    delete[] n_pixels;
    n_pixels = nullptr;
    n_pixels = new uint32_t[n_resolution * n_resolution];
    memset(n_pixels, 0, sizeof(uint32_t) * n_resolution * n_resolution);

    for (int x = 0; x < n_resolution; x++) {
        for (int y = 0; y < n_resolution; y++) {
            float colour = 0;
            if (n_PerlinOn) { colour += GenerateNoiseThread(&n_Perlin, &n_PerlinOctaves, &x, &y) * n_PerlinWeight; }
            if (n_SimplexOn) { colour += GenerateNoiseThread(&n_Simplex, &n_SimplexOctaves, &x, &y) * n_SimplexWeight; }
            if (n_CellularOn) { colour += GenerateNoiseThread(&n_Cellular, &n_CellularOctaves, &x, &y) * n_CellularWeight; }
            
            float maxWeightSum = n_PerlinOn * n_PerlinWeight +
                n_SimplexOn * n_SimplexWeight +
                n_CellularOn * n_CellularWeight;

            float normalizedColour = (colour / maxWeightSum) * 0.5f + 0.5f;  // Normalize to [0, 1]
            int temp = (int)(normalizedColour * 255);

            temp = max(0 , min(255, temp)); // Clamp for safety
            n_pixels[x + y * n_resolution] = (temp) | (temp << 8) | (temp << 16) | (255 << 24);
        }
    }
    if (n_texture) {
        n_texture->Release();
        n_texture = nullptr;
    }

    D3D11_SUBRESOURCE_DATA subrecData = {};
    subrecData.pSysMem = n_pixels;
    subrecData.SysMemPitch = n_resolution * sizeof(uint32_t);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = n_resolution;
    desc.Height = n_resolution;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subrecData, &texture);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create texture\n");
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;

    hr = g_pd3dDevice->CreateShaderResourceView(texture, &srvd, &n_texture);
    if (FAILED(hr)) {
        texture->Release();
        return;
    }

    texture->Release();
    texture = nullptr;
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
    GenerateNoise();

    g_GameObject.setDetailRoughness(t_detail, t_roughness);
    g_GameObject.generateTerrain();
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
    float val = pow(2, t_detail) * 1.5;
    g_pCamera = new Camera(XMFLOAT3(val, val, val), XMFLOAT3(-1, -1, -1), XMFLOAT3(0.0f, 1.0f, 0.0f));

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

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    static bool mouseDownR = false;
    static bool mouseDownL = false;

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
        case 'W':
            g_pCamera->MoveForward(movement);  // Adjust distance as needed
            break;
        case 'A':
            g_pCamera->StrafeLeft(movement);  // Adjust distance as needed
            break;
        case 'S':
            g_pCamera->MoveBackward(movement);  // Adjust distance as needed
            break;
        case 'D':
            g_pCamera->StrafeRight(movement);  // Adjust distance as needed
            break;
        case 'E':
            g_pCamera->MoveUpward(movement);
            break;
        case 'Q':
            g_pCamera->MoveDownward(movement);
            break;
        }
        break;

    case WM_RBUTTONDOWN:
        mouseDownR = true;
        break;
    case WM_RBUTTONUP:
        mouseDownR = false;
        break;
    case WM_LBUTTONDOWN:
        mouseDownL = true;
        break;
    case WM_LBUTTONUP:
        mouseDownL = false;
        break;
    case WM_MOUSEMOVE:
    {
        if (mouseDownL) {
            XMFLOAT3 cameraPosition = g_pCamera->GetPosition();
            XMVECTOR rayOrigin = XMLoadFloat3(&cameraPosition);
            POINTS mousePos = MAKEPOINTS(lParam);
            XMFLOAT2 temp = XMFLOAT2(mousePos.x, mousePos.y);
            XMVECTOR cursorPos = XMLoadFloat2(&temp);
            RECT rect;
            GetClientRect(hWnd, &rect);
            XMMATRIX mGO = XMLoadFloat4x4(g_GameObject.getTransform());
            XMMATRIX World = XMMatrixTranspose(mGO);
            XMVECTOR rayDirection = XMVector3Normalize(XMVector3Unproject(cursorPos, 0, 0, (rect.right - rect.left), (rect.bottom - rect.top), 0.0f, 1.0f, g_Projection, g_pCamera->GetViewMatrix(), World) - rayOrigin);

            float stepSize = 0.5f;
            float currentDistance = 0.0f;
            int maxDistance = 5000;
            XMVECTOR Position = rayOrigin;
            while (currentDistance < maxDistance)
            {
                float x = XMVectorGetX(Position);
                float z = XMVectorGetZ(Position);
                float size = g_GameObject.GetSize();
                if (x <= size && x >= 0 && z <= size && z >= 0) {
                    float height = g_GameObject.GetHeight((int)x, int(z));
                    if (abs(height - XMVectorGetY(Position)) <= 1) {
                        g_GameObject.SetHeight((int)x, (int)z, height - 5);
                        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
                        break;
                    }
                }
                Position += rayDirection * stepSize;
                currentDistance += stepSize;
            }
            break;
        }
        if (mouseDownR)
        {
            // Get the dimensions of the window
            RECT rect;
            GetClientRect(hWnd, &rect);

            // Calculate the center position of the window
            POINT windowCenter;
            windowCenter.x = (rect.right - rect.left) / 2;
            windowCenter.y = (rect.bottom - rect.top) / 2;

            // Convert the client area point to screen coordinates
            ClientToScreen(hWnd, &windowCenter);

            // Get the current cursor position
            POINTS mousePos = MAKEPOINTS(lParam);
            POINT cursorPos = { mousePos.x, mousePos.y };
            ClientToScreen(hWnd, &cursorPos);

            // Calculate the delta from the window center
            POINT delta;
            delta.x = cursorPos.x - windowCenter.x;
            delta.y = -(cursorPos.y - windowCenter.y);

            // Update the camera with the delta
            // (You may need to convert POINT to POINTS or use the deltas as is)
            g_pCamera->UpdateLookAt({ static_cast<short>(delta.x), static_cast<short>(delta.y) });

            // Recenter the cursor
            SetCursorPos(windowCenter.x, windowCenter.y);
            break;
        }
    }
    break;
   
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
    std::string fileName = std::string(s_fileName) + ".obj";
    ofstream myfile(fileName);
    int CountVertices = 0;
    int CountIndices = 0;
    int IndexCount = g_GameObject.GetIndexCount();
    SimpleVertex* SV = g_GameObject.GetVertices();
    DWORD* Faces = g_GameObject.GetIndices();
    int size = g_GameObject.GetSize();
    if (myfile)
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                myfile << "v " << SV[CountVertices].Pos.x << ' ' << SV[CountVertices].Pos.y << ' ' << SV[CountVertices].Pos.z << "\n";
                myfile << "vn " << SV[CountVertices].Normal.x << ' ' << SV[CountVertices].Normal.y << ' ' << SV[CountVertices].Normal.z << "\n";
                CountVertices++;
            }
        }

        while(CountIndices < IndexCount) {
            myfile << "f " << Faces[CountIndices]+1 << ' ' << Faces[CountIndices+1]+1 << ' ' << Faces[CountIndices+2]+1 << "\n";
            CountIndices += 3;
        }
    }
}

void GenerateTerrainWithNoise()
{
    std::vector<std::vector<float>> map(n_resolution, std::vector<float>(n_resolution));
    for (int x = 0; x < n_resolution - 1; x++) {

        for (int y = 0; y < n_resolution - 1; y++) {
            float colour = 0;
            if (n_PerlinOn) { colour += GenerateNoiseThread(&n_Perlin, &n_PerlinOctaves, &x, &y) * n_PerlinWeight; }
            if (n_SimplexOn) { colour += GenerateNoiseThread(&n_Simplex, &n_SimplexOctaves, &x, &y) * n_SimplexWeight; }
            if (n_CellularOn) { colour += GenerateNoiseThread(&n_Cellular, &n_CellularOctaves, &x, &y) * n_CellularWeight; }

            float maxWeightSum = n_PerlinOn * n_PerlinWeight +
                n_SimplexOn * n_SimplexWeight +
                n_CellularOn * n_CellularWeight;

            float normalizedColour = (colour / maxWeightSum) * 0.5f + 0.5f;
            float temp = pow(normalizedColour, n_Exponent);
            temp = (normalizedColour * n_height);
            temp = max(n_floor, min(n_height, temp));
            map[x][y] = temp;
        }
    }
    g_GameObject.noiseGenerateTerrain(&map, n_resolution);
    g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    map.clear();
    map.shrink_to_fit();
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
void DSGUI() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthL * ScaleX, 80 * ScaleY), ImVec2(UIWidthL * ScaleX, 1500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(10 * ScaleX, 10 * ScaleY));
    ImGui::Begin("Diamond Square Terrain", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    ImGui::SliderInt("Detail", &t_detail, 4, 11);
    HelpMarker("The size of the terrain, it is the 2 to the power of the detail number + 1, so a detail of 9 is a terrain of 513x513 and 10 would be 1025x1025 ");
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    ImGui::SliderFloat("Roughness", &t_roughness, 0, 1);
    HelpMarker("0 is flat and 1 is very jagged");
    if (ImGui::Button("Generate"))
    {
        g_GameObject.setDetailRoughness(t_detail, t_roughness);
        g_GameObject.generateTerrain();
        g_GameObject.initMesh(g_pd3dDevice, g_pImmediateContext);
    }
    HelpMarker("");

    width = ImGui::GetWindowWidth();
    height = ImGui::GetWindowHeight();
    ImGui::End();
}
void HydroErosionGUI() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthL*0.8 * ScaleX, 80 * ScaleY), ImVec2(UIWidthL * ScaleX, 1500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2((10 + UIWidthL) * ScaleX, 10 * ScaleY));
    ImGui::Begin("Hydraulic Erosion", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    ImGui::SliderInt("Cycles", &h_cycles, 2000, 200000);
    HelpMarker("");
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
    height = ImGui::GetWindowHeight();

    ImGui::End();
}
void NoiseGUI() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthL * ScaleX, 300 * ScaleY), ImVec2(UIWidthL * ScaleX, 1500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(10 * ScaleX, 10 * ScaleY + (int)height));
    ImGui::Begin("Noise", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("General Noise Parameters");
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    if (ImGui::SliderInt("Size", &n_resolution, 32, 2048))
    {
        GenerateNoise();
        GenerateTerrainWithNoise();
    }
    HelpMarker("");
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    if (ImGui::SliderFloat("Redistribution", &n_Exponent, 0.01f, 10.0f))
    {
        GenerateTerrainWithNoise();
    }
    HelpMarker("");
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    if (ImGui::SliderFloat("Height", &n_height, 1, 1024))
    {
        GenerateTerrainWithNoise();
    }

    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    if (ImGui::SliderFloat("Floor/Sea Level", &n_floor, 0, 1023))
    {
        GenerateTerrainWithNoise();
    }
    ImGui::Text("");
    ImGui::Text("Perlin Noise Parameters");
    ImGui::SetNextItemWidth(SliderWidth * ScaleX);
    if (ImGui::Checkbox("P Enabled", &n_PerlinOn))
    {
        GenerateNoise();
        GenerateTerrainWithNoise();
    };
    HelpMarker("");
    if (n_PerlinOn) {
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::SliderInt("P Octaves", &n_PerlinOctaves, 1, 5)) {
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
        if (ImGui::Button("New P Seed"))
        {
            n_Perlin.SetSeed(RandomSeed());
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::SliderFloat("P Intensity", &n_PerlinWeight, 0.01f, 1.0f))
        {
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);

        if (ImGui::SliderFloat("P Scale", &n_PerlinFrequency, 0.001f, 0.1f))
        {
            n_Perlin.SetFrequency(n_PerlinFrequency);
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
    }

    ImGui::Text("");
    ImGui::Text("Simplex Noise Parameters");
    if (ImGui::Checkbox("S Enabled", &n_SimplexOn))
    {
        GenerateNoise();
        GenerateTerrainWithNoise();
    };
    HelpMarker("");

    if (n_SimplexOn) {
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::SliderInt("S Octaves", &n_SimplexOctaves, 1, 5)) 
        {
            GenerateNoise();
            GenerateTerrainWithNoise();
        };
        HelpMarker("");
        if (ImGui::Button("New S Seed"))
        {
            n_Simplex.SetSeed(RandomSeed()); 
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);

        if (ImGui::SliderFloat("S Intensity", &n_SimplexWeight, 0.01f, 1.0f))
        {
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);

        if (ImGui::SliderFloat("S Scale", &n_SimplexFrequency, 0.001f, 0.1f))
        {
            n_Simplex.SetFrequency(n_SimplexFrequency);
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
    }

    ImGui::Text("");
    ImGui::Text("Cellular Noise Parameters");
    if (ImGui::Checkbox("C Enabled", &n_CellularOn))
    {
        GenerateNoise();
        GenerateTerrainWithNoise();
    };
    HelpMarker("");
    if (n_CellularOn) {
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);
        if (ImGui::SliderInt("C Octaves", &n_CellularOctaves, 1, 5)) 
        {
            GenerateNoise();
            GenerateTerrainWithNoise();
        };
        HelpMarker("");
        if (ImGui::Button("New C Seed"))
        {
            n_Cellular.SetSeed(RandomSeed());
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
        ImGui::SetNextItemWidth(SliderWidth* ScaleX);

        if (ImGui::SliderFloat("C Intensity", &n_CellularWeight, 0.01f, 1.0f))
        {
            GenerateNoise();
            GenerateTerrainWithNoise();
        }
        HelpMarker("");
        ImGui::SetNextItemWidth(SliderWidth * ScaleX);

        if (ImGui::SliderFloat("C Scale", &n_CellularFrequency, 0.001f, 0.1f))
        {
            n_Cellular.SetFrequency(n_CellularFrequency);
            GenerateNoise();
            GenerateTerrainWithNoise();
        };
        HelpMarker("");
    }
    //if (ImGui::Button("Generate terrain with noise"))
    //{
    //    GenerateTerrainWithNoise();
    //}
    ImGui::End();
}
void NoiseImage() {
    ImGui::SetNextWindowPos(ImVec2((10 + UIWidthL) * ScaleX, 10 * ScaleY + height));
    ImGui::Begin("Noise Image", 0, ImGuiWindowFlags_AlwaysAutoResize);
    HelpMarker("");
    ImGui::Image((ImTextureID)(intptr_t)n_texture, ImVec2(150 * ScaleX, 150 * ScaleY));
    ImGui::End();
}
void File() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthR * ScaleX, 80 * ScaleY), ImVec2(UIWidthR * ScaleX, 90 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(screenWidth - (10 + UIWidthR) * ScaleX, 10 * ScaleY));
    ImGui::Begin("File", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetNextItemWidth(180);
    ImGui::InputText("File Name", s_fileName, 32);
    if (ImGui::Button("Export"))
    {
        saveTerrain();
    }
    HelpMarker("");
    height = ImGui::GetWindowHeight();
    ImGui::End();
}
void View() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthR * ScaleX, 80 * ScaleY), ImVec2(UIWidthR * ScaleX, 600 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(screenWidth - (10 + UIWidthR) * ScaleX, 10 * ScaleY + height));
    ImGui::Begin("View", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", g_pCamera->GetPosition().x, g_pCamera->GetPosition().y, g_pCamera->GetPosition().z);
    ImGui::InputFloat("Camera Speed", &movement);
    ImGui::Checkbox("wireFrame", &wireframeEnabled);
    height += ImGui::GetWindowHeight();
    ImGui::End();
}
void Brush() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(UIWidthR * ScaleX, 80 * ScaleY), ImVec2(UIWidthR * ScaleX, 500 * ScaleY));
    ImGui::SetNextWindowPos(ImVec2(screenWidth - (10 + UIWidthR) * ScaleX, 10 * ScaleY + height));
    ImGui::Begin("Terrain Brush", 0, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", g_pCamera->GetPosition().x, g_pCamera->GetPosition().y, g_pCamera->GetPosition().z);
    ImGui::Checkbox("Flatten Brush", &b_FlatBool);
    ImGui::InputInt("Flatten Size", &b_FlatSize);
    ImGui::InputFloat("Max Flatten Difference", &b_MaxFlatDifference);
    ImGui::End();
}


void RenderDebugWindow(float deltaTime) {
    DSGUI();
    NoiseGUI();
    HydroErosionGUI();
    NoiseImage();
    File();
    View();
    Brush();
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
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
    ScaleX = (float)screenWidth / 1920.0f;
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



