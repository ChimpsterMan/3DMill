#ifndef GRAPHICSENGINE_H
#define GRAPHICSENGINE_H

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

#include <iostream>
#include <vector>

// graphics tools
#include "Camera.h"
#include "Light.h"
#include "Asset.h"
#include "Model.h"
#include "Mesh.h"
#include "Skybox.h"
#include "TextManager.h"

// prototypes
// callbacks
inline void framebuffer_size_callback(GLFWwindow* window, int width, int height);
inline void mouse_callback(GLFWwindow* window, double xpos, double ypos);
inline void window_focus_callback(GLFWwindow* window, int focused);
inline void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// tools
// mouse state, POV for point of view camera and controls, MOUSE for normal mouse movement detection and no camera effect.
enum MouseControlState { POV, MOUSE, CUSTOM };

// Pointer tools
// Window Clamp Mouse
inline bool *clampMousePointer;
inline MouseControlState *mouseModePointer;

inline Camera *cameraPointer;

// add all models before you start making assets
// a simple graphicsengine (uses multisampling x4)
class GraphicsEngine {
public:
	// normal vars
	GLFWwindow* window;

	Camera camera;

	Shader shader;
	Shader testCubeShader;

	// skybox
	Skybox skybox;

	// light
	Light light;

	// samples for multisampling
	int samples;

	const unsigned int *SCR_WIDTH;
	const unsigned int *SCR_HEIGHT;

	// list of active models
	std::vector<Model> models;

	// list of the physical models with all the transforms applied
	std::vector<Asset*> scene;

	// mouse modes
	MouseControlState mouseMode;

	bool clampMouse;
	bool pastClampMouse;

	// temp testing vars
	unsigned int VAO;
	unsigned int VBO;

	// text stuff
	TextManager textManager;

	GraphicsEngine() {

	}

	// takes in window display name, screen width, screen height, and number of samples per frame (1 is no multisampling). The last is if you want to have custom or preset control callbacks.
	// MULTISAMPLING TEXTURES DOES NOT WORK. MAKE SURE TO SET SAMPLES TO 1.
	GraphicsEngine(const char* windowName, const unsigned int *scr_WIDTH, const unsigned int *scr_HEIGHT, int samples, bool customCallback) {

		this->samples = samples;

		// window setup
		// glfw: initialize and configure
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		// multisampling
		if (samples > 1) {
			glfwWindowHint(GLFW_SAMPLES, samples);
		}

		SCR_WIDTH = scr_WIDTH;
		SCR_HEIGHT = scr_HEIGHT;

		// glfw window creation
		window = glfwCreateWindow(*scr_WIDTH, *scr_HEIGHT, windowName, NULL, NULL);
		if (window == NULL)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
		}
		glfwMakeContextCurrent(window);
		// make callbacks
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

		// if the callbacks are not custom
		if (!customCallback) {
			glfwSetCursorPosCallback(window, mouse_callback);
			glfwSetWindowFocusCallback(window, window_focus_callback);
			glfwSetMouseButtonCallback(window, mouse_button_callback);
		}

		// glad: load all OpenGL function pointers
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
		}

		// configure global opengl state
		glEnable(GL_DEPTH_TEST);
		stbi_set_flip_vertically_on_load(true);

		// Blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// multisampling
		if (samples > 1) {
			glEnable(GL_MULTISAMPLE);
		}

		// mouse normal callback 
		if (!customCallback) {
			// mouse control State default
			setMouseMode(MouseControlState::POV);

			clampMousePointer = &clampMouse;
			mouseModePointer = &mouseMode;
		}
		else {
			setMouseMode(MouseControlState::CUSTOM);
		}

		// text setup
		textManager = TextManager(*SCR_WIDTH, *SCR_HEIGHT);

		// Camera
		camera = Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 7*1.5, 20), true);
		cameraPointer = &camera;

		// Skybox setup
		skybox = Skybox("resources/shaders/sky_box.vs", "resources/shaders/sky_box.fs");

		// light setup
		light = Light("resources/shaders/light.vs", "resources/shaders/light.fs");

		// text cube setup
		testCubeShader = Shader("resources/shaders/cube.vs", "resources/shaders/cube.fs");
		generateTestCube();

		// local shader
		// shader = Shader("resources/shaders/basic_model.vs", "resources/shaders/basic_model.fs");
		shader = Shader("resources/shaders/lighted_model.vs", "resources/shaders/lighted_model.fs");
	}

	// switch Mouse Modes
	void setMouseMode(MouseControlState state) {
		if (state == MouseControlState::POV) {
			mouseMode = state;
			clampMouse = true;
		}
		else if (state == MouseControlState::MOUSE) {
			clampMouse = false;
			mouseMode = state;
		}
		else {
			mouseMode = state;
		}
	}

	// skybox
	void setSkybox(std::vector<const char*> faces) {
		skybox.loadSkyBox(faces);
	}

	Skybox* getSkybox() {
		return &skybox;
	}

	// color is 0-1 so white is (1,1,1)
	void setLight(glm::vec3 pos, glm::vec3 color) {
		light.pos = pos;
		light.color = color;
		light.enabled = true;
	}

	Light* getLight() {
		return &light;
	}

	// model functions
	Model *getModel(int index) {
		return &models[index];
	}

	Model *getModel(string str) {
		for (int i = 0; i < models.size(); i++) {
			if (models[i].name == str) {
				// std::cout << "Name found: " << models[i].name << " vs. " << str << std::endl;
				return &models[i];
			}
		}

		std::cout << "returned null when requesting: " << str << std::endl;
		return NULL;
	}

	void addModel(string const &path) {
		std::cout << "Added model at location: " << path << std::endl;
		models.push_back(Model(path, samples));
	}

	// asset stuff
	// add refrences to assets
	void addAsset(Asset *asset) {
		scene.push_back(asset);
	}

	// remove assets
	void removeAsset(Asset *asset) {
		for (int i = 0; i < scene.size(); i++) {
			if (scene[i] == asset) {
				scene.erase(scene.begin() + i);
				break;
			}
		}
	}

	// text stuff
	void addText(std::string text, std::string tag, float x, float y, float scale, glm::vec3 color) {
		textManager.addText(text, tag, x, y, scale, color);
	}
	
	void removeText(std::string tag) {
		textManager.removeText(tag);
	}

	void setText(std::string tag, std::string text) {
		textManager.setText(tag, text);
	}

	TextManager::Text* getText(std::string tag) {
		return textManager.getText(tag);
	}

	int renderFrame() {
		if (glfwWindowShouldClose(window)) {
			return 0;
		}

		// process input
		// optional camera input
		if (mouseMode != MouseControlState::CUSTOM) {
			processEscapeInput();
			camera.processInput(window);
		}

		// bind frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, *SCR_WIDTH, *SCR_HEIGHT);

		// clear the screen and start next frame
		glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw
		// skybox for background
		skybox.render(camera.projection, camera.update());

		// light
		light.render(camera.projection, camera.update());

		// testing stuff
		// generateTestCube();
		for (int i = 0; i < 10; i++) {
			// drawTestCube();
		}
		
		// model rendering
		shader.use();

		// if the light is valid then enter lighting mode for shader
		if (light.enabled) {
			shader.setBool("enableLighting", true);

			shader.setVec3("lightPos", light.pos);
			shader.setVec3("lightColor", light.color);
			shader.setFloat("lightBrightness", light.brightness);
			shader.setFloat("lightDistance", light.distance);
		}

		// draw assets with the corresponding model
		// draw backwards since the board is transparent and the balls and other objects need to be drawn first
		for (int i = scene.size()-1; i >= 0; i--) {
			if (scene[i]->visible) {
				// camera stuff
				glm::mat4 projection = camera.projection;
				glm::mat4 view = camera.update();
				shader.setMat4("projection", projection);
				shader.setMat4("view", view);
				shader.setVec3("viewPos", camera.pos);

				// translate model
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, scene[i]->position);
				model = glm::rotate(model, glm::radians(scene[i]->rotation.x), glm::vec3(1.0, 0.0, 0.0));
				model = glm::rotate(model, glm::radians(scene[i]->rotation.y), glm::vec3(0.0, 1.0, 0.0));
				model = glm::rotate(model, glm::radians(scene[i]->rotation.z), glm::vec3(0.0, 0.0, 1.0));
				model = glm::scale(model, scene[i]->scale);	// it's a bit too big for our scene, so scale it down
				shader.setMat4("model", model);

				// color change
				shader.setBool("overrideColorEnabled", scene[i]->overrideColorEnabled);
				if (scene[i]->overrideColorEnabled) {
					shader.setVec3("overrideColor", scene[i]->overrideColor);
				}

				// effects
				scene[i]->updateEffects(shader);

				if (scene[i]->model != nullptr) {
					scene[i]->model->Draw(shader, camera);
				}
			}
		}

		// render text elements
		textManager.render();

		glfwSwapBuffers(window);
		glfwPollEvents();

		return 1;
	}

private:
	// end opengl and free allocated resources
	void terminate() {
		glfwTerminate();
	}

	void generateTestCube() {
		float cube[] = {
			// back
			-0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,

			// front
			-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,

			// left
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,

			// right
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,

			 // bottom
			-0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,

			// top
			-0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
		};

		// init shaders
		shader = Shader("resources/shaders/cube.vs", "resources/shaders/cube.fs");

		// load vbo and make vao
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}

	void drawTestCube(glm::vec3 pos) {
		// draw it
		// use shader and set view and projection panes
		testCubeShader.use();
		testCubeShader.setMat4("projection", camera.projection);
		testCubeShader.setMat4("view", camera.update());

		// set coords and scale
		glm::mat4 cubeModel(1);
		cubeModel = glm::translate(cubeModel, pos);
		cubeModel = glm::scale(cubeModel, glm::vec3(1));
		testCubeShader.setMat4("model", cubeModel);

		// bind, draw, then reset bind to vao
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}

	// NONE CUSTOM INPUT CONTROL SECTION

	// releases clamp mouse if locked into the screen.
	void processEscapeInput() {
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			if (clampMouse) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				clampMouse = false;
			}
			else if (pastClampMouse == false) {
				glfwSetWindowShouldClose(window, true);
			}
		}
		else {
			pastClampMouse = clampMouse;
		}
	}
};

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// std::cout << "Failed to create GLFW window" << std::endl;
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on some displays

	glViewport(0, 0, width, height);
}

// focus callback
void window_focus_callback(GLFWwindow* window, int focused) {
	if (*mouseModePointer == MouseControlState::POV) {
		if (focused) {
			*clampMousePointer = true;
		}
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

// NONE CUSTOM MOUSE CALLBACKS
// clicking
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (*mouseModePointer == MouseControlState::POV) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			*clampMousePointer = true;
		}
	}
}

// mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (*mouseModePointer == MouseControlState::POV) {
		if (*clampMousePointer) {
			(*cameraPointer).mouseInputPOV(window, xpos, ypos);
		}
	}
}



#endif