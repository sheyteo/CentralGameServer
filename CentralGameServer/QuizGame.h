#pragma once
#include "pch.h"
#include "GameInstance.h"

enum QuizGameInstructions
{
	Nothing = 0,
	Client_Recv_Question = 1,
	Server_Recv_Answer = 2,
	Client_Recv_Results = 3,
	Client_Update_Leaderboard = 4,
	Client_Update_Votes_casted = 5,
	Server_Send_Leaderboard = 6,
	Server_Send_Votes_casted = 7,
};

enum QuestionTopic
{
	Sport = 0,
	Science = 1,
	Celebreties = 2,
	Geography = 3,
	Finance = 4,
	Economy = 5,
	Video_Games = 6,
	None = 10
};

struct QuizGameQuestion
{
	std::string question;
	uint32_t ID = question_ID_counter++;

	QuestionTopic topic = None;

	std::array<std::pair<std::string,bool>, 4> answers;

	static QuizGameQuestion create(const std::string& question, const std::array<std::pair<std::string,bool>, 4>& awnsers, QuestionTopic topic = None);
	void addToCM(Custom_Message& customMessage);

	static std::mt19937 rng;
	static uint32_t question_ID_counter;
};

struct QuizGamePlayerInfo
{
	std::string username;
	
	uint32_t last_answered_q;
	uint8_t awnser_index;

	uint32_t score_points;

	static std::shared_ptr<QuizGamePlayerInfo> create(std::shared_ptr<ClientHandle> sclh);
};

class QuizGameInstance : public GameInstanceBase
{
public:
	QuizGameInstance(const uint32_t& id);
private:
	std::list<QuizGameQuestion> QuizGameQuestions;
	std::unordered_map<uint32_t, std::shared_ptr<QuizGamePlayerInfo>> activePlayers;
	bool rLoop_active = false;
	std::future<void> rLoopFuture;

	void _start() override;

	void start_recieve_loop();
	void _recieve_loop();
	void stop_recieve_loop();
	void wait_stop_recieve_loop();

	void dispatch_msg(std::shared_ptr<Recv_Message> msg, uint32_t id);

	void setStartPlayers();
	void loadQuestions(const std::string& path);

	void ServerRecvAwnser(std::shared_ptr<Recv_Message>, const uint32_t& ID);
	void ServerSendLeaderboardRequest(std::shared_ptr<Recv_Message>, const uint32_t& ID);
	void ServerSendVotesCastedRequest(std::shared_ptr<Recv_Message>, const uint32_t& ID);
};