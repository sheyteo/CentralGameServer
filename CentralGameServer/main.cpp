// CentralLinuxServer.cpp : Defines the entry point for the application.
//
//#include "pch.h"
//#include "Messages.h"
//#include "ClientHandle.h"
//#include "Server.h"
//#include "Testing.h"
//#include "ServiceHub.h"
//
//
//int main()
//{
//	ServiceHub::load("");
//	std::shared_ptr< Server> server(new Server);
//	server->start(1);
//	std::this_thread::sleep_for(std::chrono::seconds(100000));
//	std::cout << "Shuting down in 5s\n";
//	std::this_thread::sleep_for(std::chrono::seconds(5));
//	std::cout << "Shuting down now \n";
//	server->shutdown();
//	std::cout << "Sucessfull shut down \n";
//
//	/*Custom_Message cm;
//	cm.push_back("abcdef");
//	cm.push_back(1);
//	cm.push_back("abcdefg");
//	std::vector<char> temp = cm.getRaw();
//	Custom_Message_View cv(temp);
//	std::string result = cv.read_str();
//	uint32_t result1 = cv.read_uint32();
//	std::string result2 = cv.read_str();
//	std::cout << result2 << "\n";*/
//	/*ServiceHub service;
//	service.load("UserData.json");
//	auto b = service.user_register("abbas", "7891");
//	auto a = service.user_login("abbas", "7891");
//	service.save();*/
//	/*std::shared_ptr< Server> server(new Server);
//	server->start(1);
//	while (true)
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(20));
//		if (server->get_CHM().first.get().size())
//		{
//			auto a = server->get_CHM().first;
//			if (a.get().begin()->second->getRecieveMessageCount())
//			{
//				auto ms = a.get().begin()->second->getRecievedMsg();
//				std::cout << ms->getHeader().sendCount << "\n";
//			}
//		}
//	}
//	std::cout << "Now continouing\n";*/
//}

#include "pch.h"
#include "NewServer.h"

class QuizGame : public GameInstanceBase
{
private:
	void _start()
	{
		std::cout << "Hello we are starting Quizgame\n";
		while (true)
		{
			std::this_thread::sleep_for(2s);
			std::shared_lock sl(clHandleMutex);
			for (auto& [id,cl] : clHandle)
			{
				if (auto t = cl.lock())
				{
					t->addSendMessage(Send_Message::create({'H','E','L','L','O'}, Fast_Redirect{}, High_Priotity, Ensured_Importance, 17));
					if (auto m = t->gMsgHandle.getHandle(17)->pop())
					{
						for (auto& c : m->getData())
						{
							std::cout << c;
						}
					};
				}
				//this->clHandle.at(0).lock()->gMsgHandle.getHandle(17)->pop(); recv
				//this->clHandle.at(0).lock()->addSendMessage(smsg); send
			}
		}
		
	}
};

int main()
{
	Server server;
	GameInstanceSection::addGameInstance(17, std::make_shared<QuizGame>());
	server.start(5,5);
	std::this_thread::sleep_for(5s);
	if (auto gi = GameInstanceSection::getGameInstance(17))
	{
		gi->start();
		std::cout << "Started GameInstance\n";
	}
	else
	{
		std::cout << "Couldnt start GameInstance\n";
	}
	
	while (true)
	{
		std::this_thread::sleep_for(100ms);
	}
}

