#include "GameInstance.h"

GameInstanceBase::GameInstanceBase(const uint32_t& id)
{
	this_game_id = id;
}

void GameInstanceBase::start()
{
	mainThread.reset(new std::thread(&GameInstanceBase::_start,this));
	running = true;
	
}

void GameInstanceBase::stop()
{
	_stop();
}

void GameInstanceBase::waitStop()
{
	while (running)
	{
		std::this_thread::sleep_for(10ms);
	}
}

void GameInstanceBase::_start()
{
	std::cout << "Base GameInstance launched\n";
	while (!stopRequest)
	{
		std::this_thread::sleep_for(1s);
		std::shared_lock sl(clHandleMutex);
		std::cout << "clHandleSize: " << clHandle.size() << "\n";
	}
	running = false;
	//this->clHandle.at(0).lock()->gMsgHandle.getHandle(17)->pop(); recv
	//this->clHandle.at(0).lock()->addSendMessage(smsg); send
}

void GameInstanceBase::_stop()
{
	stopRequest = true;
}

void GameInstanceBase::addCl(std::weak_ptr<ClientHandle> WeakClientHandle)
{
	if (auto clientHandle = WeakClientHandle.lock())
	{
		std::unique_lock ul(clHandleMutex);

		clientHandle->gMsgHandle->add_or_override_Handle(gInfo.ID);
		clHandle.insert_or_assign(clientHandle->this_ID, clientHandle);
	}
	else
	{
		std::cout << "Trying to add invalid ClientHandle to GameInstance\n";
	}
}

void GameInstanceBase::removeCl(const uint32_t& key)
{
	std::unique_lock ul(clHandleMutex);
	clHandle.erase(key);
}

std::shared_mutex GameInstanceSection::gLock;
std::unordered_map<uint32_t, std::shared_ptr<GameInstanceBase>> GameInstanceSection::gameInstances;


uint32_t GameInstanceInfo::counter = 17;