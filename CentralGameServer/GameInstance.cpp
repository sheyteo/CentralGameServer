#include "GameInstance.h"

void GameInstanceBase::start()
{
	while (true)
	{

	}
}

void GameInstanceBase::addCl(std::shared_ptr<ClientHandle> clientHandle)
{
	std::unique_lock ul(clHandleMutex);
	clientHandle->gMsgHandle.add_or_override_Handle(gInfo.ID);
	clHandle.insert_or_assign(clientHandle->this_ID,clientHandle);
}

void GameInstanceBase::removeCl(const uint32_t& key)
{
	std::unique_lock ul(clHandleMutex);
	clHandle.erase(key);
}

std::shared_mutex GameInstanceSection::gLock;
std::unordered_map<uint32_t, std::shared_ptr<GameInstanceBase>> GameInstanceSection::gameInstances;