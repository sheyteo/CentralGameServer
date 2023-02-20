#include "QuizGame.h"

std::mt19937 QuizGameQuestion::rng = std::default_random_engine{};

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

void QuizGameInstance::_start()
{
	loadQuestions(""); //TODO: insert path for questions maybe later Database
}

void QuizGameInstance::loadQuestions(const std::string& path)
{
	QuizGameQuestions.push_back(std::move(QuizGameQuestion::create("What does Matteo hate ?", { std::make_pair("Tomatos",true),std::make_pair("Ramen",false) ,std::make_pair("Muesli",false),std::make_pair("Apples",false) })));
	std::cout << "Warning QuizGameInstance::loadQuestions still not implemented\n";
}
