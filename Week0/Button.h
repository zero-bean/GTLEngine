#pragma once
#include "pch.h"
#include "Object.h"
#include <functional>

class Renderer;
class Button : public Object
{
public:
    Button(const wchar_t* newImagePath, const std::pair<float,float> newSize);
    ~Button();
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
    void SetCallback(const std::function<void()>& newCallback) {
        callback = newCallback;
    }

    bool IsOverlapWithMouse(int mouseX, int mouseY);
    void OnClick();


private:
    const wchar_t* imagePath;
    std::pair<float,float> size;
    std::function<void()> callback;
};