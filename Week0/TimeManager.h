#pragma once

class TimeManager
{
public:
	static TimeManager* GET_SINGLE() 
	{
		static TimeManager instance;
		return &instance;
	}

private:
	TimeManager() {};
	~TimeManager() {};

	TimeManager(const TimeManager&) = delete;
	TimeManager& operator=(const TimeManager&) = delete;

public:
	void Init();
	void Update();

public:
	unsigned __int32 GetFPS() const { return _fps; }
	float GetDeltaTime() const { return _deltaTime; }

private:
	unsigned __int64 _frequency = 0;
	unsigned __int64 _prevCount = 0;
	float _deltaTime = 0.f;

private:
	unsigned __int32 _frameCount = 0;
	unsigned __int32 _fps = 0;
	float _frameTime = 0.f;
};

