#include "pch.h"
#include "Render/Grid/Public/Grid.h"
#include "Mesh/Public/SceneComponent.h"

AGrid::AGrid() : Cell(1), RelativeLocationY(0.f), HalfLines(25)
{
	Component = CreateDefaultSubobject<USceneComponent>("GridRoot");
	SetRootComponent(Component);
	Build();
}

AGrid::~AGrid()
{

}

void AGrid::BeginPlay()
{
	Super::BeginPlay();
}

void AGrid::EndPlay()
{
	Super::EndPlay();
}

void AGrid::Tick()
{
	Super::Tick();
}

void AGrid::SetGridProperty(const float InCell, const float InRelativeLocationY, const int InHalfLines)
{
	Cell = InCell;
	RelativeLocationY = InRelativeLocationY;
	HalfLines = InHalfLines;
	Build();
}

void AGrid::Build()
{
	const float FullLength = static_cast<float>(HalfLines * 2) * Cell;
	const float HalfLength = FullLength * 0.5f;

	/**
	 * @brief Z 방향 라인 생성
	 */
	for (int Cnt = -HalfLines; Cnt <= HalfLines; ++Cnt)
	{
		FString Name = FString("Grid_Z_") + std::to_string(Cnt);
		ULineComponent* Line = CreateDefaultSubobject<ULineComponent>(Name);
		Line->SetRelativeLocation(FVector{ Cnt * Cell, RelativeLocationY, -HalfLength });
		Line->SetRelativeRotation(FVector{ 0.f, 0.f, 0.f });
		Line->SetRelativeScale3D(FVector{ 1.f, 1.f, FullLength });
		Line->SetColor({ 1.f, 1.f, 1.f, 1.f });
	}

	/**
	 * @brief X 방향 라인 생성
	 */
	for (int Cnt = -HalfLines; Cnt <= HalfLines; ++Cnt)
	{
		FString Name = FString("Grid_X_") + std::to_string(Cnt);
		ULineComponent* Line = CreateDefaultSubobject<ULineComponent>(Name);
		Line->SetRelativeRotation(FVector{ 0.f, 90.f, 0.f });
		Line->SetRelativeLocation(FVector{ -HalfLength, RelativeLocationY, Cnt * Cell });
		Line->SetRelativeScale3D(FVector{ 1.f, 1.f, FullLength });
		Line->SetColor({ 1.f, 1.f, 1.f, 1.f });
	}
}

