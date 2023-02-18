#include "GameInstance.h"

void GameInstanceBase::start()
{
	mainThread.reset(new std::thread(&GameInstanceBase::_start,this));
}

void GameInstanceBase::_start()
{
	while (true)
	{
		std::this_thread::sleep_for(1s);
		std::shared_lock sl(clHandleMutex);
		std::cout << "clHandleSize: " << clHandle.size() << "\n";
		for (auto& cl : clHandle)
		{
			if (auto sptr = cl.second.lock())
			{
				std::cout << sptr->this_ID << "\n";
			}
			else
			{
				std::cout << "Invalid clientHandlePtr\n";
			}

		}
	}
}

void GameInstanceBase::addCl(std::weak_ptr<ClientHandle> WeakClientHandle)
{
	if (auto clientHandle = WeakClientHandle.lock())
	{
		std::unique_lock ul(clHandleMutex);

		clientHandle->gMsgHandle.add_or_override_Handle(gInfo.ID);
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