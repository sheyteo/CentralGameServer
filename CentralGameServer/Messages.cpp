#include "Messages.h"

unsigned int S_Message::id_counter = 0;


C_Message::C_Message(unsigned int ID)
{
	msg.type = Confirmation;
	msg.confirmID = ID;
	msg.creationDate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
C_Message::C_Message(const std::vector<char>& raw)
{
	std::memcpy(&msg, raw.data(), sizeof(ConfiramtionMessage));
}
ConfiramtionMessage& C_Message::getMsg()
{
	return msg;
}

std::vector<char> C_Message::send()
{
	msg.sendTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	std::vector<char> raw(sizeof(msg));
	std::memcpy(raw.data(), &msg, sizeof(ConfiramtionMessage));
	return raw;
}

void S_Message::setData(std::vector<char> data, uint32_t fast_redirect_id, Importance importance, Priority priority)
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
	msgHeader->fast_redirect = fast_redirect_id;
	id_counter += 1;
}

const std::vector<char>& S_Message::send()
{
	msgHeader->sendTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	msgHeader->sendCount += 1;
	if (message.size() > MaxUdpPacketSize)
	{
		std::cout << "Max Packet Size overrun\n";
	}
	return message;
}

const MessageHeader* S_Message::getHeader() const
{
	return msgHeader;
};


void R_Message::parse(const std::vector<char>& rawMessage) // posibility of performing move semantics to omit use of memcpy/ shared_ptr also posible
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

const std::vector<char>& R_Message::getData()
{
		return data;
}

const MessageHeader& R_Message::getHeader()
{
	return msgHeader;
}


void Custom_Message::push_back(void* data, size_t bytes)
{
	size_t raw_size = raw.size();
	raw.resize(bytes + raw_size);
	std::memcpy(raw.data() + raw_size, data, bytes);
}
void Custom_Message::push_back(uint32_t num)
{
	size_t raw_size = raw.size();
	raw.resize(sizeof(num) + raw_size);
	std::memcpy(raw.data() + raw_size, &num, sizeof(num));
}
void Custom_Message::push_back(const std::string& str)
{
	size_t raw_size = raw.size();
	uint32_t str_length = str.size();
	raw.resize(str.size() + raw_size + sizeof(str_length));
	std::memcpy(raw.data() + raw_size, &str_length, sizeof(str_length));
	std::memcpy(raw.data() + raw_size + sizeof(str_length), str.data(), str_length);
}

std::vector<char> Custom_Message::getRaw()
{
	return raw;
}


Custom_Message_View::Custom_Message_View(const std::vector<char>& view) :_view(view) {}

std::string Custom_Message_View::read_str()
{
	uint32_t _size = *((uint32_t*)(_view.data() + offset));
	std::string str(_size, '\0');
	std::memcpy(str.data(), _view.data() + offset + sizeof(_size), _size);
	offset += _size + sizeof(_size);
	return str;
}

uint32_t Custom_Message_View::read_uint32()
{
	uint32_t _num = *((uint32_t*)(_view.data() + offset));
	offset += sizeof(_num);
	return _num;
}