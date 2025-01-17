#pragma once


#include <DirectXMath.h>
#include <algorithm>
#include <windows.h>
#include <windowsx.h>
#include "constants.h"

using namespace DirectX;

class Camera
{
public:
    Camera(XMFLOAT3 targetIn, float distanceIn, XMFLOAT3 upIn )
    {
        target = targetIn;
        distance = distanceIn;
        up = upIn;
        yaw = 0.0f;
        pitch = 0.0f;

        XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
        UpdatePosition();
        UpdateLookDir();
    }

    void Rotate(float deltaYaw, float deltaPitch) 
    {
        yaw += deltaYaw;
        pitch = std::clamp(pitch + deltaPitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);
        UpdatePosition();
    }

    void Zoom(float deltaDistance)
    {
        distance = max(1.0f, distance + deltaDistance);
        UpdatePosition();
    }

    XMFLOAT3 GetPosition() { return position; }
    XMFLOAT3 GetTarget() { return target; }
    void SetTarget(XMFLOAT3 newTarget) { target = newTarget; UpdatePosition(); UpdateLookDir(); }
    void SetOnlyTarget (XMFLOAT3 newTarget) { target = newTarget; }
    XMMATRIX GetViewMatrix() const
    {
        return XMLoadFloat4x4(&viewMatrix);
    }

    void UpdatePositionWithTargetMove(const XMFLOAT3& targetMoveDelta)
    {
        // Move the camera by the same amount as the target
        position.x += targetMoveDelta.x;
        position.y += targetMoveDelta.y;
        position.z += targetMoveDelta.z;

        // Recalculate the view matrix (the camera will still look at the target)
        UpdateViewMatrix();
    }

    XMFLOAT3 GetRight() const
    {
        XMVECTOR rightVec = XMVector3Cross(XMLoadFloat3(&lookDir), XMLoadFloat3(&up));
        XMFLOAT3 right;
        XMStoreFloat3(&right, XMVector3Normalize(rightVec));
        return right;
    }

    XMFLOAT3 GetUp() const
    {
        // You can directly return the up vector (as it doesn't change much)
        return up;
    }

    XMFLOAT3 GetLookDir() { return lookDir; }
    void SetDistance(float distanceIn) { distance = distanceIn; UpdatePosition(); }
private:

    void UpdateLookDir()
    {
        // Calculate the initial look direction based on the current yaw and pitch
        float x = distance * cosf(pitch) * sinf(yaw);
        float y = distance * sinf(pitch);
        float z = distance * cosf(pitch) * cosf(yaw);

        XMVECTOR directionVec = XMVectorSet(x, y, z, 0.0f);
        XMVECTOR targetVec = XMLoadFloat3(&target);

        // Calculate the look direction from the camera to the target
        XMVECTOR lookDirVec = XMVector3Normalize(targetVec - directionVec);

        // Store it in the lookDir variable
        XMStoreFloat3(&lookDir, lookDirVec);
    }

    void UpdateViewMatrix()
    {
        XMStoreFloat4x4(&viewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&position), XMLoadFloat3(&target), XMLoadFloat3(&up)));
    }

    void UpdatePosition()
    {
        float x = distance * cosf(pitch) * sinf(yaw);
        float y = distance * sinf(pitch);
        float z = distance * cosf(pitch) * cosf(yaw);

        position = { target.x + x, target.y + y, target.z + z };

        UpdateViewMatrix();
    }

    XMFLOAT3 target;
    float distance;
    float pitch;
    float yaw;
    XMFLOAT3 position;
    XMFLOAT3 up;
    XMFLOAT3 lookDir;
    mutable XMFLOAT4X4 viewMatrix;
};

