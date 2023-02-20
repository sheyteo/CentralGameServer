// CentralLinuxServer.cpp : Defines the entry point for the application.
//
//#include "pch.h"
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
#include "Server.h"

struct QuestionStruct
{
	std::string Topic;
	std::string question;
	std::string Right_Awnser;
	std::array<std::string, 3> wrongAwnsers;
};

class QuizGame1 : public GameInstanceBase
{
	std::list<QuestionStruct> questions;
private:
	void _start()
	{
		std::cout << "Hello we are starting Quizgame\n";

		QuestionStruct qs;
		qs.question = " Most popular programming language ?";
		qs.Topic = "Tech";
		qs.Right_Awnser = "Java Script";
		qs.wrongAwnsers = { "Java","C#","PHP" };

		questions.push_back(std::move(qs));

		while (true)
		{

			Custom_Message cm;
			cm.push_back_str(questions.front().question);
			cm.push_back_str(questions.front().wrongAwnsers[0]);
			cm.push_back_str(questions.front().Right_Awnser);
			cm.push_back_str(questions.front().wrongAwnsers[1]);
			cm.push_back_str(questions.front().wrongAwnsers[2]);


			std::this_thread::sleep_for(2s);
			{
				std::shared_lock sl(clHandleMutex);

				for (auto& [id, cl] : clHandle)
				{
					if (auto t = cl.lock())
					{
						t->addSendMessage(Send_Message::create(*cm.getRaw(), Fast_Redirect{}, High_Priotity, Ensured_Importance, 17));
					}
				}
				std::cout << "Waiting for awnsers\n";
			}
			

			std::this_thread::sleep_for(10s);
			{
				std::shared_lock sl(clHandleMutex);
				for (auto& [id, cl] : clHandle)
				{
					if (auto t = cl.lock())
					{
						if (auto m = t->gMsgHandle.getHandle(17)->pop())
						{
							Custom_Message_View cmv(m->getData());
							std::string awnser = cmv.read_str();

							if (awnser == questions.front().Right_Awnser)
							{
								std::cout << t->this_ID << " awnsered correctly\n";
							}
							else
							{
								std::cout << t->this_ID << " awnsered wrong\n";
							}
						};
					}
				}
			}
		}
		
	}
};

int main()
{
	Server server;
	GameInstanceSection::addGameInstance(17, std::make_shared<QuizGame1>());
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

