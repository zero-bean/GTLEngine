#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Global/CurveTypes.h"

class UCurveLibrary;

/**
 * Curve Editor Window
 * Allows editing cubic bezier curves stored in Level's CurveLibrary
 */
UCLASS()
class UCurveEditorWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_CLASS(UCurveEditorWindow, UUIWindow)

public:
	UCurveEditorWindow();
	virtual ~UCurveEditorWindow() override;

	void Initialize() override;

protected:
	void OnPreRenderWindow(float MenuBarOffset) override;
	void OnPostRenderWindow() override;

private:
	/** Current curve being edited (working copy) */
	FCurve CurrentCurve;

	/** Currently selected curve name */
	FString SelectedCurveName;

	/** Canvas size for curve editor */
	ImVec2 CanvasSize = ImVec2(400, 400);

	/** Grid divisions */
	int GridDivisions = 10;

	/** Input buffer for new curve name */
	char NewCurveNameBuffer[256] = "";

	/** Input buffer for rename */
	char RenameCurveBuffer[256] = "";

	// === UI Helper Methods ===

	/**
	 * Get CurveLibrary from current level
	 */
	UCurveLibrary* GetCurveLibrary() const;

	/**
	 * Called when level changes (new level loaded or created)
	 */
	void OnLevelChanged();

	/**
	 * Load selected curve from library into working copy
	 */
	void LoadSelectedCurve();

	/**
	 * Save working copy back to library
	 */
	void SaveCurrentCurve();

	/**
	 * Render curve selection dropdown
	 */
	void RenderCurveSelection();

	/**
	 * Render curve CRUD operations (New, Delete, Rename)
	 */
	void RenderCurveOperations();

	/**
	 * Render curve canvas with grid and curve
	 */
	void RenderCurveCanvas();

	/**
	 * Render manual input for control points
	 */
	void RenderManualInput();

	/**
	 * Convert normalized curve position (0~1) to canvas pixel position
	 */
	ImVec2 CurveToCanvas(ImVec2 CurvePos, ImVec2 CanvasPos, ImVec2 CanvasSize) const;

	/**
	 * Convert canvas pixel position to normalized curve position (0~1)
	 */
	ImVec2 CanvasToCurve(ImVec2 CanvasPixelPos, ImVec2 CanvasPos, ImVec2 CanvasSize) const;
};
