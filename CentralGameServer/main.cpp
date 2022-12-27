#include <iostream>
#include <vector>
#include <map>
#include <../asio/include/asio.hpp>

#define MaxUdpPacketSize 512



class Message
{
private:
	static unsigned int id_counter;
	std::vector<char> data = {};
	unsigned int ID;
	bool recieving = false;
public:
	
	enum Priority
	{
		High_Priotity,
		Middle_Priotity,
		Low_Priotity
	};

	enum Importance
	{
		High_Importance,
		Middle_Importance,
		Low_Importance,
		None_Importance
	};

	Message()
	{
		ID = id_counter;
		id_counter++;
	}
	std::vector<char> getData() const
	{
		return data;
	}

	void setData(const char* data, size_t size)
	{
		recieving = true;
		this->data = std::vector<char>(size);
		std::memcpy(this->data.data(), data, size);
	}
};

unsigned int Message::id_counter = 0;

class ClientHandle
{
	asio::ip::udp::endpoint endpoint;
	mutable std::vector<std::shared_ptr<Message>> msgSend;
	mutable std::vector<std::shared_ptr<Message>> msgRecieved;
public:
	ClientHandle(const asio::ip::udp::endpoint& endpoint) : endpoint(endpoint){}

	std::shared_ptr<Message> getToSend()
	{
		if (msgSend.size() == 0)
		{
			return nullptr;
		}
		else
		{
			return msgSend.at(0);
		}
	}

	size_t getMsgCountSend() const
	{
		return msgSend.size();
	}

	size_t getMsgCountRecieve() const
	{
		return msgRecieved.size();
	}

	void setRecieved(std::shared_ptr<Message> msg)
	{
		msgRecieved.push_back(msg);
	}

	const asio::ip::udp::endpoint& getEndpoint()
	{
		return endpoint;
	}
};

class Server
{
private:
	asio::io_context io_context;
	std::unique_ptr<asio::ip::udp::socket> socket;
	asio::ip::udp::endpoint remoteEndpoint;
	std::vector<char> buffer;
	
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
		std::cout << "Msg from " << clientEndpoint->address().to_string()
			<<":" << clientEndpoint->port()<<"\n"<<"Msg-Content :"
			<< std::string(buffer.data(),bytes_transferred) << "\n";

		std::shared_ptr<Message> msg(new Message());
		msg->setData(buffer.data(), bytes_transferred);

		if (ClientHandleMap.contains(*clientEndpoint))
		{
			ClientHandleMap.at(*clientEndpoint)->setRecieved(msg);
		}
		else
		{
			std::shared_ptr<ClientHandle> cH(new ClientHandle(*clientEndpoint));
			cH->setRecieved(msg);
			ClientHandleMap.insert(std::pair(*clientEndpoint, cH));
		}
		recieve();
	}

	void send()
	{
		
		for (auto& [key, val] : ClientHandleMap)
		{
			if (val->getMsgCountSend() != 0)
			{
				return send_impl(val);
			}
		}
		asio::steady_timer timer(io_context, std::chrono::milliseconds(1));
		timer.async_wait(std::bind(&Server::send,this));
	}

	void send_impl(std::shared_ptr<ClientHandle> clientHandle)
	{
		std::vector<char> msg = clientHandle->getToSend()->getData();
		socket->async_send_to(asio::buffer(msg.data(), msg.size()), clientHandle->getEndpoint(),
			std::bind(&Server::sendCallback, this, std::placeholders::_1, std::placeholders::_2, clientHandle));
	}

	void sendCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<ClientHandle> clientHandle)
	{
		if (error)
		{
			std::cout << "Error in sendCallback: " << error.message() << "\n";
		}

		std::cout << bytes_transferred << " Bytes send to"
			<<clientHandle->getEndpoint().address().to_string()<<":"
			<< clientHandle->getEndpoint().port()<<"\n";
		send();
		//TODO: delete msg out of msg queue with ID
	}
public:
	Server()
		:remoteEndpoint(asio::ip::udp::v4(), 6969),
		io_context(),
		buffer(MaxUdpPacketSize)
	{
		socket.reset(new asio::ip::udp::socket(io_context, remoteEndpoint));
		recieve();
		send();
		io_context.run();
	}
};

int main()
{
	Server server;
	return 0;
}