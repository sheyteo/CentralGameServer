#pragma once
#include "pch.h"
#include "GameInstance.h"

enum QuizGameActions
{
	Nothing = 0,
	Client_Recv_Question = 1,
	Server_Recv_Answer = 2,
	Client_Recv_Results = 3,
	Client_Update_Leaderboard = 4,
	Client_Update_Votes_casted = 5,
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

	QuestionTopic topic = None;

	std::array<std::pair<std::string,bool>, 4> answers;

	static QuizGameQuestion create(const std::string& question, const std::array<std::pair<std::string,bool>, 4>& awnsers, QuestionTopic topic = None);
	void addToCM(Custom_Message& customMessage);

	static std::mt19937 rng;
};

class QuizGameInstance : public GameInstanceBase
{
private:
	std::list<QuizGameQuestion> QuizGameQuestions;

	void _start() override;
	void loadQuestions(const std::string& path);
};