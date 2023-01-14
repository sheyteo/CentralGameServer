#pragma once
#include "pch.h"

#define MiddlePriorityMaxTries 5
#define MaxUdpPacketSize 512

struct MessageHeader
{
	uint32_t type;
	uint32_t ID;
	uint32_t sendCount;
	uint64_t sendTime;
	uint64_t creationDate;
	uint8_t Priority;
	uint8_t Importance;
	uint32_t data_size;
	uint32_t checkSum;
	uint8_t msgInfoType;
	uint32_t fast_redirect;
};

struct ConfiramtionMessage
{
	uint32_t type;
	uint32_t confirmID;
	uint64_t sendTime;
	uint64_t creationDate;
};

enum InfoType
{
	Ping = 0ui8,
	ServiceHubInteractions = 10ui8,
	GameInfo = 127ui8,
	GameStart = 128ui8,
	Debug = 255ui8
};

enum Type
{
	Normal = 0,
	Confirmation = 1
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


class C_Message
{
	ConfiramtionMessage msg{};
public:
	C_Message(unsigned int ID)
	{
		msg.type = Confirmation;
		msg.confirmID = ID;
		msg.creationDate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
	C_Message(const std::vector<char>& raw)
	{
		std::memcpy(&msg, raw.data(), sizeof(ConfiramtionMessage));
	}
	ConfiramtionMessage& getMsg()
	{
		return msg;
	}

	std::vector<char> send()
	{
		this;
		msg.sendTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		std::vector<char> raw(sizeof(msg));
		std::memcpy(raw.data(), &msg, sizeof(ConfiramtionMessage));
		return raw;
	}
};

class S_Message
{
	static unsigned int id_counter;
	MessageHeader* msgHeader{};
	std::vector<char> message;
public:
	void setData(std::vector<char> data, uint32_t fast_redirect_id = 0, Importance importance = Importance::High_Importance, Priority priority = Priority::High_Priotity, InfoType infoType = InfoType::Debug)
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
		msgHeader->msgInfoType = infoType;
		msgHeader->fast_redirect = fast_redirect_id;
		id_counter += 1;
	}

	const std::vector<char>& send()
	{
		msgHeader->sendTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		msgHeader->sendCount += 1;
		if (message.size() > MaxUdpPacketSize)
		{
			std::cout << "Max Packet Size overrun\n";
		}
		return message;
	}

	const MessageHeader* getHeader() const
	{
		if (msgHeader == nullptr)
		{

		}
		return msgHeader;
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
		if (rawMessage.size() == 0)
		{
			return;
		}
		if (rawMessage.size() < sizeof(MessageHeader))
		{
			std::cout << "Invalid Message length recieved\n";
			//throw std::exception("Wrong Message Recieve in R_Message::parse()");
		}

		std::memcpy(&msgHeader, rawMessage.data(), sizeof(MessageHeader));
		if (rawMessage.size() - sizeof(MessageHeader) != msgHeader.data_size)
		{
			std::cout << "Recieved Datasize is not matching with actual Size\n";
			//throw std::exception("Non matching datasize in Header in R_Message::parse()");
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
			std::cout << "checksums not matching possible wrong data\n";
			//throw std::exception("checksums not matching possible wrong data");
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

class Custom_Message
{
private:
	std::vector<char> raw;
public:
	void push_back(void* data, size_t bytes)
	{
		size_t raw_size = raw.size();
		raw.resize(bytes + raw_size);
		std::memcpy(raw.data() + raw_size, data, bytes);
	}
	void push_back(uint32_t num)
	{
		size_t raw_size = raw.size();
		raw.resize(sizeof(num) + raw_size);
		std::memcpy(raw.data() + raw_size, &num, sizeof(num));
	}
	void push_back(const std::string& str)
	{
		size_t raw_size = raw.size();
		uint32_t str_length = str.size();
		raw.resize(str.size() + raw_size + sizeof(str_length));
		std::memcpy(raw.data() + raw_size, &str_length, sizeof(str_length));
		std::memcpy(raw.data() + raw_size + sizeof(str_length), str.data(), str_length);
	}

	std::vector<char> getRaw()
	{
		return raw;
	}
};

class Custom_Message_View
{
private:
	std::vector<char>& _view;
	uint32_t offset = 0;
public:
	Custom_Message_View(std::vector<char>& view)
		:_view(view)
	{

	}
	std::string read_str()
	{
		uint32_t _size = *((uint32_t*)(_view.data() + offset));
		std::string str(_size, '\0');
		std::memcpy(str.data(), _view.data() + offset + sizeof(_size), _size);
		offset += _size + sizeof(_size);
		return str;
	}
	uint32_t read_uint32()
	{
		uint32_t _num = *((uint32_t*)(_view.data() + offset));
		offset += sizeof(_num);
		return _num;
	}
};