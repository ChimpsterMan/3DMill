3D Mill
Written in c++ and compiled with libraries for 64bit;

Requires VC_Redist 2015 or Later

Instructions: 
 	The goal of 3D Mill is to remove all of your opponent's pieces that 
 are on the board and in reserve. You can remove one of your opponent's 
 pieces from the board when you get a mill which is three of your pieces 
 in a row along the black lines of the board. If you get multiple mills in 
 one turn, you can remove the corresponding number of your opponent's pieces 
 from the board. However, you may not remove any of your opponent's pieces 
 that are currently in a mill. If all of your opponent's pieces are in a 
 mill and you get a mill, none of your opponent's pieces are removed. On 
 each turn, you must either add a piece to the board from your reserve or 
 move an existing piece. Existing pieces may only be moved one space along 
 any of the adjacent black lines. The exception to this is when you only 
 have a combined total of three pieces in your reserve and on the board. 
 The game will end when either player has less than three total pieces 
 on the board and in their reserve.

Menu Selections:
	There are 3 possible menu selections once you launch the program: 
"local", "server", or "client". Local refers to playing on one computer 
where the players physically take turns controlling the mouse and placing 
pieces. Server refers to the program that will host a multiplayer server 
for players to connect to. Once selected, it will request a port for the 
server to be opened on. You must set your router to port forward the port 
selected to the server host's computer. Client finally is what players 
select when they wish to connect to a server. They are then prompted for 
the IP Address of the network the server and the port in the format 
"IP_ADDRESS:PORT". Entering this will connect them to the server and once 
two players join, the game will start.

Controls: 
	Use W, A, S, and D to rotate the camera view around the board. 
You can also place pieces by moving your mouse over the spot where the 
piece will be and pressing left click.

Note:
	If more than two people join the server, they will be rejected. 
In order to connect two new people, the server must be remade.

Installation (Project Files):
1. Download the entire repository
2. If the libraries imported are out of date or not working:
	1. Check that the project properties is importing them correctly
	2. Rebuild the libs (Listed Below) (Recommend CMAKE Compiler: https://cmake.org/download/)

Libraries
OpenGL: https://www.opengl.org//
GLFW: https://www.glfw.org/download.html
GLAD: https://glad.dav1d.de/
Assimp: http://assimp.org/
FreeType: https://www.freetype.org/
GameNetworkingSockets: https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/BUILDING.md

Credits:
Server Code Based On:
https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/examples/example_chat.cpp

Educational Links:
https://learnopengl.com/Getting-started/Creating-a-window
https://partner.steamgames.com/doc/api/ISteamNetworkingSockets