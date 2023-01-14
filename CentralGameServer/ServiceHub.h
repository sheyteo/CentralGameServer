#pragma once
#include "pch.h"
#include "Messages.h"
#include "ClientHandle.h"
#include "Server.h"
#include "ServiceHub.h"
#include "RedirectFunctions.h"

class GameInformation
{

};

class User
{
	friend class ServiceHub;
public:
	bool newly_created = false;
	std::string username;
	std::string password;
	nlohmann::json::object_t data;
	User(std::string username, std::string password)
		:username(username), password(password), newly_created(true)
	{

	}
	User(nlohmann::json::object_t raw)
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

class ServiceHub
{
	enum ReturnCode
	{
		Succes = 0ui8,
		Username_already_in_use = 1ui8,
		Wrong_credentials = 2ui8,
		Credential_to_short = 3ui8
	};
private:
	
	static std::string fullpath;

	static std::shared_mutex mutex;

	static uint8_t MayorVersion;
	static uint8_t MinorVersion;
	static std::vector<std::shared_ptr<User>> userList;
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
			userList.emplace_back(new User(user));
		}
		//Available Games
	}

	static uint8_t user_register(const std::string& username, const std::string& password) noexcept
	{
		if (password.size() > 4 && username.size() > 4)
		{
			return Credential_to_short;
		}

		{
			std::shared_lock sl(mutex);
			for (auto iter = userList.cbegin(); iter != userList.cend(); ++iter)
			{
				if (iter->get()->username == username)
				{
					return Username_already_in_use;
				}
			}
		}
		
		std::unique_lock ul(mutex);
		userList.emplace_back(new User(username, password));
		return Succes;
	}

	static uint8_t user_login(const std::string& username, const std::string& password)  noexcept
	{
		std::shared_lock sl(mutex);
		auto invIt = std::find_if(userList.begin(), userList.end(), [&username, &password](const std::shared_ptr<User>& user) {
			return (user->username == username) && (user->password == password);
			});
		if (invIt == userList.end())
		{
			return Wrong_credentials;
		}
		else
		{
			return Succes;
		}
	}

	static nlohmann::json::object_t getDataFromUser(const std::string& username) noexcept
	{
		auto invIt = std::find_if(userList.begin(), userList.end(), [&username](const std::shared_ptr<User>& user) {
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
std::vector<std::shared_ptr<User>> ServiceHub::userList;
std::vector<GameInformation> ServiceHub::gameInfoList;
nlohmann::json ServiceHub::total;

std::shared_ptr<S_Message> createLogin(const std::string& username, const std::string& password)
{
	Custom_Message cMsg;
	cMsg.push_back(username);
	cMsg.push_back(password);

	std::shared_ptr<S_Message> smsg(new S_Message);
	smsg->setData(cMsg.getRaw(), 100, High_Importance, High_Priotity, ServiceHubInteractions);
	return smsg;
}

namespace RedirectFunctions
{
	void login(RedirectFunctionParamater) // login = 100
	{
	}
}