#pragma once
#include "Mesh/Public/Actor.h"

class USceneComponent;

class AGrid : public AActor
{
	using Super = AActor;
public:
	AGrid();
	virtual ~AGrid() override;

	virtual void BeginPlay() override;
	virtual void EndPlay() override;
	virtual void Tick() override;

	void SetGridProperty(const float InCell, const float InRelativeLocationY, const int InHalfLines);

private:
	void Build();

	/**
	 * @brief 멤버 변수 설명
	 * @var Cell 키보드 단축키 테이블 핸들
	 * @var RelativeLocationY Grid가 놓일 Y축 값
	 * @var HalfLines Grid를 중심으로 생성할 선의 개수
	 */
	USceneComponent* Component = nullptr;
	float Cell = {};
	float RelativeLocationY = {};
	int HalfLines = {};
};

