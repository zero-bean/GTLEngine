#pragma once
class CollisionManager
{
private:

	CollisionManager();
	~CollisionManager();

private:
	void Initialize();


public:
	static CollisionManager* GetInstance()
	{
		if (sCollisionInstance == nullptr)
		{
			sCollisionInstance = new CollisionManager;
			sCollisionInstance->Initialize();
		}

		return sCollisionInstance;
	}


	static CollisionManager* DestroyInstance()
	{
		if (sCollisionInstance != nullptr)
		{
			delete sCollisionInstance;
		}
	}

private:
	static CollisionManager* sCollisionInstance;

};

