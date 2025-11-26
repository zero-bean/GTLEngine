#pragma once

class D3D11RHI;

class FDynamicVertexBuffer
{
public:
    FDynamicVertexBuffer() = default;
    ~FDynamicVertexBuffer();

    // 2MB 버퍼 할당
    void Initialize(D3D11RHI* InRHI, uint32 Size = 2 * 1024 * 1024);

    // void 포인터 반환
    void* Lock(uint32 VertexCount, uint32 Stride, uint32& OutVertexOffset);
    void Unlock();

    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }

private:
    void Release();

private:
    D3D11RHI* RHIDevice = nullptr;
    ID3D11Buffer* VertexBuffer = nullptr;

    uint32 TotalSize = 0;
    uint32 CurrentOffset = 0;    
};
