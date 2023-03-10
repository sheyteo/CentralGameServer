#include "QuizGame.h"

std::mt19937 QuizGameQuestion::rng = std::default_random_engine{};
uint32_t QuizGameQuestion::question_ID_counter = 0;

QuizGameQuestion QuizGameQuestion::create(const std::string& question, const std::array<std::pair<std::string, bool>,4>& answers, QuestionTopic topic)
{
	QuizGameQuestion qgq;
	qgq.question = question;
	qgq.answers = answers;

	uint8_t test = 0;
	for (const auto& [answer, correct] : answers)
	{
		test += correct;
	}
	if (test != 1)
	{
		throw std::runtime_error("Wrong Question Format");
	}

	std::shuffle(qgq.answers.begin(), qgq.answers.end(), rng);

	qgq.topic = topic;

	return qgq; //copy ellision no std::move needed
}

void QuizGameQuestion::addToCM(Custom_Message& customMessage)
{
	customMessage.push_back_ui32(Client_Recv_Question);
	customMessage.push_back_ui32(topic);
	customMessage.push_back_str(question);
	for (auto& [answer, correct] : answers)
	{
		customMessage.push_back_str(answer);
	}
}

QuizGameInstance::QuizGameInstance(const uint32_t& id)
{
	this_game_id = id;
	std::cout << "Should be used 123\n";
}

uint32_t QuizGameInstance::getPlayerCount()
{
	return this->clHandle.size();
}

void QuizGameInstance::_start()
{
	loadQuestions(""); //TODO: insert path for questions, maybe later Database

	setStartPlayers();

	while (true)
	{
		//poll new Question
		QuizGameQuestion q = this->QuizGameQuestions.front();
		QuizGameQuestions.pop_front();
		QuizGameQuestions.push_back(q);


		//send question to all player
		Custom_Message cm;
		q.addToCM(cm);
		auto sm = Send_Message::create(*cm.getRaw(), Fast_Redirect{}, High_Priotity, Ensured_Importance, this_game_id);
		send_to_all(sm);

		//wait time
		std::this_thread::sleep_for(10s);
		
		//recv all answers
		
		//check answers
		std::shared_lock sl(clHandleMutex);
		for (auto& [id, qgpi]: activePlayers)
		{
			uint8_t sgpi = qgpi->awnser_index;
			std::cout << "User " << id << "?s with name "<<qgpi->username<<" answer is " << sgpi << "\n";
		}
		
		std::this_thread::sleep_for(5s);

		//send scores + right answer
		//wait some time
	}
}

void QuizGameInstance::start_recieve_loop()
{
	rLoop_active = true;
	rLoopFuture = std::async(std::launch::async, &QuizGameInstance::_recieve_loop, this);
}

void QuizGameInstance::_recieve_loop()
{
	while (rLoop_active)
	{
		std::shared_lock sl(clHandleMutex);
		for (auto& [id, qgpi]: activePlayers)
		{
			std::shared_ptr<ClientHandle> clh;
			try
			{
				auto& c = clHandle.at(id);
				clh = c.lock();
			}
			catch (const std::exception&){}
			if (clh == nullptr)
			{
				std::cout << "0 Error in QuizgameInsatnce::_recieve_loop\n";
				continue;
			}
			std::shared_ptr<ThreadSafeQueue<Recv_Message>> ss = clh->getGMH()->getHandle(this_game_id);
			if (ss == nullptr)
			{
				std::cout << "1 Error in QuizgameInsatnce::_recieve_loop\n";
				continue;
			}
			auto msg = ss->pop();
			if (msg != nullptr)
			{
				std::async(std::launch::async, &QuizGameInstance::dispatch_msg, this, msg, id);
			}
		}
	}
}

void QuizGameInstance::stop_recieve_loop()
{
	rLoop_active = false;
}

void QuizGameInstance::wait_stop_recieve_loop()
{
	rLoop_active = false;
	rLoopFuture.wait();
}

void QuizGameInstance::dispatch_msg(std::shared_ptr<Recv_Message> msg, uint32_t id)
{
	if (this == nullptr)
	{
		return;
	}
	Custom_Message_View cmv(msg->getData());
	uint32_t instruction = cmv.read_uint32();
	switch (instruction)
	{
	case Server_Recv_Answer:
		ServerRecvAnswer(msg, id);
		break;
	case Server_Send_Leaderboard:
		break;
	case Server_Send_Votes_casted:
		break;

	default:
		std::cout << "Error at switch case in QuizGameInstance::dispatch_msg()\n";
		break;
	}
}

void QuizGameInstance::setStartPlayers()
{
	std::shared_lock sl(clHandleMutex);
	for (auto& [id, clh]: clHandle)
	{
		if (auto sclh = clh.lock())
		{
			activePlayers.insert_or_assign(id, QuizGamePlayerInfo::create(sclh));
		}
	}

	//Creating Start Msg
	Custom_Message cm;

	//header
	cm.push_back_ui32(Client_Recv_Start_Message);

	//game name
	cm.push_back_str(this->gInfo.GameName);

	//player number
	cm.push_back_ui32(activePlayers.size());

	//player names
	for (auto& [id, info]: activePlayers)
	{
		cm.push_back_str(info->username);		
	}

	auto msg = Send_Message::create(*cm.getRaw(), Fast_Redirect{}, High_Priotity, Ensured_Importance, this_game_id);

	//Sending Start Message to every participant
	for (auto& [id, _] : activePlayers)
	{
		try
		{
			if (auto sclh = clHandle.at(id).lock())
			{
				sclh->addSendMessage(msg);
			}
		}
		catch (const std::exception&)
		{
			std::cout << "Error in QuizGame.cpp::QuizGameInstance::setStartPlayers()\n";
		}
		
	}

	
}

void QuizGameInstance::loadQuestions(const std::string& path)
{
	QuizGameQuestions.push_back(std::move(QuizGameQuestion::create("What does Matteo hate ?", { std::make_pair("Tomatos",true),std::make_pair("Ramen",false) ,std::make_pair("Muesli",false),std::make_pair("Apples",false) })));
	std::cout << "Warning QuizGameInstance::loadQuestions still not implemented\n";
}

void QuizGameInstance::send_to_all(std::shared_ptr<Send_Message> msg)
{
	std::shared_lock sl(clHandleMutex);
	for (auto& [id, clh] : clHandle)
	{
		if (auto sclh = clh.lock())
		{
			sclh->addSendMessage(std::make_shared<Send_Message>(*msg));
		}
	}
}

void QuizGameInstance::ServerRecvAnswer(std::shared_ptr<Recv_Message> msg, const uint32_t& ID)
{
	Custom_Message_View cmv(msg->getData());
	cmv.read_uint32();

	try
	{
		activePlayers.at(ID)->last_answered_q = cmv.read_uint32(); //question ID
		activePlayers.at(ID)->awnser_index = cmv.read_uint8(); //chosen answer
	}
	catch (const std::exception&)
	{

	}
}

void QuizGameInstance::ServerSendLeaderboardRequest(std::shared_ptr<Recv_Message> msg, const uint32_t& ID)
{
}

void QuizGameInstance::ServerSendVotesCastedRequest(std::shared_ptr<Recv_Message> msg, const uint32_t& ID)
{
}

std::shared_ptr<QuizGamePlayerInfo> QuizGamePlayerInfo::create(std::shared_ptr<ClientHandle> sclh)
{
	std::shared_ptr<QuizGamePlayerInfo> qgpi(new QuizGamePlayerInfo{});
	qgpi->awnser_index = UINT8_MAX;
	qgpi->last_answered_q = UINT32_MAX;
	qgpi->score_points = 0;
	qgpi->username = sclh->userInterface->get_username();
	return qgpi;
}
