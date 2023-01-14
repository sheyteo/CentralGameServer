#pragma once
#include "pch.h"
#include "Messages.h"
#include "ClientHandle.h"

class Server
{
	friend class QuizGameInstance;
private:
	asio::io_context io_context;
	std::unique_ptr<asio::ip::udp::socket> socket;
	std::vector < std::thread> executorThreads;

	int counterTest = 0;

	asio::ip::udp::endpoint remoteEndpoint;
	std::vector<char> buffer;
	asio::steady_timer cleanUpTimer;
	mutable std::shared_mutex CHM_Mutex;

	std::map<asio::ip::udp::endpoint, std::shared_ptr<ClientHandle>> ClientHandleMap;

	void recieve()
	{
		std::shared_ptr<asio::ip::udp::endpoint> clientEndpoint(new asio::ip::udp::endpoint); 
		socket->async_receive_from(asio::buffer(buffer.data(), buffer.size()),
			*clientEndpoint, std::bind(&Server::recieveCallback, this,
				std::placeholders::_1, std::placeholders::_2, clientEndpoint));
	}

	void recieveCallback(const asio::error_code& error, std::size_t bytes_transferred,
		std::shared_ptr<asio::ip::udp::endpoint> clientEndpoint)
	{
		counterTest++;
		/*std::cout << "Msg from " << clientEndpoint->address().to_string()
			<<":" << clientEndpoint->port()<<"\n"<<"Msg-Size :"
			<< bytes_transferred << "\n";*/
		Type* type{};
		type = reinterpret_cast<Type*>(buffer.data());
		if (*type == Normal)
		{
			std::shared_ptr<R_Message> msg(new R_Message());
			msg->parse(std::vector(buffer.begin(), buffer.begin() + bytes_transferred));

			CHM_Mutex.lock_shared();
			if (ClientHandleMap.contains(*clientEndpoint))
			{
				ClientHandleMap.at(*clientEndpoint)->recieve(msg);
				CHM_Mutex.unlock_shared();
			}
			else
			{
				std::shared_ptr<ClientHandle> cH(new ClientHandle(*clientEndpoint));
				cH->recieve(msg);

				CHM_Mutex.unlock_shared();
				std::unique_lock ul(CHM_Mutex); //Lock Mutex for everyone so writing is possible
				ClientHandleMap.insert(std::pair(*clientEndpoint, cH));
			}
		}
		else if (*type == Confirmation)
		{
			std::shared_ptr<C_Message> msg(new C_Message(std::move(std::vector(buffer.begin(), buffer.begin() + bytes_transferred))));

			CHM_Mutex.lock_shared();
			if (ClientHandleMap.contains(*clientEndpoint))
			{
				ClientHandleMap.at(*clientEndpoint)->recieveConfirmation(msg);
				CHM_Mutex.unlock_shared();
			}
			else
			{
				std::cout << "Most likley an error\n";
				std::shared_ptr<ClientHandle> cH(new ClientHandle(*clientEndpoint));
				cH->recieveConfirmation(msg);

				CHM_Mutex.unlock_shared();
				std::unique_lock ul(CHM_Mutex); //Lock Mutex for everyone so writing is possible
				ClientHandleMap.insert(std::pair(*clientEndpoint, cH));
			}

		}
		recieve();
	}

	void send()
	{
		std::shared_lock lg(CHM_Mutex);
		for (auto it = ClientHandleMap.begin(); it != ClientHandleMap.end(); it++)
		{
			auto& abc = *it;
			std::shared_ptr<ClientHandle> val = it->second;

			std::vector<char> msg;

			std::lock_guard lg(it->second->sendHandle.lock);
			std::lock_guard lg1(it->second->sendHandle.c_lock);
			if (val->getConfimationMsgCount() != 0)
			{
				msg = it->second->pollConfirmation()->send();
				socket->async_send_to(asio::buffer(msg.data(), msg.size()), it->second->getEndpoint(),
					std::bind(&Server::sendCallback, this, std::placeholders::_1, std::placeholders::_2, it->second));
				return;
			}
			else if (val->getSendMessageCount())
			{
				msg = it->second->poll()->send();
				socket->async_send_to(asio::buffer(msg.data(), msg.size()), it->second->getEndpoint(),
					std::bind(&Server::sendCallback, this, std::placeholders::_1, std::placeholders::_2, it->second));
				return;
			}
		}
		asio::steady_timer timer(io_context, std::chrono::milliseconds(10));
		timer.async_wait(std::bind(&Server::send, this));
	}

	void sendCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<ClientHandle> clientHandle)
	{
		if (error)
		{
			std::cout << "Error in sendCallback: " << error.message() << "\n";
		}
		send();
	}

	void tempCleanUp()
	{
		using namespace std::chrono_literals;
		CHM_Mutex.lock_shared();
		for (auto& [key, val] : ClientHandleMap)
		{
			if (val == nullptr) continue;  // Check if val is a null pointer
			if (val->recieveHandle.mmap.empty()) continue;

			if ((val->lastRecv - std::chrono::steady_clock::now()) >= 100s)  // close connection with  timeout handles
			{
				CHM_Mutex.unlock_shared();
				CHM_Mutex.lock();

				ClientHandleMap.erase(key);

				CHM_Mutex.unlock();
				CHM_Mutex.lock_shared();
				continue;
			}

			std::lock_guard gl(val->recieveHandle.lock);

			auto it = val->recieveHandle.mmap.begin();
			while (it != val->recieveHandle.mmap.end())
			{
				if (std::chrono::steady_clock::now() - it->second->timepoint > 20s) //discard messages older than 20 seconds
				{
					if (it->second->ValidIterator) // checks wether the message was already deleted
					{
						val->recieveHandle.messages.erase(it->second->fastAccess);
					}
					it = val->recieveHandle.mmap.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
		CHM_Mutex.unlock_shared();
		cleanUpTimer.async_wait(std::bind(&Server::tempCleanUp, this));
	}

	void sh()
	{
		std::shared_lock lg(CHM_Mutex);
		for (auto it = ClientHandleMap.begin(); it != ClientHandleMap.end(); it++)
		{
			while (it->second->getRecieveMessageCount())
			{
				auto a = it->second->getRecievedMsg();
				auto& b = a->getHeader();

				std::cout << "Sender: " << it->first.address().to_string() << ":" << it->first.port() << ", ID: " << b.ID << ", SendCount: " << b.sendCount << ", sTime: " << b.sendTime << ", Data: " << std::string(a->getData().data(), a->getData().size()) << "\n";
			}
		}
		sHtimer.async_wait(std::bind(&Server::sh, this));
	}

	void _start()
	{
		tempCleanUp();
#ifdef _DEBUG
		//sh();
#endif // _DEBUG
		recieve();
		send();
	}

public:
	asio::steady_timer sHtimer;
	Server()
		:remoteEndpoint(asio::ip::udp::v4(), 6969),
		cleanUpTimer(io_context, std::chrono::seconds(10)),
		sHtimer(io_context, std::chrono::milliseconds(100)),
		buffer(MaxUdpPacketSize)
	{
		socket.reset(new asio::ip::udp::socket(io_context, remoteEndpoint));
	}

	void start(uint8_t threadNum = 1)
	{
		if (threadNum == 0)
		{
			std::cout << "Warning: io_context not started due to 0 threadNum\n";
			return;
		}

		asio::post(io_context, std::bind(&Server::_start, this));

		for (size_t i = 0; i < threadNum; i++)
		{
			executorThreads.emplace_back([&]() {io_context.run(); });
		}
	}

	void adjustThreadNum(uint8_t offset)
	{
		for (int8_t i = 0; i < offset; i++)
		{
			executorThreads.emplace_back([&]() {io_context.run(); });
		}
		//TODO: update io_service to allow reduction in thread num
	}

	void stop()
	{
		io_context.stop();
		for (auto& thread : executorThreads)
		{
			thread.join();
		}
		executorThreads.clear();
		io_context.reset();
	}

	void shutdown()
	{
		if (!io_context.stopped())
		{
			stop();
		}
		std::unique_lock ul(this->CHM_Mutex);
		for (auto it = ClientHandleMap.begin(); it != ClientHandleMap.end(); ++it)
		{
			std::lock_guard lg(it->second->recieveHandle.lock);
			it->second->recieveHandle.messages.clear();
			it->second->recieveHandle.mmap.clear();
		}
		this->ClientHandleMap.clear();
	}

	std::pair<std::reference_wrapper<std::map<asio::ip::udp::endpoint, std::shared_ptr<ClientHandle>>>, std::reference_wrapper<std::shared_mutex>> get_CHM()
	{
		return std::make_pair(std::ref(ClientHandleMap), std::ref(CHM_Mutex));
	}

};