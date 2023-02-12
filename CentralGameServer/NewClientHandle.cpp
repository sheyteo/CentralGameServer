#include "NewClientHandle.h"

User::User(std::shared_ptr<ExtentPtr<ClientHandle>> clHandle)
	:clHandle(clHandle)
{

}

//void User::_register(std::shared_ptr<ExtentPtr<ClientHandle>> _,std::shared_ptr<Recv_Message> msg)
//{
//	if (msg == nullptr)
//	{
//		return;
//	}
//	Custom_Message_View::
//	msg->getData().size();
//	Send_Message::create({}, Fast_Redirect::create(105, 0, msg->getHeader()->fast_redirect.ClientMemID));
//	msg->getHeader()->
//}
//
//void User::_login(std::shared_ptr<Recv_Message> msg)
//{
//
//}
//
//void User::_ping()
//{
//
//}


std::vector<std::shared_ptr<User>> UserHub::users;

ClientHandle::SendHandle::SendStruct ClientHandle::SendHandle::createSendStruct(const std::list<std::shared_ptr<Send_Message>>::iterator iter)
{
	SendStruct ss;
	ss.fastAccess = iter;
	ss.validIterator = true;
	return ss;
}

ClientHandle::RecvHandle::RecvStruct ClientHandle::RecvHandle::createRecvStruct(std::list<std::shared_ptr<Recv_Message>>::iterator fastAccess, bool validIterator)
{
	RecvStruct RS;
	RS.fastAccess = fastAccess;
	RS.timepoint = std::chrono::steady_clock::now();
	RS.ValidIterator = validIterator;
	return RS;
}

ClientHandle::ClientHandle(const asio::ip::udp::endpoint& endpoint)
	:endpoint(endpoint)
{
	sptr_this = ExtentPtr<ClientHandle>::create(this);
	userInterface = std::make_shared<User>(sptr_this);
	bind();
	if (valid)
	{
		//TaskExecutor::addTask(std::bind(RedirectHub::getFunc(this_ID, 21), sptr_this, nullptr)); //adding keep alive
	}

}

void ClientHandle::addSendMessage(std::shared_ptr<Send_Message> smsg)
{
	std::shared_lock sl(generalMutex);
	if (!valid)
	{
		return;
		//throw custom exception
	}
	std::unique_lock ul(sendHandle.sendMutex);
	auto it = sendHandle.sendMessages.insert(sendHandle.sendMessages.end(), smsg); // for now just back
	sendHandle.sendMessageMap.emplace(smsg->P_NMH->ID, SendHandle::createSendStruct(it));
}

void ClientHandle::addRecvMessage(std::shared_ptr<Recv_Message> msg)
{
	if (msg == nullptr)
	{
		return;
	}
	std::shared_lock sl(generalMutex);
	if (!valid)
	{
		return;
		//throw custom exception
	}

	{
		std::unique_lock ul1(confirmHandle.cofirmMutex);
		confirmHandle.confirmMessages.push_back(std::make_shared<Confirm_Message>(msg));
	}

	std::unique_lock ul(recvHandle.recvMutex);

	if (!recvHandle.recvMessageMap.contains(msg->getHeader()->ID))
	{
		if (msg->getHeader()->gameInstanceID != 0 && msg->getHeader()->fast_redirect.funcID != 0)
		{
			//error handling missing
			std::cout << "Undefined behaviour\n";
		}
		else if(msg->getHeader()->gameInstanceID != 0)
		{
			auto temp = gMsgHandle.getHandle(msg->getHeader()->gameInstanceID);
			if (temp)
			{
				temp->push(msg);
			}
			else
			{
				std::cout << "Potential Error: Unregistered MsgQueue\n";
			}
		}
		else if (msg->getHeader()->fast_redirect.funcID != 0)
		{
			RedirectFunction rFunc = RedirectHub::getFunc(this_ID, msg->getHeader()->fast_redirect.funcID);

			if (rFunc != nullptr)
			{
				RecvHandle::RecvStruct rs;
				rs.ValidIterator = false;
				rs.timepoint = std::chrono::steady_clock::now();
				recvHandle.recvMessageMap.emplace(msg->getHeader()->ID, rs);
				TaskExecutor::addTask(std::bind(rFunc, sptr_this, std::move(msg)));
			}
			else
			{
				std::cout << "Potential Error: Unregisterd Redirect-Function\n";
				auto it = recvHandle.recvMessages.insert(recvHandle.recvMessages.end(), msg);
				recvHandle.recvMessageMap.emplace(msg->getHeader()->ID, RecvHandle::createRecvStruct(it, true));
			}
		}
		else
		{
			auto it = recvHandle.recvMessages.insert(recvHandle.recvMessages.end(), msg);
			recvHandle.recvMessageMap.emplace(msg->getHeader()->ID, RecvHandle::createRecvStruct(it, true));
		}
	}
}

std::shared_ptr<Send_Message> ClientHandle::poll()
{
	std::shared_lock sl(generalMutex);
	if (!valid)
	{
		return nullptr;
		//throw custom exception
	}
	std::unique_lock ul(sendHandle.sendMutex);
	if (sendHandle.sendMessages.size() == 0)
	{
		return nullptr;
	}
	auto it = sendHandle.sendMessages.begin();
	std::shared_ptr<Send_Message> msg = *it;
	if (!it->get()->olderThan(10ms))
	{
		return nullptr;
	}
	if (it->get()->check_valid(1))
	{
		std::list<std::shared_ptr <Send_Message>> temp_list;
		temp_list.splice(temp_list.begin(), sendHandle.sendMessages, it);
		sendHandle.sendMessages.splice(sendHandle.sendMessages.end(), temp_list, temp_list.begin());
	}
	else
	{
		sendHandle.sendMessageMap.find((*it)->P_NMH->ID)->second.validIterator = false;
		sendHandle.sendMessages.erase(it);
	}
	return msg;
}

std::shared_ptr<Confirm_Message> ClientHandle::c_poll()
{
	std::shared_lock sl(generalMutex);
	if (!valid)
	{
		return nullptr;
		//throw custom exception
	}
	std::unique_lock ul(confirmHandle.cofirmMutex);
	if (confirmHandle.confirmMessages.size() == 0)
	{
		return nullptr;
	}
	auto t = confirmHandle.confirmMessages.front();
	confirmHandle.confirmMessages.pop_front();
	return t;
}

void ClientHandle::confirmMessage(std::shared_ptr<Confirm_Message> msg)
{
	std::shared_lock sl(generalMutex);
	if (!valid)
	{
		return;
		//throw custom exception
	}
	std::unique_lock ul(sendHandle.sendMutex);
	auto it = sendHandle.sendMessageMap.find(msg->getCM()->confirmID);
	if (it != sendHandle.sendMessageMap.end())
	{
		if (it->second.validIterator)
		{
			sendHandle.sendMessages.erase(it->second.fastAccess);
		}
		sendHandle.sendMessageMap.erase(it);
	}
}

size_t ClientHandle::getSendMessageCount() const noexcept
{
	return sendHandle.sendMessages.size();
}

size_t ClientHandle::getRecieveMessageCount() const noexcept
{
	return recvHandle.recvMessages.size();
}

size_t ClientHandle::getConfimationMsgCount() const noexcept
{
	return confirmHandle.confirmMessages.size();
}

const asio::ip::udp::endpoint& ClientHandle::getEndpoint() const noexcept
{
	return endpoint;
}

void ClientHandle::cleanUp()
{
	if (valid)
	{
		std::unique_lock ul(generalMutex);
		valid = false;
		sendHandle.sendMessageMap.clear();
		sendHandle.sendMessages.clear();

		recvHandle.recvMessageMap.clear();
		recvHandle.recvMessages.clear();

		confirmHandle.confirmMessages.clear();
	}
}

void ClientHandle::tempCleanUp()
{
	std::shared_lock ul(recvHandle.recvMutex);
	for (auto it = recvHandle.recvMessageMap.begin(); it != recvHandle.recvMessageMap.end(); )
	{
		if ((std::chrono::steady_clock::now() - it->second.timepoint) > 5s)
		{
			ul.unlock();
			recvHandle.recvMutex.lock();
			if (it->second.ValidIterator)
			{
				recvHandle.recvMessages.erase(it->second.fastAccess);
			}
			it = recvHandle.recvMessageMap.erase(it);
			recvHandle.recvMutex.unlock();
			ul.lock();
		}
		else {
			++it;
		}
	}
}

ClientHandle::~ClientHandle()
{
	sptr_this->valid = false;
	sptr_this->value = nullptr;
	cleanUp();
}

uint16_t ClientHandle::clientIDCount = 0;

void ClientHandle::bind()
{
	//Send Ping
	RedirectHub::registerRedirect(this_ID, 25,
		[](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg) {
			if (clientHandle->valid)
			{
				auto mem = RedirectHub::allocRedirectMemory<PingMemory>(10s);
				mem.second->status = P_STILL_RUNNING;
				mem.second->start_time = std::chrono::steady_clock::now();
				clientHandle->value->addSendMessage(Send_Message::create({}, Fast_Redirect::create(28, mem.first, 0)));
				return mem.first;
			}
	return (uint64_t)0;
		});

	//recv send hello
	RedirectHub::registerRedirect(this_ID, 6,
		[](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg) {
			if (clientHandle->valid)
			{
				clientHandle->value->addSendMessage(Send_Message::create({}, Fast_Redirect::create(7, 0,
					msg->getHeader()->fast_redirect.ClientMemID),High_Priotity,Ensured_Importance));
				return (uint64_t)0;
			}
			else
			{
				return (uint64_t)1;
			}
		});

	RedirectHub::registerRedirect(this_ID, 9,
		[](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg) {
			if (clientHandle->valid)
			{
				clientHandle->value->disconnect();
			}
			return (uint64_t)0;
		});

	//recieve sended ping
	RedirectHub::registerRedirect(this_ID, 29,
		[](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg) {
			std::shared_ptr<PingMemory> memory = RedirectHub::getMemory<PingMemory>(msg->getHeader()->fast_redirect.ServerMemID);
	memory->elapsed = std::chrono::steady_clock::now() - memory->start_time;
	memory->bytes_transfered = msg->getData().size() + sizeof(NormalMessageHeader);
	memory->status = P_SUCCES;
	return (uint64_t)0;
		});

	//recieve Ping
	RedirectHub::registerRedirect(this_ID, 27, std::function([](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg)
		{
			if (clientHandle->valid)
			{
				clientHandle->value->addSendMessage(Send_Message::create(msg->getData(), Fast_Redirect::create(30, 0, msg->getHeader()->fast_redirect.ClientMemID)));
			}
			else
			{
				std::cout << "27 ClientHandle invalid Error handling missing\n";
			}
	return (uint64_t)0;
		}));

	//recv + send keep alive
	RedirectHub::registerRedirect(this_ID, 21,
		[](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg)
		{
			TaskExecutor::addAsyncTask(std::bind(std::function([clientHandle]()
			{
				std::cout << "keep alive\n";
				if (clientHandle->valid)
				{
					clientHandle->value->addSendMessage(Send_Message::create({}, Fast_Redirect::create(3, 0, 0), High_Priotity, High_Importance));
					TaskExecutor::addTask(std::bind(RedirectHub::getFunc(clientHandle->value->this_ID, 21), clientHandle, nullptr));
				}
				return (uint64_t)0;
			})), 2s);
			return (uint64_t)0;
		});

	//rec + send login
	RedirectHub::registerRedirect(this_ID, 101,
		std::function([](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg) {
		if (clientHandle->valid)
		{
			clientHandle->value->userInterface->_login(clientHandle, msg);
		}
		else
		{	
			std::cout << "101 Erro Handling still Missing\n";
		}
		return (uint64_t)0;
		}));

	RedirectHub::registerRedirect(this_ID,150,
		std::function([](std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg) {
			if (clientHandle->valid)
			{
				clientHandle->value->userInterface->_login(clientHandle, msg);
			}
			else
			{
				std::cout << "150 Erro Handling still Missing\n";
			}
	return (uint64_t)0;
			}));
}

void ClientHandle::disconnect()
{
	sptr_this->valid = false;
	valid = false;
	disconnected = true;
	RedirectHub::removerRedirectFuncs(this_ID);
}

void User::_login(std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg)
{
	if (clientHandle->valid)
	{
		Custom_Message_View cmv(msg->getData());
		std::string username = cmv.read_str();
		std::string password = cmv.read_str();

		//Interaction with Userhub Not implemented yet
		uint32_t retCode = LR_SUCCESS;
		if (retCode == LR_SUCCESS)
		{
			clientHandle->value->userInterface->loged_in = true;
			clientHandle->value->userInterface->uname = username;
		}
		Custom_Message cm;
		cm.push_back_ui32(retCode);
		cm.push_back_str(username);
		//cm.push_back(session_token);

		clientHandle->value->addSendMessage(Send_Message::create(*cm.getRaw(), Fast_Redirect::create(102, 0, msg->getHeader()->fast_redirect.ClientMemID)));
	}
}

void User::_register(std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg)
{
	if (clientHandle->valid)
	{
		Custom_Message_View cmv(msg->getData());
		std::string username = cmv.read_str();
		std::string password = cmv.read_str();

		//Interaction with Userhub Not implemented yet
		uint32_t retCode = LR_SUCCESS;

		if (retCode == LR_SUCCESS)
		{
			clientHandle->value->userInterface->loged_in = true;
			clientHandle->value->userInterface->uname = username;
		}

		Custom_Message cm;
		cm.push_back_ui32(retCode);
		cm.push_back_str(username);
		//cm.push_back(session_token);

		clientHandle->value->addSendMessage(Send_Message::create(*cm.getRaw(), Fast_Redirect::create(105, 0, msg->getHeader()->fast_redirect.ClientMemID)));
	}
}

void User::_disconnect(std::shared_ptr<ExtentPtr<ClientHandle>> clientHandle, std::shared_ptr<Recv_Message> msg)
{
	//remove from OnlineUSers
}

uint64_t User::ping(std::chrono::milliseconds* ms)
{
	return 0;
}

void User::_fetch_info(std::shared_ptr<ExtentPtr<ClientHandle>>, std::shared_ptr<Recv_Message>)
{
	return;
}