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
	uint32_t fast_redirect;
};

struct ConfiramtionMessage
{
	uint32_t type;
	uint32_t confirmID;
	uint64_t sendTime;
	uint64_t creationDate;
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
	C_Message(unsigned int ID);
	C_Message(const std::vector<char>& raw);
	ConfiramtionMessage& getMsg();

	std::vector<char> send();
};

class S_Message
{
	static unsigned int id_counter;
	MessageHeader* msgHeader{};
	std::vector<char> message;
public:
	void setData(std::vector<char> data, uint32_t fast_redirect_id = 0, Importance importance = Importance::High_Importance, Priority priority = Priority::High_Priotity);

	const std::vector<char>& send();

	const MessageHeader* getHeader() const;
};

class R_Message
{
	MessageHeader msgHeader{};
	std::vector<char> data;
public:
	void parse(const std::vector<char>& rawMessage); // posibility of performing move semantics to omit use of memcpy/ shared_ptr also posible

	const std::vector<char>& getData();

	const MessageHeader& getHeader();
};

class Custom_Message
{
private:
	std::vector<char> raw;
public:
	void push_back(void* data, size_t bytes);
	void push_back(uint32_t num);
	void push_back(const std::string& str);

	std::vector<char> getRaw();
};

class Custom_Message_View
{
private:
	const std::vector<char>& _view;
	uint32_t offset = 0;
public:
	Custom_Message_View(const std::vector<char>& view);
	std::string read_str();
	uint32_t read_uint32();
};