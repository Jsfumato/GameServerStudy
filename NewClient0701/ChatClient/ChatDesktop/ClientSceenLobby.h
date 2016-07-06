#pragma once

#include <list>
#include <string>
#include <vector>
#include <algorithm>
//#include "../../Common/Packet.h"

#include "IClientSceen.h"

constexpr int MAX_ROOM_TITLE_SIZE = 16;

class ClientSceenLobby : public IClientSceen
{
public:
	ClientSceenLobby() {}
	virtual ~ClientSceenLobby() {}

	virtual void Update() override 
	{
	}

	bool ProcessPacket(const short packetId, char* pData) override 
	{ 
		switch (packetId)
		{
		case (short)PACKET_ID::LOBBY_ENTER_RES:
		{
			auto pktRes = (NCommon::PktLobbyEnterRes*)pData;

			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{
				Init(pktRes->MaxUserCount);

				RequestRoomList(0);
				RequestUserList(0);
			}
			else
			{
				std::cout << "[LOBBY_ENTER_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}
		}
			break;
			
		case (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES:
		{
			auto pktRes = (NCommon::PktLobbyRoomListRes*)pData;

			for (int i = 0; i < pktRes->Count; ++i)
			{
				UpdateRoomInfo(&pktRes->RoomInfo[i]);
			}

			if (pktRes->IsEnd == false)
			{
				RequestRoomList(pktRes->RoomInfo[pktRes->Count - 1].RoomIndex + 1);
			}
			else
			{
				SetRoomListGui();
			}
		}
			break;

		case (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES:
		{
			auto pktRes = (NCommon::PktLobbyUserListRes*)pData;

			for (int i = 0; i < pktRes->Count; ++i)
			{
				UpdateUserInfo(false, pktRes->UserInfo[i].UserID);
			}

			if (pktRes->IsEnd == false)
			{
				RequestUserList(pktRes->UserInfo[pktRes->Count - 1].LobbyUserIndex + 1);
			}
			else
			{
				SetUserListGui();
			}
		}
			break;

		case (short)PACKET_ID::ROOM_ENTER_RES:
		{
			auto pktRes = (NCommon::PktRoomEnterRes*)pData;
			if (pktRes->ErrorCode != (short)NCommon::ERROR_CODE::NONE)
			{
				break;
			}
			SetCurSceenType(CLIENT_SCEEN_TYPE::ROOM);
			RequestRoomList(0);
			m_MakeRoomBtn->enabled(false);
		}
		break;


		case (short)PACKET_ID::ROOM_CHANGED_INFO_NTF:
		{
			auto pktRes = (NCommon::PktChangedRoomInfoNtf*)pData;
			UpdateRoomInfo(&pktRes->RoomInfo);
		}
			break;
		case (short)PACKET_ID::LOBBY_ENTER_USER_NTF:
		{
			auto pktRes = (NCommon::PktLobbyNewUserInfoNtf*)pData;
			UpdateUserInfo(false, pktRes->UserID);
		}
			break;
		case (short)PACKET_ID::LOBBY_LEAVE_USER_NTF:
		{
			auto pktRes = (NCommon::PktLobbyLeaveUserInfoNtf*)pData;
			UpdateUserInfo(true, pktRes->UserID);
		}
			break;
		default:
			return false;
		}

		return true;
	}

	void CreateUI(form* pform)
	{
		m_pForm = pform;

		m_LobbyChatList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(700, 106, 200, 383));
		m_LobbyChatList->append_header(L"text");
		
		m_LobbyChatInput = std::make_shared<textbox>((form&)*m_pForm, nana::rectangle(700, 500, 200, 25));
		m_LobbyChatInput->events().key_press([&](arg_keyboard ak) {
			if (m_LobbyChatInput->focused() == false)
				return;
			
			if (ak.key == 13)
				SendLobbyChat();
		});

		m_LobbyRoomList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(204, 106, 345, 383));
		m_LobbyRoomList->append_header(L"RoomId", 50);
		m_LobbyRoomList->append_header(L"Title", 165);
		m_LobbyRoomList->append_header(L"Cur", 30);
		m_LobbyRoomList->append_header(L"Max", 30);

		m_LobbyUserList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(550, 106, 120, 383));
		m_LobbyUserList->append_header("UserID", 90);

		m_MakeRoomName = std::make_shared<textbox>((form&)*m_pForm, nana::rectangle(204, 540, 300, 25));

		m_MakeRoomBtn = std::make_shared<button>((form&)*m_pForm, nana::rectangle(204, 500, 100, 25));
		m_MakeRoomBtn->caption("Make Room");
		m_MakeRoomBtn->events().click([&]() {
			wchar_t roomName[MAX_ROOM_TITLE_SIZE] = { 0, };

			auto title = m_MakeRoomName->caption_wstring().c_str();
			
			std::copy(&title[0], &title[MAX_ROOM_TITLE_SIZE], &roomName[0]);
			this->EnterRoom(true, roomName);
		});

		m_EnterRoomBtn = std::make_shared<button>((form&)*m_pForm, nana::rectangle(310, 500, 100, 25));
		m_EnterRoomBtn->caption("Enter Room");
		m_EnterRoomBtn->events().click([&]() {
			wchar_t roomName[MAX_ROOM_TITLE_SIZE] = { 0, };
			/*auto room = m_LobbyRoomList->selected();
			auto index = room[0].item;
			auto roomIndex = m_LobbyRoomList->at(0).at(index).text(0).c_str();
			wchar_t* roomName = m_LobbyRoomList->at(1).at(index).text(0).c_str();*/
			this->EnterRoom(false, roomName);
		});
	}

	void EnterRoom(bool isCreate, wchar_t* roomName)
	{
		NCommon::PktRoomEnterReq reqPkt;
		reqPkt.IsCreate = isCreate;
		std::copy(&roomName[0], &roomName[MAX_ROOM_TITLE_SIZE], &reqPkt.RoomTitle[0]);
		
		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_ENTER_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void Init(const int maxUserCount)
	{
		m_MaxUserCount = maxUserCount;

		m_IsRoomListWorking = true;
		m_IsUserListWorking = true;

		m_RoomList.clear();
		m_UserList.clear();
	}
		
	void RequestRoomList(const short startIndex)
	{
		NCommon::PktLobbyRoomListReq reqPkt;
		reqPkt.StartRoomIndex = startIndex;
		m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void RequestUserList(const short startIndex)
	{
		NCommon::PktLobbyUserListReq reqPkt;
		reqPkt.StartUserIndex = startIndex;
		m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_ENTER_USER_LIST_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}
	
	void SetRoomListGui()
	{
		m_IsRoomListWorking = false;

		for (auto & room : m_RoomList)
		{
			m_LobbyRoomList->at(0).append({ std::to_wstring(room.RoomIndex),
				room.RoomTitle,
				std::to_wstring(room.RoomUserCount),
				std::to_wstring(m_MaxUserCount) });
		}

		m_RoomList.clear();
	}

	void SetUserListGui()
	{
		m_IsUserListWorking = false;

		for (auto & userId : m_UserList)
		{
			m_LobbyUserList->at(0).append({ userId });
		}

		m_UserList.clear();
	}

	// LOBBY에서의 채팅 기능 구현
	void SendLobbyChat()
	{
		std::wstring test = m_LobbyChatInput->caption_wstring().c_str();
		//auto pktReq = NCommon::P
	}

	void UpdateRoomInfo(NCommon::RoomSmallInfo* pRoomInfo)
	{
		NCommon::RoomSmallInfo newRoom;
		memcpy(&newRoom, pRoomInfo, sizeof(NCommon::RoomSmallInfo));
		
		bool IsRemove = newRoom.RoomUserCount == 0 ? true : false;

		if (m_IsRoomListWorking)
		{
			if (IsRemove == false)
			{
				auto findIter = std::find_if(std::begin(m_RoomList), std::end(m_RoomList), [&newRoom](auto& room) { return room.RoomIndex == newRoom.RoomIndex; });

				if (findIter != std::end(m_RoomList))
				{
					wcsncpy_s(findIter->RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, newRoom.RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE);
					findIter->RoomUserCount = newRoom.RoomUserCount;
				}
				else
				{
					m_RoomList.push_back(newRoom);
				}
			}
			else
			{
				m_RoomList.remove_if([&newRoom](auto& room) { return room.RoomIndex == newRoom.RoomIndex; });
			}
		}
		else
		{
			std::string roomIndex(std::to_string(newRoom.RoomIndex));

			if (IsRemove == false)
			{
				for (auto& room : m_LobbyRoomList->at(0))
				{
					if (room.text(0) == roomIndex) 
					{
						room.text(1, newRoom.RoomTitle);
						room.text(2, std::to_wstring(newRoom.RoomUserCount));
						return;
					}
				}

				m_LobbyRoomList->at(0).append({ std::to_wstring(newRoom.RoomIndex),
											newRoom.RoomTitle,
										std::to_wstring(newRoom.RoomUserCount),
										std::to_wstring(m_MaxUserCount) });
			}
			else
			{
				for (auto& room : m_LobbyRoomList->at(0))
				{
					if (room.text(0) == roomIndex)
					{
						m_LobbyRoomList->erase(room);
						return;
					}
				}
			}
		}
	}

	void UpdateUserInfo(bool IsRemove, std::string userID)
	{		
		if (m_IsUserListWorking)
		{
			if (IsRemove == false)
			{
				auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [&userID](auto& ID) { return ID == userID; });

				if (findIter == std::end(m_UserList))
				{
					m_UserList.push_back(userID);
				}
			}
			else
			{
				m_UserList.remove_if([&userID](auto& ID) { return ID == userID; });
			}
		}
		else
		{
			if (IsRemove == false)
			{
				for (auto& user : m_LobbyUserList->at(0))
				{
					if (user.text(0) == userID) {
						return;
					}
				}

				m_LobbyUserList->at(0).append(userID);
			}
			else
			{
				auto i = 0;
				for (auto& user : m_LobbyUserList->at(0))
				{
					if (user.text(0) == userID)
					{
						m_LobbyUserList->erase(user);
						return;
					}
				}
			}
		}
	}

private:
	form* m_pForm = nullptr;
	std::shared_ptr<listbox> m_LobbyRoomList;
	std::shared_ptr<listbox> m_LobbyUserList;
	
	std::shared_ptr<listbox> m_LobbyChatList;
	std::shared_ptr<textbox> m_LobbyChatInput;
	
	std::shared_ptr<textbox> m_MakeRoomName;
	std::shared_ptr<button> m_MakeRoomBtn;
	std::shared_ptr<button> m_EnterRoomBtn;

	int m_MaxUserCount = 0;

	bool m_IsRoomListWorking = false;
	std::list<NCommon::RoomSmallInfo> m_RoomList;

	bool m_IsUserListWorking = false;
	std::list<std::string> m_UserList;
};
