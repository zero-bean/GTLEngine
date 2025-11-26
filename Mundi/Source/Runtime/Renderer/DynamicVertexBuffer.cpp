#include "pch.h"
#include "DynamicVertexBuffer.h"

FDynamicVertexBuffer::~FDynamicVertexBuffer()
{
    Release();
}

void FDynamicVertexBuffer::Initialize(D3D11RHI* InRHI, uint32 Size)
{
    RHIDevice = InRHI;
    TotalSize = Size;
    CurrentOffset = 0;

    RHIDevice->CreateVertexBufferWithSize(RHIDevice->GetDevice(), &VertexBuffer, TotalSize, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void* FDynamicVertexBuffer::Lock(uint32 VertexCount, uint32 Stride, uint32& OutVertexOffset)
{
    uint32 SizeNeeded = Stride * VertexCount;
    if (SizeNeeded > TotalSize)
    {
        UE_LOG("[FDynamicVertexBuffer/Lock] Requesting size larget than total buffer size");
        return nullptr;
    }

    // offset을 stride 배수로 맞춤
    if (CurrentOffset % Stride != 0)
    {
        CurrentOffset += (Stride - (CurrentOffset % Stride));
    }

    D3D11_MAP MapType = D3D11_MAP_WRITE_NO_OVERWRITE;
    // 공간 부족 시 DICARD
    if (CurrentOffset + SizeNeeded > TotalSize)
    {
        MapType = D3D11_MAP_WRITE_DISCARD;
        CurrentOffset = 0;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource = {};
    RHIDevice->GetDeviceContext()->Map(VertexBuffer, 0, MapType, 0, &MappedResource);

    // 포인트 연산으로 메모리 위치 반환
    uint8* DataPtr = static_cast<uint8*>(MappedResource.pData) + CurrentOffset;

    // 현재 오프셋은 스트라이드의 배수이기 때문에 나누면 Index가 된다.
    OutVertexOffset = CurrentOffset / Stride;

    // 다음 오프셋
    CurrentOffset += SizeNeeded;

    return (void*)DataPtr;
}

void FDynamicVertexBuffer::Unlock()
{
    RHIDevice->GetDeviceContext()->Unmap(VertexBuffer, 0);
}

void FDynamicVertexBuffer::Release()
{
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }
}
