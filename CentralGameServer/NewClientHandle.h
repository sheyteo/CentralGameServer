#pragma once
#include "pch.h"
#include "RedirectHub.h"
#include "TaskExecutor.h"


class User
{
	bool valid = true;
	std::string uname;
	bool loged_in = false;

	std::shared_ptr<ExtentPtr<ClientHandle>> clHandle;

public:
	User(std::shared_ptr<ExtentPtr<ClientHandle>> clHandle);
	uint64_t ping(std::chrono::milliseconds* ms);
	void _login(std::shared_ptr<ExtentPtr<ClientHandle>>,std::shared_ptr<Recv_Message>);
	void _register(std::shared_ptr<ExtentPtr<ClientHandle>>, std::shared_ptr<Recv_Message>);
	void _disconnect(std::shared_ptr<ExtentPtr<ClientHandle>>, std::shared_ptr<Recv_Message>);
	void _fetch_info(std::shared_ptr<ExtentPtr<ClientHandle>>, std::shared_ptr<Recv_Message>);
};

class UserHub
{
	static std::vector<std::shared_ptr<User>> users;
public:
	static uint32_t _register()
	{
		//open database conn 
		//return status
		if (LR_SUCCESS) // add to server arr
		{

		}
		return LR_SUCCESS;
	}
	static uint32_t _login()
	{
		//open database conn 
		//return status
		return LR_SUCCESS;
	}

	static void reg(std::shared_ptr<User> user)
	{
		users.push_back(user);
		std::cout << "hey registered\n";
	}
};

class ClientHandle
{
	friend class Server;
private:
	static uint16_t clientIDCount;

public:
	const uint32_t this_ID = clientIDCount++;
	uint8_t connTries = 0;
private:
	bool disconnected = false;
	std::shared_mutex generalMutex;
	asio::ip::udp::endpoint endpoint;
	std::chrono::steady_clock::time_point lastRecv;

	//Send Section
	struct SendHandle
	{
		struct SendStruct
		{
			std::list<std::shared_ptr<Send_Message>>::iterator fastAccess;
			bool validIterator{};
		};
		static SendStruct createSendStruct(const std::list<std::shared_ptr<Send_Message>>::iterator iter);
		std::shared_mutex sendMutex;
		std::list<std::shared_ptr<Send_Message>> sendMessages;
		std::unordered_map<uint64_t, SendStruct> sendMessageMap;
	};
	
	//Recieve Section
	struct RecvHandle
	{
		struct RecvStruct
		{
			std::chrono::steady_clock::time_point timepoint;
			bool ValidIterator;
			std::list<std::shared_ptr<Recv_Message>>::iterator fastAccess{};
		};
		static RecvStruct createRecvStruct(std::list<std::shared_ptr<Recv_Message>>::iterator fastAccess, bool validIterator = true);
		std::shared_mutex recvMutex;
		std::list<std::shared_ptr<Recv_Message>> recvMessages;
		std::unordered_map<uint64_t,RecvStruct> recvMessageMap;
	};

	//Confirm Section
	struct ConfirmHandle
	{
		std::shared_mutex cofirmMutex;
		std::list<std::shared_ptr<Confirm_Message>> confirmMessages;
	};

	bool valid = true;

	std::shared_ptr<ExtentPtr<ClientHandle>> sptr_this;

	SendHandle sendHandle;
	RecvHandle recvHandle;
	ConfirmHandle confirmHandle;
public:
	std::shared_ptr<User> userInterface;
private:

public:
	ClientHandle(const asio::ip::udp::endpoint& endpoint);

	void disconnect();

	void bind();

	void addSendMessage(std::shared_ptr<Send_Message> smsg);

	void addRecvMessage(std::shared_ptr<Recv_Message> msg);

	std::shared_ptr<Send_Message> poll();

	std::shared_ptr<Confirm_Message> c_poll();

	void confirmMessage(std::shared_ptr<Confirm_Message> msg);

	size_t getSendMessageCount() const noexcept;

	size_t getRecieveMessageCount() const noexcept;

	size_t getConfimationMsgCount() const noexcept;

	const asio::ip::udp::endpoint& getEndpoint() const noexcept;

	void cleanUp();

	void tempCleanUp();

	~ClientHandle();

	//TODO some error handling + add support for new version on client side
	//keep mutex lifetime in mind
};