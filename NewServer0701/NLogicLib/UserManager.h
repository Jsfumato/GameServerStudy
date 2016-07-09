#pragma once
#include <tuple>
#include <unordered_map>
#include <deque>
#include "../Common/ErrorCode.h"
#include "User.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	class UserManager
	{
	public:
		UserManager() = default;
		~UserManager() = default;

		void Init(const int maxUserCount);

		ERROR_CODE AddUser(const int sessionIndex, const char* userID);
		ERROR_CODE RemoveUser(const int sessionIndex);

		User* FindUser(const int sessionIndex);
		User* FindUser(const char* userID);

		std::tuple<ERROR_CODE, User*> GetUser(const int sessionIndex);

	private:
		User* AllocNewUserIndex();
		
		std::unordered_map<int, User*> m_UserSessionDic;
		std::unordered_map<const char*, User*> m_UserIDDic;
		
		std::vector<User> m_UserPool;
		std::deque<int> m_UserPoolIndex;
	};
}