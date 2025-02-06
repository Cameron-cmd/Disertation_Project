#pragma once


#include <DirectXMath.h>
#include <algorithm>
#include <windows.h>
#include <windowsx.h>
#include "constants.h"
#include <string>

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
        if (distance < 0.0f)
        {
            deltaYaw = deltaYaw;
            deltaPitch = -deltaPitch;
        }
        yaw += deltaYaw;
        pitch = std::clamp(pitch + deltaPitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);
        UpdatePosition();
    }

    void Zoom(float deltaDistance)
    {
        distance += deltaDistance;
        UpdatePosition();
    }

    XMFLOAT3 GetPosition() { return position; }
    XMFLOAT3 GetTarget() { return target; }
    void SetTarget(XMFLOAT3 newTarget) { target = newTarget; UpdatePosition(); }
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
        XMVECTOR directionVec = XMLoadFloat3(&position) - XMLoadFloat3(&target);
        XMVECTOR lookDirVec = XMVector3Normalize(directionVec);

        if (distance < 0.0f)
        {
            lookDirVec = -lookDirVec;
        }

        XMStoreFloat3(&lookDir, lookDirVec);

        // Debug output
        std::string debugMsg = "LookDir: (" + std::to_string(lookDir.x) +
            ", " + std::to_string(lookDir.y) +
            ", " + std::to_string(lookDir.z) + ")\n";
        OutputDebugStringA(debugMsg.c_str());
    }

    void UpdateViewMatrix()
    {
        XMVECTOR positionVec = XMLoadFloat3(&position);
        XMVECTOR targetVec = XMLoadFloat3(&target);
        XMVECTOR upVec = XMLoadFloat3(&up);

        if (distance < 0.0f)
        {
            // When distance is negative, the camera should look away from the target
            targetVec = positionVec + XMLoadFloat3(&lookDir);
        }

        XMStoreFloat4x4(&viewMatrix, XMMatrixLookAtLH(positionVec, targetVec, upVec));
    }

    void UpdatePosition()
    {
        float x = distance * cosf(pitch) * sinf(yaw);
        float y = distance * sinf(pitch);
        float z = distance * cosf(pitch) * cosf(yaw);

        position = { target.x + x, target.y + y, target.z + z };

        // Debug output
        std::string debugMsg = "Distance: " + std::to_string(distance) +
            ", Yaw: " + std::to_string(yaw) +
            ", Pitch: " + std::to_string(pitch) + "\n";
        OutputDebugStringA(debugMsg.c_str());

        debugMsg = "Position: (" + std::to_string(position.x) +
            ", " + std::to_string(position.y) +
            ", " + std::to_string(position.z) + ")\n";
        OutputDebugStringA(debugMsg.c_str());
        UpdateLookDir();
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

