#pragma once
#include "Editor/Public/Camera.h"

class FViewportClient
{
public:
	FViewportClient() = default;
	~FViewportClient() = default;

	/* *
	* @brief 출력될 화면의 너비, 높이, 깊이 등을 적용합니다.
	*/
	void Apply(ID3D11DeviceContext* InContext) const;

	/* *
	* @brief 현재는 사용하지 않지만, 추후 사용될 여지가 있음
	*/
	void ClearDepth(ID3D11DeviceContext* InContext, ID3D11DepthStencilView* InStencilView) const;

	/* *
	* @brief 카메라 상태를 업데이트합니다.
	*/
	void SnapCameraToView(const FVector& InFocusPoint);

	bool IsOrthographic() const { return CameraType != EViewportCameraType::Perspective; }

	// Getter
	D3D11_VIEWPORT GetViewportInfo() const { return ViewportInfo; }
	EViewportCameraType GetCameraType() const { return CameraType; }

	// Setter
	void SetViewportInfo(const D3D11_VIEWPORT& InViewport) { ViewportInfo = InViewport; }
	void SetCameraType(EViewportCameraType InViewportCameraType);

	D3D11_VIEWPORT ViewportInfo = {};
	UCamera Camera;
	bool bIsActive = false;
	EViewportCameraType CameraType = EViewportCameraType::Perspective;
};

