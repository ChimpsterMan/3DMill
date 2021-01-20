// always include Tools.h for shared methods between server and client
#ifndef TOOLS_H
#define TOOLS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

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

#include <iostream>

#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <GameNetworkingSockets/steam/steam_api.h>
#endif

#include <signal.h>

static bool g_bQuit = false;

static SteamNetworkingMicroseconds g_logTimeZero;

// includes the board, currently selected piece, and the scores of both players
// score is optional and only for the server to use
struct DataPacket
{
	// game data handles per move data, game_setup sends the setup info to the clients, game selection is a per selection update that just sends the position of cursor, connection status is basically just a message
	enum MsgType {GAME_DATA, GAME_SETUP, GAME_SELECTION, CONNECTION_STATUS};
	MsgType type;

	// connection status info
	std::string msg;

	// game setup
	int assignedTurn;

	// game selection
	bool selectedVisible;
	// board coordinate system 0-3 inclusive, not world space
	glm::vec3 selectedPiecePos;

	// game data info
	// 0 is None, 1 is red, blue is 2
	int score1 = 0;
	int score2 = 0;
	int currentTurn;
	int board[4][4][4];
};

// static methods
static void DebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char *pszMsg) {
	std::cout << pszMsg << std::endl;
}

static void InitSteamDatagramConnectionSockets()
{
#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	SteamDatagramErrMsg errMsg;
	if (!GameNetworkingSockets_Init(nullptr, errMsg))
		std::cout << "GameNetworkingSockets_Init failed. " << errMsg << std::endl;
#else
	SteamDatagramClient_SetAppID(570); // Just set something, doesn't matter what
	// SteamDatagramClient_SetUniverse( k_EUniverseDev );

	SteamDatagramErrMsg errMsg;
	if (!SteamDatagramClient_Init(true, errMsg))
		FatalError("SteamDatagramClient_Init failed.  %s", errMsg);

	// Disable authentication when running with Steam, for this
	// example, since we're not a real app.
	// 
	// Authentication is disabled automatically in the open-source
	// version since we don't have a trusted third party to issue
	// certs.
	SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);
#endif

	g_logTimeZero = SteamNetworkingUtils()->GetLocalTimestamp();

	SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg, DebugOutput);
}

static void ShutdownSteamDatagramConnectionSockets()
{
	// Give connections time to finish up.  This is an application layer protocol
	// here, it's not TCP.  Note that if you have an application and you need to be
	// more sure about cleanup, you won't be able to do this.  You will need to send
	// a message and then either wait for the peer to close the connection, or
	// you can pool the connection to see if any reliable data is pending.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

#ifdef STEAMNETWORKINGSOCKETS_OPENSOURCE
	GameNetworkingSockets_Kill();
#else
	SteamDatagramClient_Kill();
#endif
}

// vars

static std::mutex mutexUserInputQueue;
static std::queue< std::string > queueUserInput;

static std::thread *s_pThreadUserInput = nullptr;

void LocalUserInput_Init();

void LocalUserInput_Kill();

// trim from start (in place)
static inline void ltrim(std::string &s);

// trim from end (in place)
static inline void rtrim(std::string &s);


// Read the next line of input from stdin, if anything is available.
bool LocalUserInput_GetNext(std::string &result);

#endif