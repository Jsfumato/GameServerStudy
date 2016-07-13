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
		if ((GetCurSceenType() == CLIENT_SCEEN_TYPE::LOBBY) || (GetCurSceenType() == CLIENT_SCEEN_TYPE::ROOM))
			return;

		m_RoomList.clear();
		SetRoomListGui();
		m_UserList.clear();
		SetUserListGui();
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
				SetCurSceenType(CLIENT_SCEEN_TYPE::LOBBY);
				m_LobbyRoomList->bgcolor(nana::colors::white);
				m_LobbyUserList->bgcolor(nana::colors::white);
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

			// 일단 받은 룸 목록은 업데이트를 합니다
			for (int i = 0; i < pktRes->Count; ++i)
			{
				UpdateRoomInfo(&pktRes->RoomInfo[i]);
			}

			// case 1:
			// 이 패킷이 끝이 아니라고 한다면, 마저 이어서 받을 수 있도록 새로 패킷을 전송합니다.
			if (pktRes->IsEnd == false)
			{
				RequestRoomList(pktRes->RoomInfo[pktRes->Count - 1].RoomIndex + 1);
			}
			// case 2:
			// 그게 아니라면 GUI를 세팅합시다
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
				UpdateUserInfo(false, pktRes->UserInfo[i].UserID, m_UserList, m_LobbyUserList);
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

		case (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES:
		{
			auto pktRes = (NCommon::PktRoomUserListRes*)pData;

			for (int i = 0; i < pktRes->Count; ++i)
			{
				UpdateUserInfo(false, pktRes->UserInfo[i].UserID, m_RoomUserList, m_RoomUserListBox);
			}

			if (pktRes->IsEnd == false)
			{
				RequestRoomUserList(pktRes->UserInfo[pktRes->Count - 1].LobbyUserIndex + 1);
			}
			else
			{
				SetRoomUserListGui();
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
			//RequestRoomList(0);
			m_MakeRoomBtn->enabled(false);
			m_EnterRoomBtn->caption("Leave Room");

			m_UserList.clear();
			m_RoomList.clear();
			m_LobbyUserList->clear();
			m_LobbyRoomList->clear();
			m_LobbyUserList->bgcolor(nana::colors::dim_grey);
			m_LobbyRoomList->bgcolor(nana::colors::dim_grey);
			
			m_RoomUserListBox->bgcolor(nana::colors::white);
			RequestRoomUserList(0);
			
			// 임시로 lobbyChatList에 적어보자
			m_ChatList->clear();
			AppendLobbyChat(std::wstring(L"<SYSTEM>"), std::wstring(L"Room에 입장하였습니다"));
		}
		break;

		case (short)PACKET_ID::ROOM_LEAVE_RES:
		{
			auto pktRes = (NCommon::PktRoomLeaveRes*)pData;
			if (pktRes->ErrorCode != (short)NCommon::ERROR_CODE::NONE)
			{
				break;
			}
			SetCurSceenType(CLIENT_SCEEN_TYPE::LOBBY);
			RequestRoomList(0);
			RequestUserList(0);
			m_MakeRoomBtn->enabled(true);
			m_EnterRoomBtn->caption("Enter Room");
			
			m_LobbyUserList->bgcolor(nana::colors::white);
			m_LobbyRoomList->bgcolor(nana::colors::white);

			m_RoomUserListBox->clear();
			m_RoomUserList.clear();

			// 임시로 lobbyChatList에 적어보자
			m_ChatList->clear();
			AppendLobbyChat(std::wstring(L"<SYSTEM>"), std::wstring(L"Lobby에 입장하였습니다"));
		}
		break;

		case (short) PACKET_ID::LOBBY_LEAVE_RES:
		{
			auto pktRes = (NCommon::PktRoomLeaveRes*)pData;
			if (pktRes->ErrorCode != (short)NCommon::ERROR_CODE::NONE)
			{
				break;
			}
			SetCurSceenType(CLIENT_SCEEN_TYPE::LOGIN);
			//RequestRoomList(0);
			m_MakeRoomBtn->enabled(false);
			m_EnterRoomBtn->caption("Enter Room");
			m_EnterRoomBtn->enabled(false);

			m_LobbyRoomList->clear();
			m_LobbyUserList->clear();
			m_RoomList.clear();
			m_UserList.clear();
			m_LobbyRoomList->bgcolor(nana::colors::dim_gray);
			m_LobbyUserList->bgcolor(nana::colors::dim_gray);
		}
		break;

		case (short)PACKET_ID::LOBBY_CHAT_NTF:
		{
			auto pktRes = (NCommon::PktLobbyChatNtf*)pData;
			wchar_t userID[NCommon::MAX_USER_ID_SIZE] = { 0 };
			wchar_t msg[NCommon::MAX_LOBBY_CHAT_MSG_SIZE] = { 0 };

			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &userID[0]);
			std::copy(&pktRes->Msg[0], &pktRes->Msg[NCommon::MAX_LOBBY_CHAT_MSG_SIZE], &msg[0]);
			AppendLobbyChat(std::wstring(userID), std::wstring(msg));
		}
		break;

		case (short)PACKET_ID::ROOM_CHAT_NTF:
		{
			auto pktRes = (NCommon::PktRoomChatNtf*)pData;
			wchar_t userID[NCommon::MAX_USER_ID_SIZE] = { 0 };
			wchar_t msg[NCommon::MAX_ROOM_CHAT_MSG_SIZE] = { 0 };

			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &userID[0]);
			std::copy(&pktRes->Msg[0], &pktRes->Msg[NCommon::MAX_ROOM_CHAT_MSG_SIZE], &msg[0]);
			AppendLobbyChat(std::wstring(userID), std::wstring(msg));
		}
		break;

		// Notify! User Leave!
		case (short)PACKET_ID::LOBBY_LEAVE_USER_NTF:
		{
			auto pktRes = (NCommon::PktLobbyLeaveUserInfoNtf*)pData;
			wchar_t userID[NCommon::MAX_USER_ID_SIZE] = { 0 };
			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &userID[0]);

			std::wstring ntfMessage = userID;
			ntfMessage += L"님이 Lobby를 떠나셨습니다.";

			AppendLobbyChat(std::wstring(L"<System>"), ntfMessage);

			UpdateUserInfo(true, pktRes->UserID, m_UserList, m_LobbyUserList);
		}
		break;

		// Notify! User Leave!
		case (short)PACKET_ID::ROOM_LEAVE_USER_NTF:
		{
			auto pktRes = (NCommon::PktRoomLeaveUserInfoNtf*)pData;
			wchar_t userID[NCommon::MAX_USER_ID_SIZE] = { 0 };
			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &userID[0]);

			std::wstring ntfMessage = userID;
			ntfMessage += L"님이 Room을 떠나셨습니다.";

			AppendLobbyChat(std::wstring(L"<System>"), ntfMessage);

			UpdateUserInfo(true, pktRes->UserID, m_RoomUserList, m_RoomUserListBox);
		}
		break;

		case (short)PACKET_ID::LOBBY_ENTER_USER_NTF:
		{
			auto pktRes = (NCommon::PktLobbyLeaveUserInfoNtf*)pData;
			wchar_t userID[NCommon::MAX_USER_ID_SIZE] = { 0 };
			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &userID[0]);

			std::wstring ntfMessage = userID;
			ntfMessage += L"님이 Lobby에 입장하셨습니다.";

			AppendLobbyChat(std::wstring(L"<System>"), ntfMessage);
			UpdateUserInfo(true, pktRes->UserID, m_UserList, m_LobbyUserList);
			
			RequestUserList(0);
		}
		break;

		case (short)PACKET_ID::ROOM_ENTER_USER_NTF:
		{
			auto pktRes = (NCommon::PktRoomEnterUserInfoNtf*)pData;
			wchar_t userID[NCommon::MAX_USER_ID_SIZE] = { 0 };
			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &userID[0]);

			std::wstring ntfMessage = userID;
			ntfMessage += L"님이 Room에 입장하셨습니다.";

			AppendLobbyChat(std::wstring(L"<System>"), ntfMessage);
			UpdateUserInfo(true, pktRes->UserID, m_RoomUserList, m_RoomUserListBox);

			RequestRoomUserList(0);
		}
		break;

		// Lobby Chat Res 
		// 자신이 입력한 값이 올라간다
		case (short)PACKET_ID::LOBBY_CHAT_RES:
		{
			auto pktRes = (NCommon::PktLobbyChatRes*)pData;
			if (pktRes->ErrorCode != (short)NCommon::ERROR_CODE::NONE)
			{
				break;
			}
			
			AppendLobbyChat(std::wstring(L"나"), m_ChatInput->caption_wstring());
			m_ChatInput->reset();
		}
		break;

		case (short)PACKET_ID::ROOM_CHAT_RES:
		{
			auto pktRes = (NCommon::PktRoomChatRes*)pData;
			if (pktRes->ErrorCode != (short)NCommon::ERROR_CODE::NONE)
			{
				break;
			}

			AppendLobbyChat(std::wstring(L"나"), m_ChatInput->caption_wstring());
			m_ChatInput->reset();
		}
		break;

		case (short)PACKET_ID::WHISPER_RES:
		{
			auto pktRes = (NCommon::PktWhisperRes*)pData;
			if (pktRes->ErrorCode != (short)NCommon::ERROR_CODE::NONE)
			{
				AppendLobbyChat(std::wstring(L"<SYSTEM>"), std::wstring(L"귓속말이 실패했습니다"));
				m_ChatInput->reset();
				break;
			}
			wchar_t wSrcID[NCommon::MAX_USER_ID_SIZE];
			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &wSrcID[0]);
			
			std::wstring whisperMsg = m_ChatInput->caption_wstring();
			int readPos = 0;
			for (readPos; readPos < whisperMsg.size(); ++readPos)
			{
				if (whisperMsg[readPos] == L' ')
				{
					readPos += 1;
					break;
				}
			}
			wchar_t whisper[NCommon::MAX_WHISPER_CHAT_MSG_SIZE] = { 0 };
			std::copy(&whisperMsg[readPos], &whisperMsg[whisperMsg.size() - 2], &whisper[0]);

			AppendLobbyChat(std::wstring(L"to ")+ wSrcID, std::wstring(whisper));
			m_ChatInput->reset();
		}
		break;

		case (short)PACKET_ID::WHISPER_NTF:
		{
			auto pktRes = (NCommon::PktWhisperNtf*)pData;

			wchar_t wSrcID[NCommon::MAX_USER_ID_SIZE];
			std::copy(&pktRes->UserID[0], &pktRes->UserID[NCommon::MAX_USER_ID_SIZE], &wSrcID[0]);

			AppendLobbyChat(std::wstring(L"from ") + wSrcID, std::wstring(pktRes->Msg));
		}
		break;
		
		case (short)PACKET_ID::ROOM_CHANGED_INFO_NTF:
		{
			auto pktRes = (NCommon::PktChangedRoomInfoNtf*)pData;
			UpdateRoomInfo(&pktRes->RoomInfo);
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

		m_ChatList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(700, 106, 400, 383));
		m_ChatList->append_header(L"User", 60);
		m_ChatList->append_header(L"Chat", 300);
			
		m_RoomUserListBox = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(1100, 106, 140, 383));
		m_RoomUserListBox->append_header(L"UserID", 130);
		m_RoomUserListBox->bgcolor(nana::colors::dim_gray);

		m_ChatInput = std::make_shared<textbox>((form&)*m_pForm, nana::rectangle(700, 500, 400, 25));
		m_ChatInput->events().key_press([&](arg_keyboard ak) {
			if (m_ChatInput->focused() == false)
				return;
			if (ak.key != 13)
				return;

			std::wstring msg = m_ChatInput->caption_wstring();
			if (msg.size() == 0)
				return;

			if (msg[0] == L'/' && ((msg[1] == 'w') || (msg[1] == 'ㅈ')))
			{
				std::wstring destID;
				/*std::wstring whisper;*/
				int readPos = 2;
				for (readPos; readPos < msg.size(); ++readPos)
				{
					if (msg[readPos] == L' ')
					{
						readPos += 1;
						break;
					}
					destID.push_back(msg[readPos]);
				}

				int msgSize = msg.size() - readPos;
				wchar_t whisper[NCommon::MAX_WHISPER_CHAT_MSG_SIZE] = { 0 };
				std::copy(&msg[readPos], &msg[readPos + msgSize], &whisper[0]);
				SendWhisper(destID.c_str(), destID.size(), whisper, msgSize);
				return;
			}

			wchar_t pMsg[256] = { 0, };
			std::copy(&msg[0], &msg[msg.size()], &pMsg[0]);
			
			if(GetCurSceenType() == CLIENT_SCEEN_TYPE::LOBBY)
				SendLobbyChat(pMsg, msg.size());
			if (GetCurSceenType() == CLIENT_SCEEN_TYPE::ROOM)
				SendRoomChat(pMsg, msg.size());
		});

		m_LobbyRoomList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(204, 106, 345, 383));
		m_LobbyRoomList->append_header(L"RoomId", 50);
		m_LobbyRoomList->append_header(L"Title", 165);
		m_LobbyRoomList->append_header(L"Cur", 30);
		m_LobbyRoomList->append_header(L"Max", 30);
		m_LobbyRoomList->bgcolor(nana::colors::dim_gray);

		m_LobbyUserList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(550, 106, 120, 383));
		m_LobbyUserList->append_header("UserID", 90);
		m_LobbyUserList->bgcolor(nana::colors::dim_gray);

		m_MakeRoomName = std::make_shared<textbox>((form&)*m_pForm, nana::rectangle(204, 540, 300, 25));

		m_MakeRoomBtn = std::make_shared<button>((form&)*m_pForm, nana::rectangle(204, 500, 100, 25));
		m_MakeRoomBtn->caption("Make Room");
		m_MakeRoomBtn->events().click([&]() {
			wchar_t roomName[MAX_ROOM_TITLE_SIZE] = { 0 };
			std::wstring title = m_MakeRoomName->caption_wstring();
			if (title == L"")
				return;

			title.resize(MAX_ROOM_TITLE_SIZE);
			std::copy(&title[0], &title[MAX_ROOM_TITLE_SIZE], &roomName[0]);
			this->EnterRoom(true, roomName);
			m_MakeRoomName->reset();
		});

		m_EnterRoomBtn = std::make_shared<button>((form&)*m_pForm, nana::rectangle(310, 500, 100, 25));
		m_EnterRoomBtn->caption("Enter Room");
		m_EnterRoomBtn->events().click([&]() {

			if (GetCurSceenType() == CLIENT_SCEEN_TYPE::ROOM)
			{
				LeaveRoom();
			}
			else if(GetCurSceenType() == CLIENT_SCEEN_TYPE::LOBBY)
			{
				wchar_t roomName[MAX_ROOM_TITLE_SIZE] = { 0, };
				auto room = m_LobbyRoomList->selected();
				auto index = room[0].item;

				this->EnterRoom(false, index);
			}
		});
	}

	void LeaveRoom()
	{
		NCommon::PktRoomLeaveReq reqPkt;
		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_LEAVE_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void EnterRoom(bool isCreate, wchar_t* roomName)
	{
		NCommon::PktRoomEnterReq reqPkt;
		reqPkt.IsCreate = isCreate;
		std::copy(&roomName[0], &roomName[MAX_ROOM_TITLE_SIZE], &reqPkt.RoomTitle[0]);
		
		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_ENTER_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void EnterRoom(bool isCreate, const int roomIndex)
	{
		NCommon::PktRoomEnterReq reqPkt;
		reqPkt.IsCreate = isCreate;
		reqPkt.RoomIndex = roomIndex;
		
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
	
	void RequestRoomUserList(const short startIndex)
	{
		NCommon::PktRoomUserListReq reqPkt;
		reqPkt.StartUserIndex = startIndex;
		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_ENTER_USER_LIST_REQ, sizeof(reqPkt), (char*)&reqPkt);
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

	void SetRoomUserListGui()
	{
		m_IsUserListWorking = false;

		for (auto & userId : m_RoomUserList)
		{
			m_RoomUserListBox->at(0).append({ userId });
		}

		m_RoomUserList.clear();
	}

	// 귓속말 기능 구현
	void SendWhisper(const wchar_t* destID, int destIDSize, const wchar_t* pMsg, int msgSize)
	{
		std::wstring test = m_ChatInput->caption_wstring();

		destIDSize = (destIDSize > NCommon::MAX_USER_ID_SIZE) ? NCommon::MAX_USER_ID_SIZE : destIDSize;

		NCommon::PktWhisperReq reqPkt;
		std::copy(&destID[0], &destID[destIDSize], &reqPkt.UserID[0]);
		std::copy(&pMsg[0], &pMsg[msgSize], &reqPkt.Msg[0]);
		m_pRefNetwork->SendPacket((short)PACKET_ID::WHISPER_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	// LOBBY에서의 채팅 기능 구현
	void SendLobbyChat(wchar_t* pMsg, int msgSize)
	{
		std::wstring test = m_ChatInput->caption_wstring();
		
		NCommon::PktLobbyChatReq reqPkt;
		std::copy(&pMsg[0], &pMsg[msgSize], &reqPkt.Msg[0]);
		m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_CHAT_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	// LOBBY에서의 채팅 기능 구현
	void SendRoomChat(wchar_t* pMsg, int msgSize)
	{
		std::wstring test = m_ChatInput->caption_wstring();

		NCommon::PktRoomChatReq reqPkt;
		std::copy(&pMsg[0], &pMsg[msgSize], &reqPkt.Msg[0]);
		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_CHAT_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void UpdateRoomInfo(NCommon::RoomSmallInfo* pRoomInfo)
	{
		NCommon::RoomSmallInfo newRoom;
		memcpy(&newRoom, pRoomInfo, sizeof(NCommon::RoomSmallInfo));
		
		bool IsRemove = newRoom.RoomUserCount == 0 ? true : false;

		if (m_IsRoomListWorking == true)
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
											std::wstring(newRoom.RoomTitle),
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

	void UpdateUserInfo(bool IsRemove, std::string userID, std::list<std::string>& userList, std::shared_ptr<listbox> userListBox)
	{
		if (m_IsUserListWorking)
		{
			if (IsRemove == false)
			{
				auto findIter = std::find_if(std::begin(userList), std::end(userList), [&userID](auto& ID) { return ID == userID; });

				if (findIter == std::end(userList))
				{
					userList.push_back(userID);
				}
			}
			else
			{
				userList.remove_if([&userID](auto& ID) { return ID == userID; });
			}
		}
		else
		{
			if (IsRemove == false)
			{
				for (auto& user : userListBox->at(0))
				{
					if (user.text(0) == userID) {
						return;
					}
				}

				userListBox->at(0).append(userID);
			}
			else
			{
				auto i = 0;
				for (auto& user : userListBox->at(0))
				{
					if (user.text(0) == userID)
					{
						userListBox->erase(user);
						return;
					}
				}
			}
		}
	}

	/*void UpdateUserInfo(bool IsRemove, std::string userID)
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
	}*/

	void AppendLobbyChat(std::wstring& userID, std::wstring& chatMsg)
	{
		//m_ChatListTab->at(0)->at(0)->append({ userID, chatMsg });
		m_ChatList->at(0)->append({ userID, chatMsg });
	}

private:
	form* m_pForm = nullptr;
	std::shared_ptr<listbox> m_LobbyRoomList;
	std::shared_ptr<listbox> m_LobbyUserList;
	std::shared_ptr<listbox> m_RoomUserListBox;
	
	std::shared_ptr<listbox> m_ChatList;
	std::shared_ptr<textbox> m_ChatInput;
	
	std::shared_ptr<textbox> m_MakeRoomName;
	std::shared_ptr<button> m_MakeRoomBtn;
	std::shared_ptr<button> m_EnterRoomBtn;

	int m_MaxUserCount = 0;

	bool m_IsRoomListWorking = false;
	std::list<NCommon::RoomSmallInfo> m_RoomList;

	bool m_IsUserListWorking = false;
	std::list<std::string> m_UserList;
	std::list<std::string> m_RoomUserList;
};
