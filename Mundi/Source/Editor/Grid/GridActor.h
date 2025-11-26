#pragma once
#include "Actor.h"
#include "LineComponent.h"

class AGridActor : public AActor
{
public:
    DECLARE_CLASS(AGridActor, AActor);
    AGridActor();
    void Initialize();

protected:
    ~AGridActor() override;

public:
    // Grid and Axis creation methods
    void CreateGridLines(int32 GridSize = 50, float CellSize = 1.0f, const FVector& Center = FVector());
    void CreateAxisLines(float Length = 50.0f, const FVector& Origin = FVector());
    void ClearLines();
    void SetAxisVisible(bool bVisible);
    bool IsAxisVisible() const { return bShowAxis; }
    void SetGridVisible(bool bVisible);
    bool IsGridVisible() const { return bShowGridLines; }
    
    // Grid settings
    float GetLineSize() { return LineSize; }
    void SetLineSize(float NewLineSize) { LineSize = NewLineSize; SetActorScale({ NewLineSize, NewLineSize, NewLineSize }); }
    
    // Component access
    ULineComponent* GetLineComponent() const { return LineComponent; }

    // ���������� ���� ���� ��������������������������������������������������������
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(AGridActor)
private:
    void RegenerateGrid();

    ULineComponent* LineComponent;  // 하위 호환용 (deprecated)
    ULineComponent* GridLineComponent;  // Grid 전용
    ULineComponent* AxisLineComponent;  // Axis 전용

    // Grid settings
    int32 GridSize = 100;
    float CellSize = 1.0f;
    float AxisLength = 100.0f;

    float LineSize = 1.0f;   // �⺻������ ����
    bool bShowAxis = true;
    bool bShowGridLines = true;
};

