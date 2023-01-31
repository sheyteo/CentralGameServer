//#include "ServiceHub.h"
//
//User::User(std::string username, std::string password)
//	:username(username), password(password), newly_created(true), userID(internalUserIdCounter++)
//{
//
//}
//
//User::User(nlohmann::json::object_t raw)
//	: userID(internalUserIdCounter++)
//{
//	if (raw.contains("username") && raw.contains("password"))
//	{
//		username = raw["username"];
//		password = raw["password"];
//	}
//	if (raw.contains("data"))
//	{
//	data = raw["data"];
//	}
//}
//void ServiceHub::load(const std::string& path)
//{
//	std::unique_lock sl(UserListMutex);
//	std::ifstream file;
//	file.open(fullpath, std::ios::binary);
//	if (!file.is_open())
//	{
//		std::cout << "Failed to open file\n";
//		return;
//	}
//	total = nlohmann::json::parse(file);
//	MayorVersion = total["version"]["mayor"];
//	MinorVersion = total["version"]["minor"];
//	nlohmann::json::array_t users = total["users"];
//	for (auto& user : users)
//	{
//		userList.emplace_back(new User(user));
//	}
//	//Available Games
//}
//
//std::pair<std::uint32_t, std::chrono::milliseconds> ServiceHub::ping(ClientHandle* cHandle)
//{
//	const uint32_t tries = 5;
//	std::array<std::pair<uint32_t,std::chrono::milliseconds>, tries> erg{};
//	for (int i = 0; i < tries; i++)
//	{
//		auto basePtr = cHandle->_getRedirectMemory(30);
//		*basePtr = std::shared_ptr<shared_ping_memory>(new shared_ping_memory());
//
//		*(basePtr.get()) = std::shared_ptr<shared_ping_memory>(new shared_ping_memory);
//
//		std::shared_ptr<shared_ping_memory> sh_ptr = std::dynamic_pointer_cast<shared_ping_memory>(*basePtr.get());
//
//		sh_ptr->created = std::chrono::steady_clock::now();
//		sh_ptr->status = PingRegisterCodes::P_STILL_RUNNING;
//
//		cHandle->addNewMessage(constructSMessage(std::vector<char>(100, 0), 27));
//
//		auto timer = std::chrono::steady_clock::now();
//
//		while (sh_ptr->status == PingRegisterCodes::P_STILL_RUNNING && std::chrono::steady_clock::now() - timer <= std::chrono::seconds(5))
//		{
//			std::this_thread::sleep_for(std::chrono::milliseconds(5));
//		}
//
//		uint32_t returnCode = sh_ptr->status;
//
//		if (returnCode == PingRegisterCodes::P_STILL_RUNNING)
//		{
//			returnCode = PingRegisterCodes::P_TIMED_OUT;
//		}
//		erg.at(i) = std::make_pair(returnCode, std::chrono::duration_cast<std::chrono::milliseconds>(sh_ptr->elapsed));
//
//		//Reconstructing Memory
//		*basePtr = std::shared_ptr<shared_ping_memory>(new shared_ping_memory());
//	}
//
//	std::chrono::milliseconds ms(0);
//
//	for (auto& [code, time]: erg)
//	{
//		if (code != P_SUCESS)
//		{
//			return std::make_pair(P_FAIL, std::chrono::milliseconds(0));
//		}
//		else
//		{
//			ms += time;
//		}
//	}
//	return {P_SUCESS, ms / tries};
//}
//
//
//uint32_t ServiceHub::user_register(const std::string& username, const std::string& password, ClientHandle* clientHandle, const bool& autologin) noexcept
//{
//	if (password.size() > 4 && username.size() > 4)
//	{
//		return LoginRegisterCodes::INVALID_CREDENTIALS;
//	}
//
//	{
//		std::shared_lock sl(UserListMutex);
//		for (auto iter = userList.cbegin(); iter != userList.cend(); ++iter)
//		{
//			if (iter->get()->username == username)
//			{
//				return LoginRegisterCodes::USERNAME_ALREADY_IN_USE;
//			}
//		}
//	}
//
//	std::unique_lock ul(UserListMutex);
//	auto& newUser = userList.emplace_back(new User(username, password));
//	newUser->currently_online = autologin;
//	if (autologin)
//	{
//		bindDisconnect(clientHandle, username);
//		std::unique_lock ul_O(OnlineUserMutex);
//		OnlineUsers.insert(std::make_pair(newUser->userID, newUser));
//	}
//	return LoginRegisterCodes::SUCCES;
//}
//
//uint32_t ServiceHub::user_login(const std::string& username, const std::string& password, ClientHandle* clientHandle)  noexcept
//{
//	std::shared_lock sl(UserListMutex);
//	auto invIt = std::find_if(userList.begin(), userList.end(), [&username, &password](const std::shared_ptr<User>& user) {
//		return (user->username == username) && (user->password == password);
//		});
//	if (invIt == userList.end())
//	{
//		return LoginRegisterCodes::WRONG_CREDENTIALS;
//	}
//	else
//	{
//		if (!(*invIt)->currently_online)
//		{
//			bindDisconnect(clientHandle, username);
//			sl.unlock();
//			{
//				std::unique_lock ul(OnlineUserMutex);
//				(*invIt)->currently_online = true;
//				OnlineUsers.insert(std::make_pair((*invIt)->userID, *invIt));
//			}
//			sl.lock();
//			return LoginRegisterCodes::SUCCES;
//		}
//		else
//		{
//			return LoginRegisterCodes::ACCOUNT_ALREADY_IN_USE;
//		}
//	}
//}
//
//uint32_t ServiceHub::user_disconnect(const std::string& username)
//{
//	std::shared_lock sl(UserListMutex);
//	auto findIt = std::find_if(userList.begin(), userList.end(), [&username](const std::shared_ptr<User>& user) {
//		return (user->username == username);
//		});
//	if (findIt == userList.end())
//	{
//		return LoginRegisterCodes::DISCONNECT_USER_NOT_FOUND;
//	}
//	else
//	{
//		if (!(*findIt)->currently_online)
//		{
//			return LoginRegisterCodes::DISCONNECT_USER_ALREADY_OFFLINE;
//		}
//		else
//		{
//			std::unique_lock ul_O(OnlineUserMutex);
//			OnlineUsers.erase((*findIt)->userID);
//			(*findIt)->currently_online = false;
//			return LoginRegisterCodes::DISCONNECT_SUCCES;
//		}
//	}
//}
//
//nlohmann::json::object_t ServiceHub::getDataFromUser(const std::string& username) noexcept
//{
//	auto invIt = std::find_if(userList.begin(), userList.end(), [&username](const std::shared_ptr<User>& user) {
//		return (user->username == username);
//		});
//	if (invIt != userList.end())
//	{
//		return invIt->get()->data;
//	}
//	return {};
//}
//
//const std::vector<GameInformation>& ServiceHub::getGameInfoList() noexcept
//{
//	return gameInfoList;
//}
//
//void ServiceHub::save() noexcept
//{
//	std::unique_lock ul(UserListMutex);
//	nlohmann::json::array_t arr = total["users"];
//	for (auto iterator = userList.begin(); iterator != userList.end(); ++iterator)
//	{
//		if ((*iterator)->newly_created) {
//			nlohmann::json::object_t user;
//			user["username"] = (*iterator)->username;
//			user["password"] = (*iterator)->password;
//			user["data"] = (*iterator)->data;
//
//			arr.push_back(user);
//			(*iterator)->newly_created = false;
//		}
//	}
//	total["users"] = arr;
//
//	std::string raw = total.dump(4, ' ', true);
//	std::ofstream file(fullpath);
//	file.write(raw.data(), raw.size());
//	file.close();
//}
//
//uint32_t User::internalUserIdCounter = 0;
//
//void bindDisconnect(ClientHandle* clientHandle, const std::string& username)
//{
//	ClientHandle::RedirectHandler rh;
//
//	rh.function = [username](ClientHandle*, std::shared_ptr<R_Message>)
//	{
//		std::cout << "Disconnected from Server :" << username << " with responseCode :" << ServiceHub::user_disconnect(username) << "\n";
//	};
//
//	clientHandle->registerRedirectFunction(51, rh);
//
//	rh.function = [username](ClientHandle* ch, std::shared_ptr<R_Message>)
//	{
//		Custom_Message cm;
//		uint32_t resp_code = ServiceHub::user_disconnect(username);
//		cm.push_back(resp_code);
//		cm.push_back(username);
//
//		ch->addNewMessage(constructSMessage(cm.getRaw(), 52, High_Importance, High_Priotity));
//		std::cout << "Disconnected " << username << " with responseCode :" << resp_code << "\n";
//	};
//
//	clientHandle->registerRedirectFunction(50, rh);
//}
//
//std::string ServiceHub::fullpath = "C:/Users/Matteo/source/repos/CentralGameServer/UserData.json";
//uint8_t ServiceHub::MayorVersion = 0;
//uint8_t ServiceHub::MinorVersion = 0;
//std::shared_mutex ServiceHub::UserListMutex;
//std::vector<std::shared_ptr<User>> ServiceHub::userList;
//std::vector<GameInformation> ServiceHub::gameInfoList;
//nlohmann::json ServiceHub::total;
//std::shared_mutex ServiceHub::OnlineUserMutex;
//std::unordered_map<uint32_t, std::shared_ptr<User>> ServiceHub::OnlineUsers;
