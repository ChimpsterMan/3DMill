// Requires GameNetworkingSockets to compile. See the build link: https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/BUILDING.md

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
#include "Server.h"
#include "Client.h"

// Board and game classes
#include "GameManager.h"
#include "Board.h"
#include "Piece.h"

// callback setup
void winCallback(Piece::Color color);

Server *Server::s_pCallbackInstance = nullptr;

Client *Client::s_pCallbackInstance = nullptr;

const uint16 DEFAULT_SERVER_PORT = 25565;

void PrintUsageAndExit()
{
	std::cout << "The information entered was invalid.\n" <<
		"Cmd argument usage:\n" << 
		"3DFourConnect.exe client SERVER_ADDR\n" <<
		"3DFourConnect.exe server [--port PORT]" << std::endl;
}

// start up options
int main(int argc, const char *argv[])
{
	bool bServer = false;
	bool bClient = false;
	bool bLocal = false;
	int nPort = DEFAULT_SERVER_PORT;
	SteamNetworkingIPAddr addrServer; addrServer.Clear();

	// test exe cmd args
	for (int i = 1; i < argc; ++i)
	{
		if (!bClient && !bServer)
		{
			if (!strcmp(argv[i], "client"))
			{
				bClient = true;
				continue;
			}
			if (!strcmp(argv[i], "server"))
			{
				bServer = true;
				continue;
			}
		}
		if (!strcmp(argv[i], "--port"))
		{
			++i;
			if (i >= argc)
				PrintUsageAndExit();
			nPort = atoi(argv[i]);
			if (nPort <= 0 || nPort > 65535)
				std::cout << "Invalid port " << nPort << std::endl;
			continue;
		}

		// Anything else, must be server address to connect to
		if (bClient && addrServer.IsIPv6AllZeros())
		{
			if (!addrServer.ParseString(argv[i]))
				std::cout << "Invalid server address " << argv[i] << std::endl;
			if (addrServer.m_port == 0)
				addrServer.m_port = DEFAULT_SERVER_PORT;
			continue;
		}

		PrintUsageAndExit();
	}

	// cin arguements if there is nothing in the main method arguements on launch
	if (argc == 1) {
		std::string type = "";
		std::string additionalInfo = "";

		std::cout << "No startup arguments were found. Please enter \"server\" (Server Host), \"client\" (Connect To Server), or \"local\" (Play On Same Computer)." << std::endl;
		while (type != "client" && type != "server" && type != "local") {
			std::cin >> type;

			if (type != "client" && type != "server" && type != "local") {
				std::cout << "Invalid arguements were given. Please say either \"server\" or \"client\"." << std::endl;
			}
		}

		if (type == "client") {
			bClient = true;

			std::cout << "Please enter the ip address of the server you want to connect to \"IP Address:Port\" (Port is optional)" << std::endl;
			std::cin >> additionalInfo;

			if (!addrServer.ParseString(additionalInfo.c_str()))
				std::cout << "Invalid server address " << additionalInfo.c_str() << std::endl;
			if (addrServer.m_port == 0)
				addrServer.m_port = DEFAULT_SERVER_PORT;
		}

		if (type == "server") {
			bServer = true;

			std::cout << "Please enter the port you wish to open the server on.\nType \"0\" and Port: " << nPort << " will be defaulted to.\nMake sure the port is open on your network aswell. (Port Forward)" << std::endl;
			std::cin >> additionalInfo;

			if (std::stoi(additionalInfo) != 0) {
				nPort = std::stoi(additionalInfo);
			}
		}

		if (type == "local") {
			bLocal = true;
		}
	}

	// if invalid entries for some reason
	if ((bClient == bServer || (bClient && addrServer.IsIPv6AllZeros())) && bLocal == false)
		PrintUsageAndExit();

	// get the base path and send it to the game
	// char basePath[255] = "";
	// _fullpath(basePath, argv[0], sizeof(basePath));
	// std::cout << basePath << std::endl;

	// Create client and server sockets
	InitSteamDatagramConnectionSockets();
	LocalUserInput_Init();

	// decide which game to make
	if (bLocal) {
		Local3DMill game;
		// game.gameManager.setWinCallback(winCallback);
		while (game.run() == 1) {};
	}
	else if (bClient)
	{
		Client client;
		client.Run(addrServer);
	}
	else
	{
		Server server;
		server.Run((uint16)nPort);
	}

	ShutdownSteamDatagramConnectionSockets();

	// Ug, why is there no simple solution for portable, non-blocking console user input?
	// Just nuke the process
	// LocalUserInput_Kill();
	// NukeProcess(0);
}

// test callback
void winCallback(Piece::Color color) {
	std::cout << "Win recieved" << std::endl;
}