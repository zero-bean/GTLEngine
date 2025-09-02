#include "Scene.h"

extern class UBallList;
extern class URenderer;

class TestScene : public Scene {
private:
	bool bIsGravity;
	int TargetBallCount;
	UBallList BallList;
public:
	TestScene();

	void Start() override;
	void Update(float deltaTime) override;
	void OnGUI(HWND hWND) override;
	void OnRender(URenderer * renderer) override;
	void Shutdown() override;
};