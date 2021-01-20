// the manager for a rendered 3dfourconnect game.

#ifndef LOCAL3DMILL_H
#define LOCAL3DMILL_H

// utility
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <img/stb_image.h>
// idk but manually importing fixes the problem
// #include <img/ImageLoader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <shader.h>

#include <FileSystem>
#include <iostream>

#include <vector>
#include <chrono>
#include <cmath>
#include <ctime>
#include <windows.h>

// written class refrences
#include "GraphicsEngine.h"
#include "TextManager.h"

// Board and game classes
#include "GameManager.h"
#include "Board.h"
#include "Piece.h"

// file paths
// models
inline const char* RedModelPath = "resources\\objects\\redball\\redball.obj";
inline const char* BlueModelPath = "resources\\objects\\blueball\\blueball.obj";
inline const char* OutlineModelPath = "resources\\objects\\outlineball\\outlineball.obj";
inline const char* BoardModelPath = "resources\\objects\\3dmill\\3dmill.obj";
inline const char* BackpackModelPath = "resources\\objects\\testing\\backpack\\backpack.obj";

// skybox paths
inline const std::vector<const char*> cloudySkybox
{
	"resources/textures/CloudySkyBox/cubemap_1.jpg",
	"resources/textures/CloudySkyBox/cubemap_3.jpg",
	"resources/textures/CloudySkyBox/cubemap_4.jpg",
	"resources/textures/CloudySkyBox/cubemap_5.jpg",
	"resources/textures/CloudySkyBox/cubemap_0.jpg",
	"resources/textures/CloudySkyBox/cubemap_2.jpg",
};

// unused second option for skybox
inline const std::vector<const char*> galexySkybox
{
	"resources/textures/SpaceSkyBox/rightImage.png",
	"resources/textures/SpaceSkyBox/leftImage.png",
	"resources/textures/SpaceSkyBox/upImage.png",
	"resources/textures/SpaceSkyBox/downImage.png",
	"resources/textures/SpaceSkyBox/frontImage.png",
	"resources/textures/SpaceSkyBox/backImage.png",
};

// prototypes
// control callback for clicking the mouse
inline void mouse_button_callback_custom(GLFWwindow* window, int button, int action, int mods);
// control callback for moving the mouse
inline void mouse_callback_custom(GLFWwindow* window, double xpos, double ypos);

// settings
inline float relativeScreenSize = 0.85;

inline float aspectRatio = 16 / 9;

// default
inline unsigned int SCR_WIDTH = 1600*relativeScreenSize;
inline unsigned int SCR_HEIGHT = 900*relativeScreenSize;

inline bool fpsCounter = true;
inline const double fps = 60;

inline GameManager *gM;

// set the static variable filepath before you create a class
class Local3DMill {
public:
	// make the graphics engine (Jordan: Do not focus too much on this, it is very complicated and not relevant to the problem.
	GraphicsEngine graphics;

	// make the board and game manager
	GameManager gameManager;

	int fpsCount;
	int fpsCounter;

	int gameState;

	bool enableFPSCounter;

	Local3DMill() {
		// set window size to max while also maintaining size ratio
		RECT rect;
		GetClientRect(GetDesktopWindow(), &rect);

		SCR_WIDTH = (rect.right - rect.left) * relativeScreenSize;
		SCR_HEIGHT = (rect.bottom - rect.top) * relativeScreenSize;

		if (SCR_WIDTH / SCR_HEIGHT < aspectRatio) {
			// base the size off the width
			SCR_HEIGHT = SCR_WIDTH * (1 / aspectRatio);
		}
		if (SCR_HEIGHT / SCR_WIDTH > aspectRatio) {
			// base the size off the height
			SCR_WIDTH = SCR_HEIGHT * (aspectRatio);
		}

		// make the graphics engine (Jordan: Do not focus too much on this, it is very complicated and not relevant to the problem.
		graphics = GraphicsEngine("3D Four Connect", &SCR_WIDTH, &SCR_HEIGHT, 1, true);

		// add all the models that are going to be used immediatley
		graphics.addModel(BoardModelPath);
		graphics.addModel(RedModelPath);
		graphics.addModel(BlueModelPath);
		graphics.addModel(OutlineModelPath);

		// set skybox
		graphics.setSkybox(cloudySkybox);

		// set light
		graphics.setLight(glm::vec3(100, 150, 100), glm::vec3(1));
		graphics.getLight()->visible = false;

		// make the board and game manager
		gameManager = GameManager(graphics, graphics.camera, glm::vec3(0, 0, 0));

		// set pointers
		gM = &gameManager;

		// set camera starting pos
		graphics.camera.setPos(glm::vec3(0.0f, 7 * 1.5f, 40.0f));

		// set callbacks
		glfwSetCursorPosCallback(graphics.window, mouse_callback_custom);
		glfwSetMouseButtonCallback(graphics.window, mouse_button_callback_custom);

		// add text
		// graphics.textManager.addText("This is sample text", "test", 15.0f, 15.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));

		// add test piece
		gameManager.board.addPiece(Piece::Color::BLUE, 0, 1, 0, 2);
		 gameManager.board.addPiece(Piece::Color::RED, 1, 2, 2, 2);

		enableFPSCounter = true;

		fpsCount = 0;
		fpsCounter = 0;

		gameState = 1;
	}

	int run() {
		// start timer
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

		// input

		// update the board and game (handles input too)
		gameManager.update();

		// std::cout << graphics.camera.yaw << " " << graphics.camera.pitch << std::endl;

		// render frame
		gameState = graphics.renderFrame();

		// end of timer sleep and normalize the clock
		std::chrono::system_clock::time_point after = std::chrono::system_clock::now();
		std::chrono::microseconds difference(std::chrono::time_point_cast<std::chrono::microseconds>(after) - std::chrono::time_point_cast<std::chrono::microseconds>(now));

		// count the fps
		int diffCount = difference.count();
		if (diffCount == 0) {
			diffCount = 1;
		}

		int sleepDuration = ((1000000 / fps * 1000) - diffCount) / 1000000;

		// output fps
		fpsCount += 1;
		fpsCounter += 1000000 / diffCount;

		if (fpsCount % int(fps) == 0) {
			if (enableFPSCounter) {
				std::cout << "\rFPS: " << fpsCounter / fpsCount;
			}
			fpsCount = 0;
			fpsCounter = 0;
		}

		if (sleepDuration < 0) {
			sleepDuration = 0;
		}

		// std::cout << sleepDuration << std::endl;
		Sleep(sleepDuration);

		return gameState;
	}
};

// clicking
void mouse_button_callback_custom(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		(*gM).leftClick();
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		(*gM).rightClick();
	}
}

// mouse movement
void mouse_callback_custom(GLFWwindow* window, double xpos, double ypos)
{
	(*gM).mouseUpdate(xpos, ypos);
}

#endif