//#pragma once
//#include "pch.h"
//#include "Message.h"
//#include "ClientHandle.h"
//
//class GameInformation
//{
//
//};
//
//class User
//{
//	friend class ServiceHub;
//private:
//	static uint32_t internalUserIdCounter;
//
//	uint32_t userID = 0;
//	std::string username;
//	std::string password;
//
//	bool currently_online = false;
//	bool newly_created = false;
//	
//	nlohmann::json::object_t data;
//public:
//	User(std::string username, std::string password);
//	User(nlohmann::json::object_t raw);
//};
//
//
//enum LoginRegisterCodes
//{
//	SUCCES = 0,
//	USERNAME_ALREADY_IN_USE = 1,
//	WRONG_CREDENTIALS = 2,
//	INVALID_CREDENTIALS = 3,
//	_STILL_RUNNING = 4,
//	TIMED_OUT = 5,
//	ACCOUNT_ALREADY_IN_USE = 6,
//	DISCONNECT_USER_NOT_FOUND = 7,
//	DISCONNECT_USER_ALREADY_OFFLINE = 8,
//	DISCONNECT_SUCCES = 9
//};
//
//enum PingRegisterCodes
//{
//	P_SUCESS = 0,
//	P_TIMED_OUT = 1,
//	P_FAIL = 2,
//	P_STILL_RUNNING = 3
//};
//
//class ServiceHub
//{
//private:
//
//	static std::string fullpath;
//
//	static std::shared_mutex UserListMutex;
//
//	static uint8_t MayorVersion;
//	static uint8_t MinorVersion;
//	static std::vector<std::shared_ptr<User>> userList;
//	static std::vector<GameInformation> gameInfoList;
//
//	static nlohmann::json total;
//
//	static std::shared_mutex OnlineUserMutex;
//	static std::unordered_map<uint32_t, std::shared_ptr<User>> OnlineUsers;
//
//public:
//	static void load(const std::string& path);
//
//	static std::pair<std::uint32_t, std::chrono::milliseconds> ping(ClientHandle* cHandle);
//
//	static uint32_t user_register(const std::string& username, const std::string& password, ClientHandle* clientHandle = nullptr, const bool& autologin = true) noexcept;
//
//	static uint32_t user_login(const std::string& username, const std::string& password, ClientHandle* clientHandle)  noexcept;
//
//	static uint32_t user_disconnect(const std::string& username);
//
//	static nlohmann::json::object_t getDataFromUser(const std::string& username) noexcept;
//
//	static const std::vector<GameInformation>& getGameInfoList() noexcept;
//
//	static void save() noexcept;
//
//};