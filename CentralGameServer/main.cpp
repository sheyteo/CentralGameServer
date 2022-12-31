#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <chrono>
#include "asio/include/asio.hpp"

#define MaxUdpPacketSize 512

struct MessageHeader
{
	unsigned int ID;
	unsigned int sendCount;
	unsigned long int sendTime;
	unsigned long int creationDate; // For performance Tests
	unsigned char Priority; // useless for now
	unsigned char Importance;
	unsigned int data_size;
	unsigned int checkSum; //Testing needed
};

enum Type
{
	Incoming,
	Outgoing
};
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

class S_Message
{
	static unsigned int id_counter;
	MessageHeader* msgHeader{};
	std::vector<char> message;
public:
	void setData(std::vector<char> data, Importance importance, Priority priority)
	{
		unsigned int datasize = data.size();
		message.resize(datasize + sizeof(MessageHeader));
		std::memcpy(message.data() + sizeof(MessageHeader), data.data(), datasize);
		msgHeader = reinterpret_cast<MessageHeader*>(message.data());
		msgHeader->creationDate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		msgHeader->Importance = importance;
		msgHeader->Priority = priority;
		unsigned int temp = 0;
		for (size_t i = 0; i < datasize; i++)
		{
			temp += data.at(i);
		}
		msgHeader->checkSum = temp;
		msgHeader->data_size = datasize;
		msgHeader->ID = id_counter;
		id_counter += 1;
	}

	const std::vector<char>& send()
	{
		msgHeader->sendTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		msgHeader->sendCount += 1;
		if (message.size()>MaxUdpPacketSize)
		{
			throw std::exception("Max packet size overwritten");
		}
		return message;
	}
};

unsigned int S_Message::id_counter = 0;

class R_Message
{
	MessageHeader msgHeader{};
	std::vector<char> data;
public:
	void parse(const std::vector<char>& rawMessage) // posibility of performing move semantics to omit use of memcpy/ shared_ptr also posible
	{
		if(rawMessage.size()<sizeof(MessageHeader))
		{
			throw std::exception("Wrong Message Recieve in R_Message::parse()");
		}
		
		std::memcpy(&msgHeader, rawMessage.data(), sizeof(MessageHeader));
		if (rawMessage.size() - sizeof(MessageHeader) != msgHeader.data_size)
		{
			throw std::exception("Non matching datasize in Header in R_Message::parse()");
		}
		data.resize(msgHeader.data_size);
		std::memcpy(data.data(), rawMessage.data() + sizeof(MessageHeader), msgHeader.data_size);
		unsigned int checksum = 0;
		for (size_t i = 0; i < msgHeader.data_size; i++)
		{
			checksum += data.at(i);
		}
		if (checksum != msgHeader.checkSum)
		{
			throw std::exception("checksums not matching possible wrong data");
		}
	}

	const std::vector<char>& getData()
	{
		return data;
	}

	const MessageHeader& getHeader()
	{
		return msgHeader;
	}
};


class ClientHandle
{
	class Recieve
	{
	public:
		std::vector<std::shared_ptr<R_Message>> messages;
		std::vector<unsigned int> recvMsgID;
	};

	class Send
	{
	public:
		std::vector<std::shared_ptr<S_Message>> messages; // optimize iwth priority queue
	};

	asio::ip::udp::endpoint endpoint;
	Recieve recieveHandle;
	Send sendHandle;
public:
	ClientHandle(const asio::ip::udp::endpoint& endpoint) : endpoint(endpoint){}

	std::shared_ptr<S_Message> poll() const
	{
		if (sendHandle.messages.size() == 0)
		{
			return nullptr;
		}
		else
		{
			return sendHandle.messages.at(0);
		}
	}

	void addNewMessage(std::shared_ptr<S_Message> msg)
	{
		sendHandle.messages.push_back(msg);
	}

	size_t getSendMessageCount()
	{
		return sendHandle.messages.size();
	}

	const asio::ip::udp::endpoint& getEndpoint() const noexcept
	{
		return endpoint;
	}

	void recieve(std::shared_ptr<R_Message> message)
	{
		recieveHandle.messages.push_back(message);
		recieveHandle.recvMsgID.push_back(message->getHeader().ID);
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

		std::shared_ptr<R_Message> msg(new R_Message());
		msg->parse(buffer);

		if (ClientHandleMap.contains(*clientEndpoint))
		{
			ClientHandleMap.at(*clientEndpoint)->recieve(msg);
		}
		else
		{
			std::shared_ptr<ClientHandle> cH(new ClientHandle(*clientEndpoint));
			cH->recieve(msg);
			ClientHandleMap.insert(std::pair(*clientEndpoint, cH));
		}
		recieve();
	}

	void send()
	{
		
		for (auto& [key, val] : ClientHandleMap)
		{
			if (val->getSendMessageCount() != 0)
			{
				return send_impl(val);
			}
		}
		asio::steady_timer timer(io_context, std::chrono::milliseconds(1));
		timer.async_wait(std::bind(&Server::send,this));
	}

	void send_impl(std::shared_ptr<ClientHandle> clientHandle)
	{
		std::vector<char> msg = clientHandle->poll()->send();
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
	}

	void start()
	{
		recieve();
		send();
		io_context.run();
	}
};

int main()
{
	Server server;
	server.start();
	return 0;
}