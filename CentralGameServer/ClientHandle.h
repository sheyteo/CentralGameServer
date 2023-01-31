#pragma once
#include "pch.h"
#include "Messages.h"

struct shared_redirect_memory
{
	std::chrono::steady_clock::time_point created{};
	virtual ~shared_redirect_memory() {};
};

struct shared_ping_memory : public shared_redirect_memory
{
	uint32_t status {};
	std::chrono::steady_clock::duration elapsed{};
};

class ClientHandle
{
	friend class Server;
private:
	class Recieve
	{
	public:
		mutable std::mutex lock;
		std::list<std::shared_ptr<R_Message>> messages;
		struct t_mmap
		{
			std::chrono::steady_clock::time_point timepoint;
			bool ValidIterator;
			std::list<std::shared_ptr<R_Message>>::iterator fastAccess;
		};

		std::unordered_map<unsigned int, std::shared_ptr<t_mmap>> mmap;
	};

	class Send
	{
	public:
		mutable std::mutex lock;
		std::list<std::shared_ptr<S_Message>> messages; // optimisation may be good // Priority is missing partly
		std::unordered_map<unsigned int, std::list<std::shared_ptr<S_Message>>::iterator> mmap;

		mutable std::mutex c_lock;
		std::list<std::shared_ptr<C_Message>> cMessage;
	};
public:
	struct RedirectHandler
	{
		std::function<void(ClientHandle*, std::shared_ptr<R_Message>)> function;
		std::shared_ptr< std::shared_ptr<shared_redirect_memory>> data;
	};
private:
	bool stopRequest = false;
	bool valid = true;

	std::unordered_map<uint32_t, RedirectHandler> redirectMap;

	asio::ip::udp::endpoint endpoint;
	std::chrono::steady_clock::time_point lastRecv;
	Recieve recieveHandle;
	Send sendHandle;

public:
	ClientHandle(const asio::ip::udp::endpoint& endpoint);

	~ClientHandle();

	void cleanUp();

	void bind();

	std::shared_ptr<S_Message> poll();

	std::shared_ptr<C_Message> pollConfirmation();

	void registerRedirectFunction(uint32_t redirect_func_id, RedirectHandler rHandler);

	void removeRedirectFunction(uint32_t redirect_func_id);

	std::shared_ptr<std::shared_ptr<shared_redirect_memory>> _getRedirectMemory(uint32_t redirect_func_id);

	std::function<void(ClientHandle*, std::shared_ptr<R_Message>)> callRedirectFunction(uint32_t redirect_func_id);

	void addNewMessage(std::shared_ptr<S_Message> msg);

	size_t getSendMessageCount() const noexcept;

	size_t getRecieveMessageCount() const noexcept;

	size_t getConfimationMsgCount() const noexcept;

	asio::ip::udp::endpoint getEndpoint() const noexcept;

	void recieve(std::shared_ptr<R_Message> message);

	void recieveConfirmation(std::shared_ptr<C_Message> message);

	std::shared_ptr<R_Message> getRecievedMsg();

	std::mutex& getRecievedMutex();
};

extern void bindToServiceHub(ClientHandle* clientHandle);