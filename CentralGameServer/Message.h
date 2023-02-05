#include "pch.h"
#define MaxUdpPacketSize 512

enum MessageType
{
	T_Normal = 0,
	T_Confirm = 1
};

template<class T>
MessageType peakType(T* ptr)
{
	return *reinterpret_cast<MessageType*>(ptr);
}

template<class T>
struct ExtentPtr
{
	T* value{};
	bool valid = false;
	static std::shared_ptr<ExtentPtr<T>> create(T* _ptr)
	{
		std::shared_ptr<ExtentPtr<T>> ex(new ExtentPtr<T>);
		ex->value = _ptr;
		ex->valid = true;
		return ex;
	}
};

enum MessageDirection
{
	Outgoing = 0,
	Incoming = 1
};

enum Priority
{
	High_Priotity = 0,
	Middle_Priotity = 1,
	Low_Priotity = 2
};

enum Importance
{
	High_Importance = 0,
	Middle_Importance = 1,
	Low_Importance = 2,
	None_Importance = 3,
	Ensured_Importance = 4,
};

enum ImportanceTries
{
	N_I_T = 1,
	L_I_T = 3,
	M_I_T = 7,
	H_I_T = 100
};

static uint64_t SendMessageIdCounter = 0;

struct Fast_Redirect
{
	uint16_t funcID;
	uint64_t ServerMemID;
	uint64_t ClientMemID;
	static Fast_Redirect create(uint16_t funcID, uint64_t ServerMemID, uint64_t ClientMemID)
	{
		Fast_Redirect fr{};
		fr.funcID = funcID;
		fr.ServerMemID = ServerMemID;
		fr.ClientMemID = ClientMemID;
		return fr;
	}
};

struct NormalMessageHeader
{
	uint8_t type;
	uint64_t ID;
	uint64_t sendTime;
	uint64_t creationDate;
	uint16_t sendCount;
	uint8_t Priority;
	uint8_t Importance;
	uint32_t data_size;
	uint32_t checkSum;
	Fast_Redirect fast_redirect;
};

class Recv_Message
{
	friend class Confirm_Message;
private:
	std::vector<char> data;
	std::shared_ptr<NormalMessageHeader> msgHeader;
public:
	Recv_Message(const std::vector<char>& raw_buffer, size_t size);

	//Read-only func
	std::shared_ptr<const NormalMessageHeader> getHeader() const noexcept;
	//Read-only func
	const std::vector<char>& getData() const noexcept;
};

class Send_Message
{
	friend class SendMessageHandle;
	friend class ClientHandle;
private:
	std::vector<char> completeMsg;
	NormalMessageHeader* P_NMH;
	std::chrono::steady_clock::time_point last_send;

public:
	Send_Message(const std::vector<char>& data, Fast_Redirect fast_redirect = Fast_Redirect{},
		Priority priority = Priority::High_Priotity, Importance importance = Importance::High_Importance);

	static std::shared_ptr<Send_Message> create_from_raw(const std::vector<char>& data);

	static std::shared_ptr<Send_Message> create(const std::vector<char>& data, Fast_Redirect fast_redirect = Fast_Redirect{},
		Priority priority = Priority::High_Priotity, Importance importance = Importance::High_Importance);

	std::shared_ptr<std::vector<char>> getRaw();

	bool check_valid(int future = 0);
	bool olderThan(std::chrono::steady_clock::duration dur);
};

struct ConfirmMessage
{
	uint8_t type;
	uint64_t sendTime;
	uint16_t triesNeeded;
	uint64_t confirmID;
};

class Confirm_Message
{
	ConfirmMessage* cm{};
	std::vector<char> _raw;
public:
	Confirm_Message(const std::vector<char>& raw, size_t size);
	Confirm_Message(std::shared_ptr<Recv_Message> rmsg);

	std::shared_ptr<std::vector<char>> getRaw();
	const ConfirmMessage* getCM();
};

class SendMessageHandle
{
	friend class ClientHandle;
private:
	const uint64_t ID;
	uint16_t triesNeeded = 0;
	SendMessageHandle(uint64_t id)
		:ID(id)
	{}
public:
	const uint16_t& getTries()
	{
		return triesNeeded;
	}
	bool has_arrived()
	{
		return triesNeeded != 0;
	}
};

class Custom_Message
{
private:
	std::shared_ptr<std::vector<char>> raw;
public:
	Custom_Message();
	void push_back_void(void* data, size_t bytes);
	void push_back_ui32(uint32_t num);
	void push_back_str(const std::string& str);
	template<class T>
	void push_back(const T& obj)
	{
		size_t size = raw->size();
		raw->resize(size + sizeof(T));
		std::memcpy(raw->data(), &obj, sizeof(T));
	}
	std::shared_ptr<std::vector<char>> getRaw();
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
	template<class T>
	const T* read()
	{
		T* t_ptr = _view.data() + offset;
		offset += sizeof(T);
		return t_ptr;
	}
};