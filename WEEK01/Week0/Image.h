#pragma once
#include "pch.h"
#include "Object.h"
class Renderer;
class Image : public Object
{
public:
    Image(const wchar_t* newImagePath, const std::pair<float,float> newSize);
    ~Image();
    void Initialize(Renderer& renderer)override;
    void Update(Renderer& renderer) override;
    void Render(Renderer& renderer) override;
    void Release() override;
    void Collidable() override;
public:
    // getter
    inline FVector3 GetWorldPosition()const { return WorldPosition; }
    inline std::pair<float,float> GetSize() { return size; }
                    
    //setter        
    inline void SetWorldPosition(const FVector3& InNewPosition) { WorldPosition = InNewPosition; }
    inline void SetSize(const std::pair<int, int>& newSize) { size = newSize; }


private:
    const wchar_t* imagePath;
    std::pair<float,float> size;
};