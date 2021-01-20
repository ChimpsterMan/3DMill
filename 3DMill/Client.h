// Note: I did not write this code 100% myself.
// This is an adaptation of the GameNetworkingSockets public example 
// which was originally a simple chat program with clients and a server.

#ifndef CLIENT_H
#define CLIENT_H

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <cctype>

#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <GameNetworkingSockets/steam/steam_api.h>
#endif

#include <iostream>
#include <signal.h>

#include "Local3DMill.h"

#include "Tools.h"

// prototypes
// callbacks
void placePieceCallback(Piece::Color color, glm::vec3 pos);
void clearBoardCallback();
void outlinePieceMoveCallback(bool visible, glm::vec3 pos);

void* clientPtr;

class Client
{
public:
	Local3DMill game;
	int currentTurn;

	// Start and run the Client
	void Run(const SteamNetworkingIPAddr &serverAddr)
	{
		// setup local game stuff
		clientPtr = this;

		game.gameManager.setPiecePlaceCallback(placePieceCallback);
		game.gameManager.setClearBoardCallback(clearBoardCallback);
		game.gameManager.setOutlinePieceMoveCallback(outlinePieceMoveCallback);

		// disable fps counter
		game.enableFPSCounter = false;

		// Select instance to use.  For now we'll always use the default.
		m_pInterface = SteamNetworkingSockets();

		// Start connecting
		char szAddr[SteamNetworkingIPAddr::k_cchMaxString];
		serverAddr.ToString(szAddr, sizeof(szAddr), true);
		std::cout << "Connecting to chat server at " << szAddr << std::endl;
		SteamNetworkingConfigValue_t opt;
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
		m_hConnection = m_pInterface->ConnectByIPAddress(serverAddr, 1, &opt);
		if (m_hConnection == k_HSteamNetConnection_Invalid) {
			std::cout << "Failed to create connection" << std::endl;
		}

		std::cout << "Server commands include: '/quit' and '/clear'" << std::endl;

		// main loop
		while (!g_bQuit && game.run() == 1)
		{
			PollIncomingMessages();
			PollConnectionStateChanges();
			PollLocalUserInput();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void SendDataToServer(DataPacket *data) {
		m_pInterface->SendMessageToConnection(m_hConnection, data, (uint32)sizeof(*data), k_nSteamNetworkingSend_Reliable, nullptr);
	}

	// shortcut to just send all relevant info to the server
	void SendCurrentDataToServer() {
		// get the base packet
		DataPacket data = convertBoardToPacket();
		data.type = DataPacket::MsgType::GAME_DATA;

		// set scores
		data.score1 = game.gameManager.score1;
		data.score2 = game.gameManager.score2;

		// send turn after the turn has been switched already.
		data.currentTurn = game.gameManager.currentTurn;

		// send to server
		SendDataToServer(&data);
	}

	// game stuff
	// convert the pieces on the board to data 1's and 2's to represent red and blue respectivley.
	// returns a datapacket with the int array converted to numbers
	DataPacket convertBoardToPacket() {
		DataPacket data;
		data.type = DataPacket::MsgType::GAME_DATA;

		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				for (int z = 0; z < 4; z++) {
					if (game.gameManager.board.data[x][y][z][0].type == Piece::Color::NONE) {
						data.board[x][y][z] = 0;
					}
					if (game.gameManager.board.data[x][y][z][0].type == Piece::Color::RED) {
						data.board[x][y][z] = 1;
					}
					if (game.gameManager.board.data[x][y][z][0].type == Piece::Color::BLUE) {
						data.board[x][y][z] = 2;
					}
				}
			}
		}

		return data;
	}

private:

	string filepath;

	HSteamNetConnection m_hConnection;
	ISteamNetworkingSockets *m_pInterface;

	void PollIncomingMessages()
	{
		while (!g_bQuit)
		{
			ISteamNetworkingMessage *pIncomingMsg = nullptr;
			int numMsgs = m_pInterface->ReceiveMessagesOnConnection(m_hConnection, &pIncomingMsg, 1);
			if (numMsgs == 0)
				break;
			if (numMsgs < 0)
				std::cout << "Error checking for messages" << std::endl;

			// Just echo anything we get from the server
			// we trust anything coming from the server so just set the current board to whatever this is
			DataPacket *data = (DataPacket*)pIncomingMsg->m_pData;
			// std::cout << "data recieved" << std::endl;
			switch (data->type) {
				// connection info
				// currently broken for some reason
				case DataPacket::MsgType::CONNECTION_STATUS: {
					// std::cout << data->msg.c_str() << std::endl;
					break;
				}
				// a person is moving their outline piece
				case DataPacket::MsgType::GAME_SELECTION: {
					game.gameManager.setOpponentOutlinePiece(data->selectedVisible, data->selectedPiecePos);

					break;
				}
				// attempting to place a piece
				case DataPacket::MsgType::GAME_DATA: {
					// don't update the data on the board if one player is still in the win-pause menu.
					if (!game.gameManager.winPause) {
						// board pieces
						game.gameManager.board.setBoardToData(data);

						// set current scores
						game.gameManager.setScores((int)data->score1, (int)data->score2);

						// set the current turn
						game.gameManager.setTurnToInt(data->currentTurn);
					}

					break;
				}
				// First setup message recieved from server that specifies the clients turn (Color)
				case DataPacket::MsgType::GAME_SETUP: {
					game.gameManager.placeOnlyOnTurn = data->assignedTurn;

					break;
				}
				default: {
					std::cout << "Recieved data of no known type" << std::endl;
					break;
				}
			}

			// We don't need this anymore.
			pIncomingMsg->Release();
		}
	}

	void PollLocalUserInput()
	{
		std::string cmd;
		while (!g_bQuit && LocalUserInput_GetNext(cmd))
		{

			// Check for known commands
			if (strcmp(cmd.c_str(), "/quit") == 0)
			{
				g_bQuit = true;
				std::cout << "Disconnecting from chat server" << std::endl;

				// Close the connection
				m_pInterface->CloseConnection(m_hConnection, 0, "Goodbye", true);
				break;
			}

			// reset the game board in case of error
			if (strcmp(cmd.c_str(), "/clear") == 0) {
				DataPacket data;
				data.type = DataPacket::MsgType::GAME_DATA;
				data.currentTurn = 1;

				for (int x = 0; x < 4; x++) {
					for (int y = 0; y < 4; y++) {
						for (int z = 0; z < 4; z++) {
							data.board[x][y][z] = 0;
						}
					}
				}

				SendDataToServer(&data);
			}

			std::cout << "Server commands include: '/quit' and '/clear'" << std::endl;

			// Anything else, just send it to the server and let them parse it
			// m_pInterface->SendMessageToConnection(m_hConnection, cmd.c_str(), (uint32)cmd.length(), k_nSteamNetworkingSend_Reliable, nullptr);
		}
	}

	void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo)
	{
		assert(pInfo->m_hConn == m_hConnection || m_hConnection == k_HSteamNetConnection_Invalid);

		// What's the state of the connection?
		switch (pInfo->m_info.m_eState)
		{
		case k_ESteamNetworkingConnectionState_None:
			// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			g_bQuit = true;

			// Print an appropriate message
			if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
			{
				// Note: we could distinguish between a timeout, a rejected connection,
				// or some other transport problem.
				std::cout << "Host not found. " << pInfo->m_info.m_szEndDebug << std::endl;
			}
			else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				std::cout << "Contact lost with Host. " << pInfo->m_info.m_szEndDebug << std::endl;
			}
			else
			{
				// NOTE: We could check the reason code for a normal disconnection
				std::cout << "The host has disconnected. " << pInfo->m_info.m_szEndDebug << std::endl;
			}

			// Clean up the connection.  This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed.  We must close it on our end, too
			// to finish up.  The reason information do not matter in this case,
			// and we cannot linger because it's already closed on the other end,
			// so we just pass 0's.
			m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
			m_hConnection = k_HSteamNetConnection_Invalid;
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting:
			// We will get this callback when we start connecting.
			// We can ignore this.
			break;

		case k_ESteamNetworkingConnectionState_Connected:
			std::cout << "Connected to server OK" << std::endl;
			break;

		default:
			// Silences -Wswitch
			break;
		}
	}

	static Client *s_pCallbackInstance;
	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
	{
		s_pCallbackInstance->OnSteamNetConnectionStatusChanged(pInfo);
	}

	void PollConnectionStateChanges()
	{
		s_pCallbackInstance = this;
		m_pInterface->RunCallbacks();
	}
};

// called when a piece is moved
void outlinePieceMoveCallback(bool visible, glm::vec3 pos) {
	// send packet to server
	Client *client = (Client*)clientPtr;

	DataPacket data;
	data.type = data.GAME_SELECTION;
	data.selectedVisible = visible;
	data.selectedPiecePos = pos;

	client->SendDataToServer(&data);
}

// called when a piece is placed
void placePieceCallback(Piece::Color color, glm::vec3 pos) {
	// send packet to server
	Client *client = (Client*)clientPtr;
	// if it is the clients turn

	client->SendCurrentDataToServer();

	// std::cout << "sent message to server of type" << std::endl;
}

// called when the board is cleared
void clearBoardCallback() {
	Client *client = (Client*)clientPtr;

	// get empty board
	DataPacket data = client->convertBoardToPacket();

	// set packet type
	data.type = data.GAME_DATA;

	// set scores
	data.score1 = client->game.gameManager.score1;
	data.score2 = client->game.gameManager.score2;

	// set turn to neutral so the original turns remain.
	data.currentTurn = 0;

	client->SendDataToServer(&data);
}

#endif