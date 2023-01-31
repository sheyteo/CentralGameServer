#pragma once

#include "pch.h"
#include "NewClientHandle.h"

class Server
{
	friend class ClientHandle;
	friend class ClientHandleMapSection;
private:
	bool currently_functional = false;

	std::unique_ptr<asio::thread_pool> thread_pool;
	std::unique_ptr<asio::ip::udp::socket> _socket;

	asio::ip::udp::endpoint remoteEndpoint;

	std::unique_ptr<asio::steady_timer> cleanUpTimer;

	class ClientHandleMapSection
	{
		mutable std::shared_mutex clientHandleMapMutex;
		std::unordered_map<asio::ip::udp::endpoint, std::shared_ptr<ClientHandle>> ClientHandleMap;
		mutable std::shared_mutex sendOrderMutex;
		std::deque<asio::ip::udp::endpoint> sendOrder;
	public:
		void addClientHandle(const asio::ip::udp::endpoint&, std::shared_ptr<ClientHandle>);
		void removeClientHandle(const asio::ip::udp::endpoint&);
		std::shared_ptr<ClientHandle> getClientHandleOrder();
		std::shared_ptr<ClientHandle> getClientHandle(const asio::ip::udp::endpoint&) const;
		std::chrono::nanoseconds tempCleanUp();
		uint8_t try_offset(const asio::ip::udp::endpoint&,int8_t offset);
		void clear();
	};
	
	ClientHandleMapSection clms;

	void send();
	void sendCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<ClientHandle> clientHandle, std::shared_ptr<std::vector<char>>);

	void recieve();
	void recieveCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<asio::ip::udp::endpoint> clientEndpoint, std::shared_ptr<std::vector<char>>);

	void tempCleanUp();
public:
	Server();
	~Server();

	void start(uint8_t num_threads = 1, uint8_t executor_num_threads=1);
	void shutdown();

	bool isActive();
	
};