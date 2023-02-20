#pragma once
#ifndef GAMEINSTANCE_H
#define GAMEINSTANCE_H
#include "pch.h"
#include "ClientHandle.h"

struct GameInstanceInfo
{
    std::string GameName = "NAME";
    uint32_t ID = counter++;
    uint16_t PlayerNum = 0;
	static uint32_t counter;
};


class GameInstanceBase
{
protected:
    std::unordered_map<uint32_t,std::weak_ptr<ClientHandle>> clHandle;
    std::shared_mutex clHandleMutex;
    GameInstanceInfo gInfo;
	std::unique_ptr<std::thread> mainThread;

	bool stopRequest = false;
	bool running = false;

	virtual void _start();
	virtual void _stop();

	virtual ~GameInstanceBase() {}

public:
	void start();
	void stop();
	void waitStop();
    void addCl(std::weak_ptr<ClientHandle>);
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
#endif