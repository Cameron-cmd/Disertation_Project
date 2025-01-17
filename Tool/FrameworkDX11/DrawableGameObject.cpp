#include "DrawableGameObject.h"
#include "FastNoiseLite.h"

using namespace std;
using namespace DirectX;


DrawableGameObject::DrawableGameObject()
{
	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;
	m_pTextureResourceView = nullptr;
	m_pSamplerLinear = nullptr;

	// Initialize the world matrix
	hydraulicErosionClass = HydraulicErosion();
	XMStoreFloat4x4(&m_World, XMMatrixIdentity());
}


DrawableGameObject::~DrawableGameObject()
{
	cleanup();
}

void DrawableGameObject::cleanup()
{
	if (m_pVertexBuffer)
		m_pVertexBuffer->Release();
	m_pVertexBuffer = nullptr;

	if (m_pIndexBuffer)
		m_pIndexBuffer->Release();
	m_pIndexBuffer = nullptr;

	if (m_pTextureResourceView)
		m_pTextureResourceView->Release();
	m_pTextureResourceView = nullptr;

	if (m_pSamplerLinear)
		m_pSamplerLinear->Release();
	m_pSamplerLinear = nullptr;

	if (m_pMaterialConstantBuffer)
		m_pMaterialConstantBuffer->Release();
	m_pMaterialConstantBuffer = nullptr;

	if (m_indicesArray) { delete[] m_indicesArray;  m_indicesArray = nullptr; }
	if (m_verticesArray) { delete[] m_verticesArray;  m_verticesArray = nullptr; }
}

void DrawableGameObject::setDetailRoughness(int detail, float roughness) {
	m_detail = detail;
	m_roughness = roughness;
}

void DrawableGameObject::printVertices()
{
	int count = 0;
	for (int x = 0; x < m_size; x++)
	{
		for (int y = 0; y < m_size; y++)
		{
			SimpleVertex SV = m_verticesArray[count];
			OutputDebugStringA((to_string(SV.Pos.x) + " " + to_string(SV.Pos.y) + " " + to_string(SV.Pos.z) + "\n ").c_str());
			count++;
		}
	}
}

void DrawableGameObject::hydraulicErosion(int cycles)
{
	m_map = hydraulicErosionClass.Erode(m_map, cycles, m_size);
}

void DrawableGameObject::noiseGenerateTerrain(std::vector<std::vector<float>>* pMap, int size)
{
	m_map.clear();
	m_map = *pMap;
	m_size = size - 1;
	m_max = size - 2;
}

void DrawableGameObject::generateTerrain()
{
	newTerrain = TerrainGenDS(m_detail);
	newTerrain.Generate(m_roughness);
	m_map = newTerrain._map;
	m_size = newTerrain._size;
	m_max = newTerrain._max;
	hydraulicErosionClass.scale = m_max;
}

void DrawableGameObject::printIndicies()
{
	int count = 0;
	while (count < m_IndexCount)
	{
		OutputDebugStringA((to_string(m_indicesArray[count])+" ").c_str());
		count++;
		OutputDebugStringA((to_string(m_indicesArray[count]) + " ").c_str());
		count++;
		OutputDebugStringA((to_string(m_indicesArray[count]) + "\n").c_str());
		count++;
	}
}

float DrawableGameObject::RatioValueConverter(float old_min, float old_max, float new_min, float new_max, float value)
{
	return (((value - old_min) / (old_max - old_min)) * (new_max - new_min) + new_min);
}

HRESULT DrawableGameObject::initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext)
{
	if (m_indicesArray) { delete[] m_indicesArray;  m_indicesArray = nullptr; }
	if (m_verticesArray) { delete[] m_verticesArray;  m_verticesArray = nullptr; }
	if (m_pIndexBuffer) { m_pIndexBuffer->Release(); m_pIndexBuffer = nullptr; }
	if (m_pVertexBuffer) { m_pVertexBuffer->Release(); m_pVertexBuffer = nullptr; }
	if (m_pMaterialConstantBuffer) { m_pMaterialConstantBuffer->Release(); m_pMaterialConstantBuffer = nullptr; }
	if (m_pSamplerLinear) { m_pSamplerLinear->Release(); m_pSamplerLinear = nullptr; }

	int vertexCount = 0;
	int indexCount = 0;

	vertexCount = m_size * m_size;
	indexCount = (((m_size-1) * 2) * (m_size - 1)) * 3;
	m_verticesArray = new SimpleVertex[vertexCount];
	m_indicesArray = new DWORD[indexCount];
	m_IndexCount = indexCount;
	m_VertexCount = vertexCount;
	int count = 0;
	for (int x = 0; x < m_size; x++)
	{
		for (int y = 0; y < m_size; y++)
		{
			SimpleVertex SV;
			SV.Pos = XMFLOAT3(x, m_map[x][y], y);
			SV.Colour = XMFLOAT3(0.20f, 0.20f, 0.20f);
			m_verticesArray[count] = SV;
			count++;
		}
	}
	
	int ind1 = 0;
	int ind2 = 1;
	count = 0;
	int remainder = 0;
	while (count < indexCount)
	{
		//OutputDebugStringA((to_string(count) + "\n").c_str());
		if (ind1 % m_max == remainder && ind1 != 0) { ind1++; ind2++; remainder++;}
		m_indicesArray[count] = (DWORD)ind1;
		count++;
		m_indicesArray[count] = (DWORD)ind2;
		count++;
		m_indicesArray[count] = (DWORD)(ind2 + m_size -1);
		count++;
		XMFLOAT3 edgef1 = XMFLOAT3(m_verticesArray[ind1].Pos.x - m_verticesArray[ind2].Pos.x, m_verticesArray[ind1].Pos.y - m_verticesArray[ind2].Pos.y, m_verticesArray[ind1].Pos.z - m_verticesArray[ind2].Pos.z);
		XMVECTOR edge1 = XMLoadFloat3(&edgef1);
		XMFLOAT3 edgef2 = XMFLOAT3(m_verticesArray[ind1].Pos.x - m_verticesArray[ind2 + m_size - 1].Pos.x, m_verticesArray[ind1].Pos.y - m_verticesArray[ind2 + m_size - 1].Pos.y, m_verticesArray[ind1].Pos.z - m_verticesArray[ind2 + m_size - 1].Pos.z);
		XMVECTOR edge2 = XMLoadFloat3(&edgef2);
		XMVECTOR normal = XMVector3Cross(edge1, edge2);
		XMFLOAT3 normFloat = XMFLOAT3(XMVectorGetX(normal), XMVectorGetY(normal), XMVectorGetZ(normal));

		m_verticesArray[ind1].Normal = XMFLOAT3(normFloat.x + m_verticesArray[ind1].Normal.x, normFloat.y + m_verticesArray[ind1].Normal.y, normFloat.z + m_verticesArray[ind1].Normal.z);
		m_verticesArray[ind2].Normal = XMFLOAT3(normFloat.x + m_verticesArray[ind2].Normal.x, normFloat.y + m_verticesArray[ind2].Normal.y, normFloat.z + m_verticesArray[ind2].Normal.z);
		m_verticesArray[ind2 + m_size - 1].Normal = XMFLOAT3(normFloat.x + m_verticesArray[ind2 + m_size - 1].Normal.x, normFloat.y + m_verticesArray[ind2 + m_size - 1].Normal.y, normFloat.z + m_verticesArray[ind2 + m_size - 1].Normal.z);

		m_indicesArray[count] = (DWORD)(ind2);
		count++;
		m_indicesArray[count] = (DWORD)(ind2 + m_size);
		count++;
		m_indicesArray[count] = (DWORD)(ind2 + m_size - 1);
		count++;

		edgef1 = XMFLOAT3(m_verticesArray[ind2].Pos.x - m_verticesArray[ind2 + m_size].Pos.x, m_verticesArray[ind2].Pos.y - m_verticesArray[ind2 + m_size].Pos.y, m_verticesArray[ind2].Pos.z - m_verticesArray[ind2 + m_size].Pos.z);
		edge1 = XMLoadFloat3(&edgef1);
		edgef2 = XMFLOAT3(m_verticesArray[ind2].Pos.x - m_verticesArray[ind2 + m_size - 1].Pos.x, m_verticesArray[ind2].Pos.y - m_verticesArray[ind2 + m_size - 1].Pos.y, m_verticesArray[ind2].Pos.z - m_verticesArray[ind2 + m_size - 1].Pos.z);
		edge2 = XMLoadFloat3(&edgef2);
		normal = XMVector3Cross(edge1, edge2);
		normFloat = XMFLOAT3(XMVectorGetX(normal), XMVectorGetY(normal), XMVectorGetZ(normal));

		m_verticesArray[ind2].Normal = XMFLOAT3(normFloat.x + m_verticesArray[ind2].Normal.x, normFloat.y + m_verticesArray[ind2].Normal.y, normFloat.z + m_verticesArray[ind2].Normal.z);
		m_verticesArray[ind2 + m_size].Normal = XMFLOAT3(normFloat.x + m_verticesArray[ind2 + m_size].Normal.x, normFloat.y + m_verticesArray[ind2 + m_size].Normal.y, normFloat.z + m_verticesArray[ind2 + m_size].Normal.z);
		m_verticesArray[ind2 + m_size - 1].Normal = XMFLOAT3(normFloat.x + m_verticesArray[ind2 + m_size - 1].Normal.x, normFloat.y + m_verticesArray[ind2 + m_size - 1].Normal.y, normFloat.z + m_verticesArray[ind2 + m_size - 1].Normal.z);

		ind1 += 1;
		ind2 += 1;
	}
	count = 0;
	for (int x = 0; x < m_size; x++)
	{
		for (int y = 0; y < m_size; y++)
		{
			XMVECTOR temp = XMVector3Normalize(XMVECTOR(XMLoadFloat3(&m_verticesArray[count].Normal)));
			m_verticesArray[count].Normal = XMFLOAT3(XMVectorGetX(temp*-1), XMVectorGetY(temp*-1), XMVectorGetZ(temp*-1));
			count++;
		}
	}
	//ifstream myfile;
	//myfile.open("Resources//City1 Block 1.obj", ios::in);
	//if (!myfile.fail())
	//{
	//	string line;
	//	while (getline(myfile, line)) {
	//		if (line[0] == 'v') { vertexCount++; }
	//		if (line[0] == 'f') { indexCount++; indexCount++; indexCount++; }
	//	}
	//	line.clear();
	//	m_IndexCount = indexCount;
	//	vertices = new SimpleVertex[vertexCount];
	//	m_indicesArray = new WORD[indexCount];
	//	int vertIndex = 0;
	//	int indIndex = 0;
	//	float indcount = 0;
	//	myfile.clear();                 // clear fail and eof bits
	//	myfile.seekg(0, std::ios::beg); // back to the start!
	//	while (getline(myfile, line)) {
	//		if (!line.empty() && line[0] == 'v') {
	//			XMFLOAT3 vertex;
	//			sscanf(line.c_str(), "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
	//			SimpleVertex SV;
	//			SV.Pos = vertex;
	//			SV.Normal = XMFLOAT3(0, 0, 0);
	//			SV.TexCoord = XMFLOAT2(0, 0);
	//			vertices[vertIndex] = SimpleVertex(SV);
	//			//OutputDebugStringA((to_string(vertices[vertIndex].Pos.x) + " ").c_str());
	//			//OutputDebugStringA((to_string(vertices[vertIndex].Pos.y) + " ").c_str());
	//			//OutputDebugStringA((to_string(vertices[vertIndex].Pos.z)+"\n").c_str());
	//			vertIndex++;
	//			continue;
	//		}
	//		if (!line.empty() && line[0] == 'f') {
	//			XMINT3 face;
	//			sscanf(line.c_str(), "f %i %i %i", &face.x, &face.y, &face.z);
	//			m_indicesArray[indIndex] = face.x - 1;
	//			//OutputDebugStringA((to_string(m_indicesArray[indIndex])+" ").c_str());
	//			indIndex++;
	//			m_indicesArray[indIndex] = face.y - 1;
	//			//OutputDebugStringA((to_string(m_indicesArray[indIndex]) + " ").c_str());
	//			indIndex++;
	//			m_indicesArray[indIndex] = face.z - 1;
	//			//OutputDebugStringA((to_string(m_indicesArray[indIndex]) + "\n").c_str());
	//			indIndex++;

	//		}
	//	}

	//}
	//}

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * vertexCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = m_verticesArray;
	HRESULT hr = pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);


	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(DWORD) * indexCount;        // 36 vertices needed for 12 triangles in a triangle list
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = m_indicesArray;
	hr = pd3dDevice->CreateBuffer(&bd, &InitData, &m_pIndexBuffer);
	if (FAILED(hr))
		return hr;

	// Set index buffer
	pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);

	m_material.Material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_material.Material.Specular = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
	m_material.Material.SpecularPower = 32.0f;
	m_material.Material.UseTexture = true;

	// Create the material constant buffer
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(MaterialPropertiesConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = pd3dDevice->CreateBuffer(&bd, nullptr, &m_pMaterialConstantBuffer);
	if (FAILED(hr))
		return hr;
	return hr;
}

void DrawableGameObject::setPosition(XMFLOAT3 position)
{
	m_position = position;
}

void DrawableGameObject::update(float t, ID3D11DeviceContext* pContext, XMMATRIX rotation)
{
	static float cummulativeTime = 0.1;
	cummulativeTime += t;

	// Cube:  Rotate around origin
	XMMATRIX mSpin = XMMatrixRotationY(cummulativeTime);

	XMMATRIX mTranslate = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX world = mTranslate * rotation;
	XMStoreFloat4x4(&m_World, world);

	pContext->UpdateSubresource(m_pMaterialConstantBuffer, 0, nullptr, &m_material, 0, 0);
}

void DrawableGameObject::draw(ID3D11DeviceContext* pContext)
{
	
	pContext->PSSetShaderResources(0, 1, &m_pTextureResourceView);
	pContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

	pContext->DrawIndexed(m_IndexCount, 0, 0);
}