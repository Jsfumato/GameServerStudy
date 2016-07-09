#include "UserManager.h"

namespace NLogicLib
{
	void UserManager::Init(const int maxUserCount)
	{
		for (int i = 0; i < maxUserCount; ++i)
		{
			User user;
			user.Init(i);

			m_UserPool.push_back(user);
			m_UserPoolIndex.push_back(i);
		}
	}

	ERROR_CODE UserManager::AddUser(const int sessionIndex, const char* userID)
	{
		if (FindUser(userID) != nullptr)
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;

		auto pUser = AllocNewUserIndex();
		if (pUser == nullptr)
			return ERROR_CODE::USER_MGR_MAX_USER_COUNT;

		pUser->Set(sessionIndex, userID);

		m_UserSessionDic.insert({ sessionIndex, pUser });
		m_UserIDDic.insert({ pUser->GetID().c_str(), pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE UserManager::RemoveUser(const int sessionIndex)
	{
		auto pUser = FindUser(sessionIndex);
		if (pUser == nullptr)
			return ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX;

		auto keyID = pUser->GetID().c_str();
		m_UserIDDic.erase(keyID);
		m_UserSessionDic.erase(sessionIndex);

		m_UserPoolIndex.push_back(pUser->GetIndex());
		pUser->Clear();

		return ERROR_CODE::NONE;
	}

	std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int sessionIndex)
	{
		auto pUser = FindUser(sessionIndex);

		if (pUser == nullptr)
			return std::make_tuple(ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr);

		if (pUser->IsConfirm() == false) {
			return std::make_tuple(ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr);
		}
		return std::make_tuple(ERROR_CODE::NONE, pUser);
	}
	
	User* UserManager::AllocNewUserIndex()
	{
		if (m_UserPoolIndex.empty())
			return nullptr;

		int index = m_UserPoolIndex.front();
		m_UserPoolIndex.pop_front();
		return &m_UserPool[index];
	}

	User* UserManager::FindUser(const int sessionIndex)
	{
		auto result = m_UserSessionDic.find(sessionIndex);

		if (result == m_UserSessionDic.end())
			return nullptr;

		return result->second;
	}

	User* UserManager::FindUser(const char * userID)
	{
		bool ret = true;
		User* result = nullptr;
		for (auto iter : m_UserIDDic)
		{
			ret = true;
			auto comp1 = std::string(iter.first);
			auto comp2 = std::string(userID);

			if (comp1.compare(comp2) == 0)
				result = iter.second;
		}

		return result;
	}
}