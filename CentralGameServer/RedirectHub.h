#pragma once

#include "pch.h"
#include "Message.h"
#include "TaskExecutor.h"

class ClientHandle;
typedef std::function<uint64_t(std::shared_ptr<ExtentPtr<ClientHandle>>, std::shared_ptr<Recv_Message>)> RedirectFunction;

struct RedirectMemoryBase
{
	std::chrono::steady_clock::time_point creation_time = std::chrono::steady_clock::now();
	virtual ~RedirectMemoryBase() {};
};

struct PingMemory : public RedirectMemoryBase
{
	std::chrono::steady_clock::time_point start_time;
	std::chrono::steady_clock::duration elapsed;
	uint8_t status;
	uint16_t bytes_transfered;
};

enum PingStatus
{
	P_SUCCES = 0,
	P_FAIL = 1,
	P_STILL_RUNNING = 2
};

enum LoginRegisterStatus
{
	LR_SUCCESS = 0,
	LR_FAIL = 1,
	LR_PENDING = 2,
	LR_INAVLID_CREDENTIALS = 3,
	LR_USERNAME_ALREADY_USED = 4,
	LR_INVALID_INPUT = 5
};

template<typename T>
concept isRedirectMemory = std::is_base_of<RedirectMemoryBase, T>::value;

class RedirectHub
{
	static std::unordered_map < uint16_t, std::unordered_map< uint16_t, RedirectFunction>> funcHandle;
	static std::unordered_map<uint64_t, std::shared_ptr<RedirectMemoryBase>> memory;
	static uint64_t memoryIdCounter;
public:
	template<isRedirectMemory T>
	static std::shared_ptr<T> getMemory(uint64_t MemoryID)
	{
		try
		{
			return std::dynamic_pointer_cast<T>(memory.at(MemoryID));
		}
		catch (const std::exception&)
		{
			std::cout << "Aome error\n";
			return nullptr;
		}
	}

	static RedirectFunction getFunc(uint16_t ClientID, uint16_t FuncID)
	{
		try
		{
			return funcHandle.at(ClientID).at(FuncID);
		}
		catch (const std::exception&)
		{
			std::cout << "Some error\n";
			return nullptr;
		}
	}
	static void registerRedirect(uint16_t ClientID, uint16_t FuncID, RedirectFunction func)
	{
		try
		{
			if (funcHandle.contains(ClientID))
			{
				funcHandle.at(ClientID).insert_or_assign(FuncID, func);
			}
			else
			{
				funcHandle.emplace(ClientID, std::unordered_map<uint16_t, RedirectFunction>({ {FuncID,func} }));
			}
		}
		catch (const std::exception&)
		{
			std::cout << "WSome error\n";
			return;
		}
	}
	template<isRedirectMemory T>
	static std::pair<uint64_t, std::shared_ptr<T>> allocRedirectMemory(std::chrono::steady_clock::duration dur = 0s)
	{
		//maybe add timeout to avoid orphaned memory
		uint64_t memID = ++memoryIdCounter;
		std::shared_ptr<T> ptr(new T);
		memory.emplace(memID, std::dynamic_pointer_cast<RedirectMemoryBase>(ptr));
		if (dur > 0s)
		{
			TaskExecutor::addAsyncTask(std::bind(std::function([memID]() {RedirectHub::freeRedirectMemory<RedirectMemoryBase>(memID); })), dur);
		}
		return { memID, ptr };//;
	}
	template<isRedirectMemory T>
	static void freeRedirectMemory(uint64_t memory_id)
	{
		auto it = memory.find(memory_id);
		if (it == memory.end())
		{
			return;
		}
		memory.erase(it);
		return;
	}
	static void removerRedirectFuncs(const uint16_t& uid)
	{
		auto it = funcHandle.find(uid);
		if (it != funcHandle.end())
		{
			funcHandle.erase(it);
		}
	}
};