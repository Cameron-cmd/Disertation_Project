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
    Camera(XMFLOAT3 posIn, XMFLOAT3 lookDirIn, XMFLOAT3 upIn )
    {
        position = posIn;
        lookDir = lookDirIn;
        up = upIn;

        XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
    }
    void SetPosition(XMFLOAT3 pos) { position = pos; }
    XMFLOAT3 GetPosition() { return position; }

    void MoveUpward(float distance)
    {
        // Get the current up vector
        XMVECTOR upVec = XMLoadFloat3(&up);
        XMVECTOR posVec = XMLoadFloat3(&position);

        // Move upwards by adding the scaled up vector to the position
        XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(distance), upVec, posVec));
    }

    void MoveDownward(float distance)
    {
        MoveUpward(-distance);
    }

    void MoveForward(float distance)
    {
        // Get the normalized forward vector (camera's look direction)
        XMVECTOR forwardVec = XMVector3Normalize(XMLoadFloat3(&lookDir));
        XMVECTOR posVec = XMLoadFloat3(&position);

        // Move in the direction the camera is facing
        XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(distance), forwardVec, posVec));
    }

    void StrafeLeft(float distance)
    {
        // Get the current look direction and up vector
        XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
        XMVECTOR upVec = XMLoadFloat3(&up);

        // Calculate the right vector (side vector of the camera)
        XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(upVec, lookDirVec));
        XMVECTOR posVec = XMLoadFloat3(&position);

        // Move left by moving opposite to the right vector
        XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(-distance), rightVec, posVec));
    }

    void MoveBackward(float distance)
    {
        // Call MoveForward with negative distance to move backward
        MoveForward(-distance);
    }

    void StrafeRight(float distance)
    {
        // Call StrafeLeft with negative distance to move right
        StrafeLeft(-distance);
    }

    void UpdateLookAt(POINTS delta)
    {
        // Sensitivity factor for mouse movement
        const float sensitivity = 0.001f;
    
        // Apply sensitivity
        float dx = delta.x * sensitivity; // Yaw change
        float dy = delta.y * sensitivity; // Pitch change
    
    
        // Get the current look direction and up vector
        // Convert look direction to spherical coordinates
        float yaw = atan2f(lookDir.x, lookDir.z);
        float pitch = asinf(lookDir.y);

        // Update yaw and pitch based on mouse movement
        yaw += dx;
        pitch =  std::clamp(pitch + dy, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f);

        // Recalculate look direction using spherical coordinates
        XMVECTOR newLookDir = XMVectorSet(
            cosf(pitch) * sinf(yaw),
            sinf(pitch),
            cosf(pitch) * cosf(yaw),
            0.0f
        );

        newLookDir = XMVector3Normalize(newLookDir);

        // Update class variables
        XMStoreFloat3(&lookDir, newLookDir);
    }

    void Update() { UpdateViewMatrix(); }

    XMMATRIX GetViewMatrix() const
    {
        UpdateViewMatrix();
        return XMLoadFloat4x4(&viewMatrix);
    }

private:
    void UpdateViewMatrix() const
    {
        // Calculate the look-at point based on the position and look direction
        XMVECTOR posVec = XMLoadFloat3(&position);
        XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
        XMVECTOR lookAtPoint = posVec + lookDirVec; // This is the new look-at point

        // Update the view matrix to look from the camera's position to the look-at point
        XMStoreFloat4x4(&viewMatrix, XMMatrixLookAtLH(posVec, lookAtPoint, XMLoadFloat3(&up)));
    }

    XMFLOAT3 position;
    XMFLOAT3 lookDir;
    XMFLOAT3 up;
    mutable XMFLOAT4X4 viewMatrix;
};

