// CentralLinuxServer.cpp : Defines the entry point for the application.
//
#include "pch.h"
#include "Messages.h"
#include "ClientHandle.h"
#include "Server.h"
#include "Testing.h"
#include "ServiceHub.h"

void bindToServiceHub(ClientHandle* clientHandle)
{
	ClientHandle::RedirectHandler loginHandle;
	loginHandle.function = Login;
	clientHandle->registerRedirectFunction(100, loginHandle);

	ClientHandle::RedirectHandler registerHandle;
	registerHandle.function = Register;
	clientHandle->registerRedirectFunction(102, registerHandle);
}

int main()
{
	ServiceHub::load("");
	std::shared_ptr< Server> server(new Server);
	server->start(1);
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		ServiceHub::save();
	}
	/*Custom_Message cm;
	cm.push_back("abcdef");
	cm.push_back(1);
	cm.push_back("abcdefg");
	std::vector<char> temp = cm.getRaw();
	Custom_Message_View cv(temp);
	std::string result = cv.read_str();
	uint32_t result1 = cv.read_uint32();
	std::string result2 = cv.read_str();
	std::cout << result2 << "\n";*/
	/*ServiceHub service;
	service.load("UserData.json");
	auto b = service.user_register("abbas", "7891");
	auto a = service.user_login("abbas", "7891");
	service.save();*/
	/*std::shared_ptr< Server> server(new Server);
	server->start(1);
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		if (server->get_CHM().first.get().size())
		{
			auto a = server->get_CHM().first;
			if (a.get().begin()->second->getRecieveMessageCount())
			{
				auto ms = a.get().begin()->second->getRecievedMsg();
				std::cout << ms->getHeader().sendCount << "\n";
			}
		}
	}
	std::cout << "Now continouing\n";*/
}