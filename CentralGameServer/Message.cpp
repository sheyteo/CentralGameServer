#include "Message.h"

Recv_Message::Recv_Message(const std::vector<char>& raw_buffer, size_t size)
{
	if (size < sizeof(NormalMessageHeader) || size > raw_buffer.size())
	{
		std::cout << "Length mismatch\n";
	}

	msgHeader.reset(new NormalMessageHeader());
	std::memcpy(msgHeader.get(), raw_buffer.data(), sizeof(NormalMessageHeader));

	if (true)
	{
		//error Handling
	}

	data.resize(size - sizeof(NormalMessageHeader));
	std::memcpy(data.data(), raw_buffer.data() + sizeof(NormalMessageHeader), data.size());
}

std::shared_ptr<const NormalMessageHeader> Recv_Message::getHeader() const noexcept
{
	return msgHeader;
}

const std::vector<char>& Recv_Message::getData() const noexcept
{
	return data;
}

Send_Message::Send_Message(const std::vector<char>& data, Fast_Redirect fast_redirect, Priority priority, Importance importance, uint32_t gameInstanceID)
	:completeMsg(data.size() + sizeof(NormalMessageHeader))
{
	std::memcpy(completeMsg.data() + sizeof(NormalMessageHeader), data.data(), data.size());

	P_NMH = (NormalMessageHeader*)completeMsg.data();

	P_NMH->ID = SendMessageIdCounter++;

	P_NMH->sendCount = 0;

	P_NMH->type = T_Normal;
	P_NMH->Importance = importance;
	P_NMH->Priority = priority;

	P_NMH->fast_redirect = fast_redirect;

	P_NMH->gameInstanceID = gameInstanceID;

	P_NMH->data_size = data.size();
	P_NMH->creationDate = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	uint32_t temp_checksum = 0;
	for (const auto& c : data)
	{
		temp_checksum += c;
	}
	P_NMH->checkSum = temp_checksum;
}

std::shared_ptr<Send_Message> Send_Message::create(const std::vector<char>& data, Fast_Redirect fast_redirect,
	Priority priority, Importance importance, uint32_t gameInstanceID)
{
	return std::make_shared<Send_Message>(data, fast_redirect, priority, importance, gameInstanceID);
}

std::shared_ptr<Send_Message> Send_Message::create_from_raw(const std::vector<char>& data)
{
	NormalMessageHeader* msg = (NormalMessageHeader*)data.data();
	return std::make_shared<Send_Message>(std::vector<char>(data.begin() + sizeof(NormalMessageHeader), data.end()),
		msg->fast_redirect, (Priority)msg->Priority,(Importance)msg->Importance,msg->gameInstanceID);
}

std::shared_ptr<std::vector<char>> Send_Message::getRaw()
{
	last_send = std::chrono::steady_clock::now();
	P_NMH->sendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(last_send.time_since_epoch()).count();
	P_NMH->sendCount += 1;
	return std::make_shared<std::vector<char>>(completeMsg);
}

bool Send_Message::check_valid(int future)
{
	if (P_NMH->Importance == Ensured_Importance)
	{
		return true;
	}
	else if (P_NMH->Importance == High_Importance)
	{
		if (P_NMH->sendCount + future >= H_I_T)
		{
			return false;
		}
	}
	else if (P_NMH->Importance == Middle_Importance)
	{
		if (P_NMH->sendCount + future >= M_I_T)
		{
			return false;
		}
	}
	else if (P_NMH->Importance == Low_Importance)
	{
		if (P_NMH->sendCount + future >= L_I_T)
		{
			return false;
		}
	}
	else if (P_NMH->Importance == None_Importance)
	{
		if (P_NMH->sendCount + future >= N_I_T)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool Send_Message::olderThan(std::chrono::steady_clock::duration dur)
{
	return (std::chrono::steady_clock::now() - last_send) > dur;
}

Confirm_Message::Confirm_Message(const std::vector<char>& raw, size_t size)
	:_raw(sizeof(ConfirmMessage))
{
	if (size != sizeof(ConfirmMessage))
	{
		std::cout << "Error in size mismatch in Confirm_Message\n";
		return;
	}
	this->cm = (ConfirmMessage*)_raw.data();
	std::memcpy(cm, raw.data(), sizeof(ConfirmMessage));

}

Confirm_Message::Confirm_Message(std::shared_ptr<Recv_Message> rmsg)
	:_raw(sizeof(ConfirmMessage))
{
	cm = (ConfirmMessage*)_raw.data();
	cm->type = T_Confirm;
	cm->confirmID = rmsg->msgHeader->ID;
	cm->triesNeeded = rmsg->msgHeader->sendCount;
}

std::shared_ptr<std::vector<char>> Confirm_Message::getRaw()
{
	cm->sendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	return std::make_shared<std::vector<char>>(_raw);
}

const ConfirmMessage* Confirm_Message::getCM()
{
	return cm;
}

Custom_Message::Custom_Message()
	: raw(new std::vector<char>(0))
{

}

void Custom_Message::push_back_void(void* data, size_t bytes)
{
	size_t raw_size = raw->size();
	raw->resize(bytes + raw_size);
	std::memcpy(raw->data() + raw_size, data, bytes);
}
void Custom_Message::push_back_ui32(uint32_t num)
{
	size_t raw_size = raw->size();
	raw->resize(sizeof(num) + raw_size);
	std::memcpy(raw->data() + raw_size, &num, sizeof(num));
}
void Custom_Message::push_back_str(const std::string& str)
{
	size_t raw_size = raw->size();
	uint32_t str_length = str.size();
	raw->resize(str.size() + raw_size + sizeof(str_length));
	std::memcpy(raw->data() + raw_size, &str_length, sizeof(str_length));
	std::memcpy(raw->data() + raw_size + sizeof(str_length), str.data(), str_length);
}

std::shared_ptr<std::vector<char>> Custom_Message::getRaw()
{
	return raw;
}


Custom_Message_View::Custom_Message_View(const std::vector<char>& view) :_view(view) {}

std::string Custom_Message_View::read_str()
{
	//maybe implement std::string_view later
	uint32_t _size = *((uint32_t*)(_view.data() + offset));
	std::string str(_size, '\0');
	std::memcpy(str.data(), _view.data() + offset + sizeof(_size), _size);
	offset += _size + sizeof(_size);
	return str;
}

uint64_t Custom_Message_View::read_uint64()
{
	uint64_t _num = *((uint64_t*)(_view.data() + offset));
	offset += sizeof(_num);
	return _num;
}

uint32_t Custom_Message_View::read_uint32()
{
	uint32_t _num = *((uint32_t*)(_view.data() + offset));
	offset += sizeof(_num);
	return _num;
}

uint16_t Custom_Message_View::read_uint16()
{
	uint16_t _num = *((uint16_t*)(_view.data() + offset));
	offset += sizeof(_num);
	return _num;
}

uint8_t Custom_Message_View::read_uint8()
{
	uint8_t _num = *((uint8_t*)(_view.data() + offset));
	offset += sizeof(_num);
	return _num;
}