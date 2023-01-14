#include "ClientHandle.h"


ClientHandle::ClientHandle(const asio::ip::udp::endpoint& endpoint) : endpoint(endpoint)
{
	bind();
}

void ClientHandle::bind()
{
	bindToServiceHub(this);
}

std::shared_ptr<S_Message> ClientHandle::poll()
{
	std::shared_ptr <S_Message> temp = sendHandle.messages.front();
	const MessageHeader* tempHeader = temp->getHeader();

	if (tempHeader->Importance == High_Importance ||
		(tempHeader->Importance == Middle_Importance && tempHeader->sendCount < MiddlePriorityMaxTries) ||
		(tempHeader->Importance == Low_Importance && tempHeader->sendCount == 0))
	{
		auto it = sendHandle.messages.begin();
		std::list<std::shared_ptr <S_Message>> temp_list;
		temp_list.splice(temp_list.begin(), sendHandle.messages, it);
		sendHandle.messages.splice(sendHandle.messages.end(), temp_list, temp_list.begin());
	}
	else
	{
		auto it = sendHandle.mmap.find(temp->getHeader()->ID);
		sendHandle.messages.erase(it->second);
		sendHandle.mmap.erase(it);
	}

	return temp;
}
std::shared_ptr<C_Message> ClientHandle::pollConfirmation()
{
	std::shared_ptr < C_Message> temp = sendHandle.cMessage.front();
	sendHandle.cMessage.pop_front();
	return temp;
}

void ClientHandle::registerRedirectFunction(uint32_t redirect_func_id, RedirectHandler rHandler)
{
#ifdef _DEBUG
	if (redirectMap.contains(redirect_func_id))
	{
		std::cout << "Overriding redirect funcs possible error\n";
	}
#endif // _DEBUG
	redirectMap.insert_or_assign(redirect_func_id, rHandler);
}

void ClientHandle::removeRedirectFunction(uint32_t redirect_func_id)
{
	if (redirectMap.erase(redirect_func_id) == 0)
	{
#ifdef _DEBUG
		std::cout << "Redirect Function not Found Possible Error\n";
#endif // _DEBUG
	}
}

template<class T>
T& ClientHandle::_getRedirectFunction(uint32_t redirect_func_id)
{
	if (!redirectMap.contains(redirect_func_id))
	{
		std::cout << "Maybe derefferencing nullptr\n";
		return nullptr;
	}
	return redirectMap.at(redirect_func_id).data;
}

void ClientHandle::addNewMessage(std::shared_ptr<S_Message> msg)
{
	std::lock_guard lg(sendHandle.lock);
	std::list<std::shared_ptr<S_Message>>::iterator it;
	if (msg->getHeader()->Priority == High_Priotity)
	{
		it = sendHandle.messages.insert(sendHandle.messages.begin(), msg);
	}
	else if (msg->getHeader()->Priority == Middle_Priotity)
	{
		it = sendHandle.messages.insert(std::next(sendHandle.messages.begin(), sendHandle.messages.size() / 2), msg);
	}
	else if (msg->getHeader()->Priority == Low_Priotity)
	{
		it = sendHandle.messages.insert(sendHandle.messages.end(), msg);
	}
	sendHandle.mmap.insert(std::make_pair(msg->getHeader()->ID, it));
}

size_t ClientHandle::getSendMessageCount() const noexcept
{
	return sendHandle.messages.size();
}

size_t ClientHandle::getRecieveMessageCount() const noexcept
{
	return recieveHandle.messages.size();
}

size_t ClientHandle::getConfimationMsgCount() const noexcept
{
	return sendHandle.cMessage.size();
}

asio::ip::udp::endpoint ClientHandle::getEndpoint() const noexcept
{
	return endpoint;
}

void ClientHandle::recieve(std::shared_ptr<R_Message> message)
{
	lastRecv = std::chrono::steady_clock::now();

	std::lock_guard lg(recieveHandle.lock);
	if (recieveHandle.mmap.find(message->getHeader().ID) == recieveHandle.mmap.end())
	{
		std::shared_ptr<Recieve::t_mmap> msgInfo(new Recieve::t_mmap{});
		msgInfo->timepoint = std::chrono::steady_clock::now();
		auto redirectFunc = redirectMap.find(message->getHeader().fast_redirect);
		if (redirectFunc != redirectMap.end())	// if in fast direct do redirect
		{
			redirectFunc->second.function(this, message); //blocks, maybe optimize later
			msgInfo->ValidIterator = false;
		}
		else // do standard pathway
		{
			msgInfo->fastAccess = recieveHandle.messages.insert(recieveHandle.messages.end(), message);
			msgInfo->ValidIterator = true;
		}
		recieveHandle.mmap.insert(std::make_pair(message->getHeader().ID, msgInfo));
	}

	std::lock_guard lcg(sendHandle.c_lock);
	sendHandle.cMessage.push_back(std::make_shared<C_Message>(message->getHeader().ID));
}
void ClientHandle::recieveConfirmation(std::shared_ptr<C_Message> message)
{
	std::lock_guard lg(sendHandle.lock);
	if (sendHandle.messages.size() == 0)
	{
		return;
	}

	auto it = sendHandle.mmap.find(message->getMsg().confirmID);
	if (it != sendHandle.mmap.end())
	{
		sendHandle.messages.erase(it->second);
		sendHandle.mmap.erase(it);
	}
}
std::shared_ptr<R_Message> ClientHandle::getRecievedMsg()
{
	std::lock_guard lg(recieveHandle.lock);
	auto it = recieveHandle.messages.begin();
	std::shared_ptr<R_Message> temp = *it;
	recieveHandle.mmap.at(temp->getHeader().ID)->ValidIterator = false;
	recieveHandle.messages.erase(it);
	return temp;
}

std::mutex& ClientHandle::getRecievedMutex()
{
	return  recieveHandle.lock;
}