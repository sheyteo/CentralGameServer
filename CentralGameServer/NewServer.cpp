#include "NewServer.h"

void Server::ClientHandleMapSection::addClientHandle(const asio::ip::udp::endpoint& endp, std::shared_ptr<ClientHandle> clientHandle)
{	
	std::shared_lock ssl(sendOrderMutex);
	if (std::find(sendOrder.begin(), sendOrder.end(), endp) != sendOrder.end())
	{
		std::cout << "Errorrrrrr\n";
		return;
	}
	ssl.unlock();
	try
	{
		std::unique_lock ul(clientHandleMapMutex);
		ClientHandleMap.emplace(endp, clientHandle);
		ul.unlock();
		std::unique_lock ul1(sendOrderMutex);
		sendOrder.push_front(endp);
	}
	catch (const std::exception&)
	{
		std::cout << "emplace error addClientHandle()\n";
		return;
	}
}

void Server::ClientHandleMapSection::removeClientHandle(const asio::ip::udp::endpoint& endp)
{
	std::unique_lock cul(clientHandleMapMutex);
	sendOrder.erase(std::remove(sendOrder.begin(), sendOrder.end(), endp));
	auto cit = ClientHandleMap.find(endp);
	if (cit != ClientHandleMap.end())
	{
		cit->second->cleanUp();
		ClientHandleMap.erase(cit);
	}
}


std::shared_ptr<ClientHandle> Server::ClientHandleMapSection::getClientHandleOrder()
{
	try
	{
		std::unique_lock cul(sendOrderMutex);
		if (sendOrder.size() == 0)
		{
			return nullptr;
		}
		auto& front = sendOrder.front();
		std::shared_lock sl(clientHandleMapMutex);
		auto& temp = ClientHandleMap.at(front);

		if (sendOrder.size() >= 2)
		{
			sendOrder.push_back(front);
			sendOrder.pop_front();
		}
		
		return temp;
	}
	catch (const std::exception&)
	{
		std::cout << "gcho\n";
		return nullptr;
	}
}

std::shared_ptr<ClientHandle> Server::ClientHandleMapSection::getClientHandle(const asio::ip::udp::endpoint& endp) const
{
	try
	{
		std::shared_lock<std::shared_mutex> sl(clientHandleMapMutex);
		return ClientHandleMap.at(endp);
	}
	catch (const std::exception&)
	{
		return nullptr;
	}
}

std::chrono::nanoseconds Server::ClientHandleMapSection::tempCleanUp()
{
	auto start = std::chrono::steady_clock::now();
	std::shared_lock sl(clientHandleMapMutex);
	for (auto it = ClientHandleMap.begin(); it != ClientHandleMap.end();)
	{
		if (it->second->disconnected)
		{
			{
				std::unique_lock ul0(sendOrderMutex);
				auto it1 = std::find(sendOrder.begin(), sendOrder.end(), it->first);
				if (it1 == sendOrder.end())
				{
					std::cout << "Warning tempCleanUp not there\n";
				}
				else
				{
					sendOrder.erase(it1);
				}
			}
			sl.unlock();
			{
				std::unique_lock ul1(clientHandleMapMutex);
				it = ClientHandleMap.erase(it);
			}
			sl.lock();
		}
		else
		{
			it->second->tempCleanUp();
			it++;
		}
	}
	std::chrono::nanoseconds elapsed = std::chrono::steady_clock::now() - start;

	std::cout << "TempCleanUp time : " << (elapsed.count() / 1000000.f) << "ms\n";

	std::chrono::nanoseconds left = (5s - elapsed);

	if (left < 0ns)
	{
		left = 0ns;
	}
	return left;
}

uint8_t Server::ClientHandleMapSection::try_offset(const asio::ip::udp::endpoint& endp,int8_t offset)
{
	try
	{
		std::shared_lock sl(clientHandleMapMutex);
		if (offset == 0)
		{
			ClientHandleMap.at(endp)->connTries = 0;
		}
		else
		{
			ClientHandleMap.at(endp)->connTries += offset;
		}
		return ClientHandleMap.at(endp)->connTries;
	}
	catch (const std::exception&)
	{
		return 0;
	}
}

void Server::ClientHandleMapSection::clear()
{
	std::unique_lock ul(sendOrderMutex);
	std::unique_lock ul1(clientHandleMapMutex);
	for (auto& [endp,client]: ClientHandleMap)
	{
		client->cleanUp();
	}
	ClientHandleMap.clear();
	sendOrder.clear();
}

void Server::send()
{
	auto clH = clms.getClientHandleOrder();
	if (clH == nullptr)
	{
		std::this_thread::sleep_for(1ms);
		return asio::post(*thread_pool, std::bind(&Server::send, this));
	}
	try
	{
		if (clH->valid)
		{
			std::shared_ptr<std::vector<char>> data;
			std::shared_ptr<Confirm_Message> cMsg = clH->c_poll();
			if (cMsg != nullptr)
			{
				data = cMsg->getRaw();
			}
			else
			{
				std::shared_ptr<Send_Message> sMsg = clH->poll();
				if (sMsg != nullptr)
				{
					data = sMsg->getRaw();
				}
			}
			if (data != nullptr)
			{
				return _socket->async_send_to(asio::buffer(*data),
					clH->getEndpoint(), std::bind(&Server::sendCallback, this,
						std::placeholders::_1, std::placeholders::_2, clH, data));
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "error\n";
	}
	std::this_thread::sleep_for(1ns);
	return asio::post(*thread_pool, std::bind(&Server::send, this));
}

void Server::sendCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<ClientHandle> clientHandle, std::shared_ptr<std::vector<char>> raw)
{
	if (error.value() == asio::error::connection_reset || error.value() == asio::error::connection_refused)
	{
		if (clms.try_offset(clientHandle->endpoint,1) >= 10)
		{
			clms.removeClientHandle(clientHandle->endpoint);
		}
		else
		{
			clms.getClientHandle(clientHandle->endpoint)->addSendMessage(Send_Message::create_from_raw(*raw));
		}
		return asio::post(*thread_pool, std::bind(&Server::send, this));
	}
	else if(error)
	{
		//eror handling missing
		std::cout << "Error in sendCallback: " << error.message()<<" "<< error.category().name() << "\n";
	}
	else
	{
		clms.try_offset(clientHandle->endpoint, 0);
	}
	asio::post(*thread_pool, std::bind(&Server::send, this));
}

void Server::recieve()
{
	std::shared_ptr<asio::ip::udp::endpoint> clientEndpoint(new asio::ip::udp::endpoint);
	auto temp = std::make_shared<std::vector<char>>(MaxUdpPacketSize);
	_socket->async_receive_from(asio::buffer(*temp),
		*clientEndpoint, std::bind(&Server::recieveCallback, this,
			std::placeholders::_1, std::placeholders::_2, clientEndpoint,temp));
}

void Server::recieveCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<asio::ip::udp::endpoint> clientEndpoint, std::shared_ptr<std::vector<char>> raw)
{
	if (error.value() == asio::error::connection_reset || error.value() == asio::error::connection_refused)
	{
		if (clms.try_offset(*clientEndpoint, 1) >= 10)
		{
			clms.removeClientHandle(*clientEndpoint);
		}
	}
	else if (!error)
	{
		MessageType type = peakType(raw->data());
		auto clh = clms.getClientHandle(*clientEndpoint);
		if (type == T_Normal)
		{
			std::shared_ptr<Recv_Message> msg(new Recv_Message(*raw, bytes_transferred));
			//ignore old messages
			if (clh == nullptr)
			{
				std::shared_ptr<ClientHandle> cH(new ClientHandle(*clientEndpoint));
				cH->addRecvMessage(msg);
				clms.addClientHandle(*clientEndpoint, cH);
			}
			else
			{
				clh->addRecvMessage(msg);
			}
		}
		else if (type == T_Confirm)
		{
			std::shared_ptr<Confirm_Message> msg(new Confirm_Message(*raw, bytes_transferred));
			if (clh == nullptr)
			{
				std::cout << "Most likely an error due to disconnected client\n";
			}
			else
			{
				clh->confirmMessage(msg);
			}
		}
		else
		{
			std::cout << "Error wrong msg start\n";
			//error Handling;
		}
	}
	else
	{
		//eror handling missing
		std::cout << "Error in recvCallback: " << error.message() << "\n";
	}
	asio::post(*thread_pool, std::bind(&Server::recieve, this));
}

void Server::tempCleanUp()
{
	std::chrono::nanoseconds time = clms.tempCleanUp();
	cleanUpTimer->expires_after(time);
	cleanUpTimer->async_wait(std::bind(&Server::tempCleanUp, this));
}

Server::Server()
	:remoteEndpoint(asio::ip::udp::v4(), 6969)
{
}

void Server::start(uint8_t num_threads, uint8_t executor_num_threads)
{
	if (currently_functional)
	{
		throw std::exception("Already running");;
	}
	if (num_threads == 0 || executor_num_threads == 0)
	{
		throw std::exception("Starting Server with 0 threads");
	}
	thread_pool.reset(new asio::thread_pool(num_threads));

	TaskExecutor::start(executor_num_threads); // start TaskExecutor service

	_socket.reset(new asio::ip::udp::socket(*thread_pool, remoteEndpoint));
	cleanUpTimer.reset(new asio::steady_timer(*thread_pool, 5s));
	asio::post(*thread_pool, std::bind(&Server::recieve, this));
	asio::post(*thread_pool, std::bind(&Server::send, this));
	asio::post(*thread_pool, std::bind(&Server::tempCleanUp, this));
	

	currently_functional = true;
}
bool Server::isActive()
{
	return currently_functional;
}

void Server::shutdown()
{
	if (!currently_functional)
	{
		throw std::exception("Not running");
	}

	currently_functional = false;

	_socket->cancel();

	clms.clear();

	_socket->close();

	thread_pool->stop();

	_socket.reset();
	thread_pool.reset();
}

Server::~Server()
{
	if (currently_functional)
	{
		shutdown();
	}
}