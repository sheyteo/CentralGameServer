#pragma once
#include "pch.h"
#include "Messages.h"

class ClientHandle
{
	friend class Server;
	class Recieve
	{
	public:
		mutable std::mutex lock;
		std::list<std::shared_ptr<R_Message>> messages;
		struct t_mmap
		{
			std::chrono::steady_clock::time_point timepoint;
			bool ValidIterator;
			InfoType infoType;
			std::list<std::shared_ptr<R_Message>>::iterator fastAccess;
		};

		std::map<unsigned int, std::shared_ptr<t_mmap>> mmap;
	};

	class Send
	{
	public:
		mutable std::mutex lock;
		std::list<std::shared_ptr<S_Message>> messages; // optimisation may be good // Priority is missing partly
		std::map<unsigned int, std::list<std::shared_ptr<S_Message>>::iterator> mmap;

		mutable std::mutex c_lock;
		std::list<std::shared_ptr<C_Message>> cMessage;
	};

	std::unordered_map<uint32_t, std::function<void(std::shared_ptr<ClientHandle>, std::shared_ptr<R_Message>)>> redirectMap;

	asio::ip::udp::endpoint endpoint;
	std::chrono::steady_clock::time_point lastRecv;
	Recieve recieveHandle;
	Send sendHandle;

public:
	ClientHandle(const asio::ip::udp::endpoint& endpoint) : endpoint(endpoint) 
	{

	}

	std::shared_ptr<S_Message> poll()
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
	std::shared_ptr<C_Message> pollConfirmation()
	{
		std::shared_ptr < C_Message> temp = sendHandle.cMessage.front();
		sendHandle.cMessage.pop_front();
		return temp;
	}

	void registerRedirectFunction(uint32_t redirect_func_id, std::function<void(std::shared_ptr<ClientHandle>, std::shared_ptr<R_Message>)> func)
	{
#ifdef _DEBUG
		if (redirectMap.contains(redirect_func_id))
		{
			std::cout << "Overriding redirect funcs possible error\n";
		}
#endif // _DEBUG
		redirectMap.insert_or_assign(redirect_func_id, func);
	}

	void removeRedirectFunction(uint32_t redirect_func_id)
	{
		if (redirectMap.erase(redirect_func_id) == 0)
		{
#ifdef _DEBUG
			std::cout << "Redirect Function not Found Possible Error\n";
#endif // _DEBUG
		}
	}

	void addNewMessage(std::shared_ptr<S_Message> msg)
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

	size_t getSendMessageCount() const noexcept
	{
		return sendHandle.messages.size();
	}

	size_t getRecieveMessageCount() const noexcept
	{
		return recieveHandle.messages.size();
	}

	size_t getConfimationMsgCount() const noexcept
	{
		return sendHandle.cMessage.size();
	}

	asio::ip::udp::endpoint getEndpoint() const noexcept
	{
		return endpoint;
	}

	void recieve(std::shared_ptr<R_Message> message)
	{
		lastRecv = std::chrono::steady_clock::now();

		std::lock_guard lg(recieveHandle.lock);
		if (recieveHandle.mmap.find(message->getHeader().ID) == recieveHandle.mmap.end())
		{
			std::shared_ptr<Recieve::t_mmap> msgInfo(new Recieve::t_mmap{});
			msgInfo->timepoint = std::chrono::steady_clock::now();
			msgInfo->infoType = (InfoType)message->getHeader().msgInfoType;
			auto redirectFunc = redirectMap.find(message->getHeader().fast_redirect);
			if (redirectFunc != redirectMap.end())	// if in fast direct do redirect
			{
				redirectFunc->second(std::shared_ptr<ClientHandle>(this), message); //blocks, maybe optimize later
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
	void recieveConfirmation(std::shared_ptr<C_Message> message)
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
	std::shared_ptr<R_Message> getRecievedMsg()
	{
		std::lock_guard lg(recieveHandle.lock);
		auto it = recieveHandle.messages.begin();
		std::shared_ptr<R_Message> temp = *it;
		recieveHandle.mmap.at(temp->getHeader().ID)->ValidIterator = false;
		recieveHandle.messages.erase(it);
		return temp;
	}

	std::mutex& getRecievedMutex()
	{
		return  recieveHandle.lock;
	}
};