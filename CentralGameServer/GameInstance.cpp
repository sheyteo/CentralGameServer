#include "GameInstance.h"

void GameInstanceBase::stop()
{

}

bool GameInstanceBase::is_stopped()
{
	return stopped.load();
}
