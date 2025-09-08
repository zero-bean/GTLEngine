#pragma once

class UEngineStatics
{
public:
	static UINT GenUUID()
	{
		return NextUUID++;
	}
private:
	static UINT NextUUID;
};
