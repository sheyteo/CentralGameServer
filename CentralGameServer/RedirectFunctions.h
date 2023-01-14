#pragma once
#include "pch.h"
#include "Messages.h"
#include "ClientHandle.h"

#define RedirectFunctionParamater std::shared_ptr<ClientHandle> clientHandle, std::shared_ptr<R_Message> message

namespace RedirectFunctions
{
	void pingResponse(RedirectFunctionParamater)
	{
		std::shared_ptr<S_Message> msg(new S_Message());
		msg->setData(message->getData(), 0, High_Importance, High_Priotity, Ping);
		clientHandle->addNewMessage(msg);
	}
}