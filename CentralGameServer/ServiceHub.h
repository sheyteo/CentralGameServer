#pragma once
#include "pch.h"
#include "Messages.h"
#include "ClientHandle.h"
#include "Server.h"
#include "ServiceHub.h"

class GameInformation
{

};

class UserObject
{
	friend class ServiceHub;
public:
	bool newly_created = false;
	std::string username;
	std::string password;
	nlohmann::json::object_t data;
	UserObject(std::string username, std::string password)
		:username(username), password(password), newly_created(true)
	{

	}
	UserObject(nlohmann::json::object_t raw)
	{
		if (raw.contains("username") && raw.contains("password"))
		{
			username = raw["username"];
			password = raw["password"];
		}
		if (raw.contains("data"))
		{
			data = raw["data"];
		}
	}
};

enum LoginRegisterCodes
{
	SUCCES = 0ui32,
	USERNAME_ALREADY_IN_USE = 1ui32,
	WRONG_CREDENTIALS = 2ui32,
	INVALID_CREDENTIALS = 3ui32,
	_STILL_RUNNING = 4ui32,
	TIMED_OUT = 5ui32
};

class ServiceHub
{
private:
	
	static std::string fullpath;

	static std::shared_mutex mutex;

	static uint8_t MayorVersion;
	static uint8_t MinorVersion;
	static std::vector<std::shared_ptr<UserObject>> userList;
	static std::vector<GameInformation> gameInfoList;

	static nlohmann::json total;

public:
	static void load(const std::string& path)
	{
		std::unique_lock sl(mutex);
		std::ifstream file;
		file.open(fullpath, std::ios::binary);
		if (!file.is_open())
		{
			std::cout << "Failed to open file\n";
			return;
		}
		total = nlohmann::json::parse(file);
		MayorVersion = total["version"]["mayor"];
		MinorVersion = total["version"]["minor"];
		nlohmann::json::array_t users = total["users"];
		for (auto& user : users)
		{
			userList.emplace_back(new UserObject(user));
		}
		//Available Games
	}

	static uint32_t user_register(const std::string& username, const std::string& password) noexcept
	{
		if (password.size() > 4 && username.size() > 4)
		{
			return INVALID_CREDENTIALS;
		}

		{
			std::shared_lock sl(mutex);
			for (auto iter = userList.cbegin(); iter != userList.cend(); ++iter)
			{
				if (iter->get()->username == username)
				{
					return USERNAME_ALREADY_IN_USE;
				}
			}
		}
		
		std::unique_lock ul(mutex);
		userList.emplace_back(new UserObject(username, password));
		return SUCCES;
	}

	static uint32_t user_login(const std::string& username, const std::string& password)  noexcept
	{
		std::shared_lock sl(mutex);
		auto invIt = std::find_if(userList.begin(), userList.end(), [&username, &password](const std::shared_ptr<UserObject>& user) {
			return (user->username == username) && (user->password == password);
			});
		if (invIt == userList.end())
		{
			return WRONG_CREDENTIALS;
		}
		else
		{
			return SUCCES;
		}
	}

	static nlohmann::json::object_t getDataFromUser(const std::string& username) noexcept
	{
		auto invIt = std::find_if(userList.begin(), userList.end(), [&username](const std::shared_ptr<UserObject>& user) {
			return (user->username == username);
			});
		if (invIt != userList.end())
		{
			return invIt->get()->data;
		}
		return {};
	}

	static const std::vector<GameInformation>& getGameInfoList() noexcept
	{
		return gameInfoList;
	}

	static void save() noexcept
	{
		std::unique_lock ul(mutex);
		nlohmann::json::array_t arr = total["users"];
		for (auto iterator = userList.begin(); iterator != userList.end(); ++iterator)
		{
			if ((*iterator)->newly_created) {
				nlohmann::json::object_t user;
				user["username"] = (*iterator)->username;
				user["password"] = (*iterator)->password;
				user["data"] = (*iterator)->data;
				
				arr.push_back(user);
				(*iterator)->newly_created = false;
			}
		}
		total["users"] = arr;

		std::string raw = total.dump(4, ' ', true);
		std::ofstream file(fullpath);
		file.write(raw.data(), raw.size());
		file.close();
	}
};

std::string ServiceHub::fullpath = "C:/Users/Matteo/source/repos/CentralGameServer/UserData.json";
uint8_t ServiceHub::MayorVersion = 0;
uint8_t ServiceHub::MinorVersion = 0;
std::shared_mutex ServiceHub::mutex;
std::vector<std::shared_ptr<UserObject>> ServiceHub::userList;
std::vector<GameInformation> ServiceHub::gameInfoList;
nlohmann::json ServiceHub::total;


void Login(ClientHandle* clientHandle, std::shared_ptr<R_Message> message)
{
	Custom_Message_View custom_msg(message->getData());
	const std::string& username = custom_msg.read_str();
	const std::string& password = custom_msg.read_str();

	uint32_t responseCode = ServiceHub::user_login(username, password);

	Custom_Message cmsg;
	cmsg.push_back(responseCode);
	cmsg.push_back(username);
	//Maybe add session keys for security

	std::shared_ptr<S_Message> loginResponse(new S_Message);
	loginResponse->setData(cmsg.getRaw(), 101, High_Importance, High_Priotity);

	clientHandle->addNewMessage(loginResponse);
}

void Register(ClientHandle* clientHandle, std::shared_ptr<R_Message> message)
{
	Custom_Message_View custom_msg(message->getData());
	const std::string& username = custom_msg.read_str();
	const std::string& password = custom_msg.read_str();

	uint32_t responseCode = ServiceHub::user_register(username, password);

	Custom_Message cmsg;
	cmsg.push_back(responseCode);
	cmsg.push_back(username);
	//Maybe add session keys for security

	std::shared_ptr<S_Message> registerResponse(new S_Message);
	registerResponse->setData(cmsg.getRaw(), 103, High_Importance, High_Priotity);

	clientHandle->addNewMessage(registerResponse);
}