#pragma once
#include <algorithm>
#include <cmath>
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 960

class ScreenUtil {
public:
    // Convert from NDC (-1 to 1) to screen coordinates (0 to SCREEN_WIDTH/HEIGHT)
    static std::pair<float, float> NDCToScreenPos(std::pair<float, float> ndcPos)
    {
        // NDC ranges from -1 to 1, screen coordinates from 0 to WIDTH/HEIGHT
        float screenX = (ndcPos.first + 1.0f) * 0.5f * SCREEN_WIDTH;
        float screenY = (1.0f - ndcPos.second) * 0.5f * SCREEN_HEIGHT;

        return std::make_pair(screenX, screenY);
    }

    // Convert from screen coordinates to NDC (-1 to 1)
    static std::pair<float, float> ScreenPosToNDC(std::pair<float, float> screenPos)
    {
        // Screen coordinates from 0 to WIDTH/HEIGHT, NDC from -1 to 1
        float ndcX = (2.0f * screenPos.first) / SCREEN_WIDTH - 1.0f;
        float ndcY = 1.0f - (2.0f * screenPos.second) / SCREEN_HEIGHT;

        return std::make_pair(ndcX, ndcY);
    }

    static float GetAspectRatio()
    {
        return (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    }
};