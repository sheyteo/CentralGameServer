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
	customMessage.push_back_str("--QUESTION--");
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

void QuizGameInstance::_start()
{
	loadQuestions(""); //TODO: insert path for questions, maybe later Database

	setStartPlayers();

	while (true)
	{
		std::this_thread::sleep_for(20ms);
	}
}

void QuizGameInstance::start_recieve_loop()
{
	rLoop_active = true;
	rLoopFuture = std::async(std::launch::async, &QuizGameInstance::_recieve_loop, this);
}

void QuizGameInstance::_recieve_loop()
{
	while (true)
	{
		clHandleMutex.lock_shared();
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
	Custom_Message_View cmv(msg->getData());
	uint32_t instruction = cmv.read_uint32();
	switch (instruction)
	{
	case Server_Recv_Answer:
		ServerRecvAwnser(msg, id);
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
}

void QuizGameInstance::loadQuestions(const std::string& path)
{
	QuizGameQuestions.push_back(std::move(QuizGameQuestion::create("What does Matteo hate ?", { std::make_pair("Tomatos",true),std::make_pair("Ramen",false) ,std::make_pair("Muesli",false),std::make_pair("Apples",false) })));
	std::cout << "Warning QuizGameInstance::loadQuestions still not implemented\n";
}

void QuizGameInstance::ServerRecvAwnser(std::shared_ptr<Recv_Message> msg, const uint32_t& ID)
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
