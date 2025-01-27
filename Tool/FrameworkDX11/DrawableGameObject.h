#pragma once

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxcolors.h>
#include <DirectXCollision.h>
#include "DDSTextureLoader.h"
#include "resource.h"
#include <iostream>
#include <string>
#include "structures.h"
#include <fstream>
#include <vector>
#include "TerrainGenDS.h";
#include "HydraulicErosion.h";

using namespace DirectX;

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT3 Colour;

	SimpleVertex() : Pos(XMFLOAT3(0, 0, 0)), Normal(XMFLOAT3(0, 0, 0)), Colour(XMFLOAT3(0.5f, 0.8f, 0.3f)) {}
};

class DrawableGameObject
{
public:
	DrawableGameObject();
	~DrawableGameObject();

	void cleanup();

	

	HRESULT								initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext);
	void								update(float t, ID3D11DeviceContext* pContext, XMMATRIX rotation);
	void								draw(ID3D11DeviceContext* pContext);
	ID3D11Buffer*						getVertexBuffer() { return m_pVertexBuffer; }
	ID3D11Buffer*						getIndexBuffer() { return m_pIndexBuffer; }
	ID3D11ShaderResourceView**			getTextureResourceView() { return &m_pTextureResourceView; 	}
	XMFLOAT4X4*							getTransform() { return &m_World; }
	ID3D11SamplerState**				getTextureSamplerState() { return &m_pSamplerLinear; }
	ID3D11Buffer*						getMaterialConstantBuffer() { return m_pMaterialConstantBuffer;}
	void								setPosition(XMFLOAT3 position);
	void								setDetailRoughness(int detail, float roughness);
	
	void								printIndicies();
	void								printVertices();
	void								hydraulicErosion(int cycles);
	void								generateTerrain(uint32_t*& pixels, ID3D11Texture2D*& n_texture, ID3D11Device* pd3dDevice);
	void								noiseGenerateTerrain(std::vector<std::vector<float>>* pMap, int size);
	void								loadTerrain(std::vector<std::vector<float>>* pMap, std::vector<std::vector<XMFLOAT3>>* colourMap, int size);
	float								RatioValueConverter(float old_min, float old_max, float _min, float new_max, float value);
	SimpleVertex*						GetVertices() { return m_verticesArray; }
	DWORD*								GetIndices() { return m_indicesArray; }
	INT									GetSize() { return m_size; }
	INT									GetIndexCount() { return m_IndexCount; }
	INT									GetVertexCount() { return m_VertexCount; }
	float GetHeight(int x, int y) { return m_map[x][y]; }
	void SetHeight(int x, int y, float z) { m_map[x][y] = z; }
	XMFLOAT3 GetColour(int x, int y) { return m_colourMap[x][y]; }
	void SetColour(int x, int y, XMFLOAT3 z) { m_colourMap[x][y] = z; }
	std::vector<std::vector<XMFLOAT3>>* GetColourMap() { return &m_colourMap; }
	HydraulicErosion					hydraulicErosionClass;
private:
	
	XMFLOAT4X4							m_World;

	ID3D11Buffer*						m_pVertexBuffer;
	ID3D11Buffer*						m_pIndexBuffer;
	ID3D11ShaderResourceView*			m_pTextureResourceView;
	ID3D11SamplerState *				m_pSamplerLinear;
	MaterialPropertiesConstantBuffer	m_material;
	ID3D11Buffer*						m_pMaterialConstantBuffer = nullptr;
	XMFLOAT3							m_position;
	INT									m_IndexCount = 0;
	INT									m_VertexCount = 0;

	SimpleVertex*						m_verticesArray;
	DWORD*								m_indicesArray;

	INT									m_size;
	INT									m_max;
	INT									m_detail;
	FLOAT								m_roughness;
	std::vector<std::vector<float>>		m_map;
	std::vector<std::vector<XMFLOAT3>> m_colourMap;
	TerrainGenDS						newTerrain;
};

