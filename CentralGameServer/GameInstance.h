#pragma once
#include "pch.h"
#include "NewClientHandle.h"

struct GameInstanceInfo
{
    std::string GameName;
    uint32_t ID;
    uint16_t PlayerNum;
};

class GameInstanceBase
{
    std::unordered_map<uint32_t,std::shared_ptr<ClientHandle>> clHandle;
    std::shared_mutex clHandleMutex;
    GameInstanceInfo gInfo;
public:
    void start();
    void addCl(std::shared_ptr<ClientHandle>);
    void removeCl(const uint32_t&);
};

class GameInstanceSection
{
private:
	static std::shared_mutex gLock;
	static std::unordered_map<uint32_t, std::shared_ptr<GameInstanceBase>> gameInstances;
public:
	static void addGameInstance(const uint32_t& id, std::shared_ptr<GameInstanceBase> qg)
	{
		std::unique_lock uLock(gLock);
		gameInstances.insert_or_assign(id, qg);
	}
	static void removeGameInstance(const uint32_t& key)
	{
		std::unique_lock uLock(gLock);
		gameInstances.erase(key);
	}
	static std::shared_ptr<GameInstanceBase> getGameInstance(const uint32_t& key)
	{
		std::shared_lock sLock(gLock);
		auto it = gameInstances.find(key);
		if (it != gameInstances.end())
		{
			return it->second;
		}
		else
		{
			return nullptr;
		}
	}
};