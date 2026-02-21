#pragma once
#include "Global/Vector.h"
#include <string>

/**
 * Cubic Bezier Curve
 * Defined by 4 parameters (2 control points)
 * P0 = (0, 0) - Start point (fixed)
 * P1 = (X1, Y1) - First control point
 * P2 = (X2, Y2) - Second control point
 * P3 = (1, 1) - End point (fixed)
 */
struct FCurve
{
	/** First control point X (0.0 ~ 1.0) */
	float ControlPoint1X = 0.25f;

	/** First control point Y (free range, but typically 0.0 ~ 1.0) */
	float ControlPoint1Y = 0.25f;

	/** Second control point X (0.0 ~ 1.0) */
	float ControlPoint2X = 0.75f;

	/** Second control point Y (free range, but typically 0.0 ~ 1.0) */
	float ControlPoint2Y = 0.75f;

	/** Curve name/identifier */
	std::string CurveName = "Untitled";

	FCurve() = default;

	FCurve(float InCP1X, float InCP1Y, float InCP2X, float InCP2Y)
		: ControlPoint1X(InCP1X)
		, ControlPoint1Y(InCP1Y)
		, ControlPoint2X(InCP2X)
		, ControlPoint2Y(InCP2Y)
	{
	}

	/**
	 * Evaluate both X and Y at parameter t using Cubic Bezier formula
	 * @param t Time value (0.0 ~ 1.0)
	 * @param OutX Output X coordinate
	 * @param OutY Output Y coordinate
	 */
	void EvaluateXY(float t, float& OutX, float& OutY) const
	{
		// Clamp t to [0, 1]
		t = std::clamp(t, 0.0f, 1.0f);

		// Cubic Bezier formula: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
		// P0 = (0, 0), P3 = (1, 1)
		const float OneMinusT = 1.0f - t;
		const float OneMinusT2 = OneMinusT * OneMinusT;
		const float T2 = t * t;

		// X(t) = 3(1-t)²t * CP1.X + 3(1-t)t² * CP2.X + t³
		OutX = 3.0f * OneMinusT2 * t * ControlPoint1X
		     + 3.0f * OneMinusT * T2 * ControlPoint2X
		     + T2 * t;

		// Y(t) = 3(1-t)²t * CP1.Y + 3(1-t)t² * CP2.Y + t³
		OutY = 3.0f * OneMinusT2 * t * ControlPoint1Y
		     + 3.0f * OneMinusT * T2 * ControlPoint2Y
		     + T2 * t;
	}

	/**
	 * Evaluate the curve at time t using Cubic Bezier formula
	 * @param t Time value (0.0 ~ 1.0)
	 * @return Evaluated Y value
	 */
	float Evaluate(float t) const
	{
		float X, Y;
		EvaluateXY(t, X, Y);
		return Y;
	}

	/**
	 * Evaluate the curve at X position (solving for t first)
	 * Note: For performance, this uses linear approximation
	 * @param X Input X value (0.0 ~ 1.0)
	 * @return Evaluated Y value
	 */
	float EvaluateAtX(float X) const
	{
		X = std::clamp(X, 0.0f, 1.0f);

		// Simple binary search to find t for given X
		float t = X; // Initial guess
		const int Iterations = 8; // Newton-Raphson iterations

		for (int i = 0; i < Iterations; ++i)
		{
			const float OneMinusT = 1.0f - t;
			const float OneMinusT2 = OneMinusT * OneMinusT;
			const float T2 = t * t;

			// X = 3(1-t)²t * CP1.X + 3(1-t)t² * CP2.X + t³
			const float CurrentX = 3.0f * OneMinusT2 * t * ControlPoint1X
			                     + 3.0f * OneMinusT * T2 * ControlPoint2X
			                     + T2 * t;

			const float Error = CurrentX - X;
			if (std::abs(Error) < 0.001f)
				break;

			// Derivative: dX/dt = 3(1-t)²CP1.X - 6(1-t)tCP1.X + 3t²CP2.X - 6(1-t)tCP2.X + 3t²
			const float Derivative = 3.0f * OneMinusT2 * ControlPoint1X
			                       - 6.0f * OneMinusT * t * ControlPoint1X
			                       + 3.0f * T2 * ControlPoint2X
			                       - 6.0f * OneMinusT * t * ControlPoint2X
			                       + 3.0f * T2;

			if (std::abs(Derivative) < 0.0001f)
				break;

			t -= Error / Derivative;
			t = std::clamp(t, 0.0f, 1.0f);
		}

		return Evaluate(t);
	}

	/**
	 * Get control point 1 as FVector2 (ImVec2 compatible)
	 */
	FVector2 GetControlPoint1() const
	{
		return FVector2(ControlPoint1X, ControlPoint1Y);
	}

	/**
	 * Get control point 2 as FVector2 (ImVec2 compatible)
	 */
	FVector2 GetControlPoint2() const
	{
		return FVector2(ControlPoint2X, ControlPoint2Y);
	}

	/**
	 * Set control point 1
	 */
	void SetControlPoint1(float X, float Y)
	{
		ControlPoint1X = std::clamp(X, 0.0f, 1.0f);
		ControlPoint1Y = Y; // Y can go outside [0, 1] for overshoot
	}

	/**
	 * Set control point 2
	 */
	void SetControlPoint2(float X, float Y)
	{
		ControlPoint2X = std::clamp(X, 0.0f, 1.0f);
		ControlPoint2Y = Y; // Y can go outside [0, 1] for overshoot
	}

	// === Preset Curves ===

	static FCurve Linear()
	{
		// For perfect linear: control points should be at 1/3 and 2/3
		return FCurve(0.333f, 0.333f, 0.666f, 0.666f);
	}

	static FCurve EaseIn()
	{
		return FCurve(0.42f, 0.0f, 1.0f, 1.0f);
	}

	static FCurve EaseOut()
	{
		return FCurve(0.0f, 0.0f, 0.58f, 1.0f);
	}

	static FCurve EaseInOut()
	{
		return FCurve(0.42f, 0.0f, 0.58f, 1.0f);
	}

	static FCurve EaseInBack()
	{
		return FCurve(0.6f, -0.28f, 0.735f, 0.045f);
	}

	static FCurve EaseOutBack()
	{
		return FCurve(0.175f, 0.885f, 0.32f, 1.275f);
	}
};
