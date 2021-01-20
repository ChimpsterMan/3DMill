// version 2 OpenGl GLFW camera class (updated for more abstraction)
#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>

#include <string>
#include <cmath>
#include <iostream>

class Camera {
public:
	// screen sizes
	const unsigned int *SCR_WIDTH;
	const unsigned int *SCR_HEIGHT;

	// camera calc values
	glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 Right;

	bool firstMouse;
	float yaw;
	float pitch;
	float lastX;
	float lastY;
	float fov;

	float nearPlane;
	float farPlane;

	// physics components
	glm::vec3 rot;
	glm::vec3 pos;
	glm::vec3 vel;
	float deceleration;

	// graphics components
	glm::mat4 view, projection;

	// control mouse sense and velocity
	float speedMultiplier;
	float senseMultiplier;

	float speedIncrement;
	float senseIncrement;

	float sensitivity;

	// blank camera
	Camera() {

	}

	// Main camera setup
	Camera(const unsigned int* SCR_WIDTH, const unsigned int* SCR_HEIGHT, glm::vec3 startPos, bool pov) {
		// set default variables
		firstMouse = true;
		yaw = 90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
		pitch = 0.0f;
		lastX = 1600.0f / 2.0;
		lastY = 900.0f / 2.0;
		fov = 45.0f;

		nearPlane = 0.1f;
		farPlane = 1000.0f;

		// pov control info
		speedMultiplier = 1;
		senseMultiplier = 1;

		speedIncrement = 1.1;
		senseIncrement = 1.1;

		sensitivity = 0.1f;


		// make identity matrix
		view = glm::mat4(1.0f);
		projection = glm::mat4(1.0f);

		// set variables
		this->SCR_WIDTH = SCR_WIDTH;
		this->SCR_HEIGHT = SCR_HEIGHT;

		// set camera type and variables
		projection = glm::perspective(glm::radians(fov), (float)*SCR_WIDTH / (float)*SCR_HEIGHT, nearPlane, farPlane);

		// set position and rotation
		pos = startPos;

		vel = glm::vec3(0, 0, 0);
		deceleration = 0.05;

		rot = glm::vec3(0, 0, 0);

		// do vector update
		updateCameraVectors();
	}

	// set position
	void setPos(glm::vec3 newPos) {
		pos = newPos;
	}

	// moves the cameras position based on translation vector
	void move(glm::vec3 translation) {
		pos.x += translation.x;
		pos.y += translation.y;
		pos.z += translation.z;
	}

	// updates the physics components of the camera
	void updatePhysics() {
		// update pos with velocity
		pos += vel;

		vel *= (1 - deceleration) * (1 - deceleration);

		if (vel.x < deceleration*0.6 && vel.x > -deceleration * 0.6 &&
			vel.y < deceleration*0.6 && vel.y > -deceleration * 0.6 &&
			vel.z < deceleration*0.6 && vel.z > -deceleration * 0.6) {

			vel = glm::vec3(0, 0, 0);
		}
	}

	glm::mat4 update() {
		// update vectors
		updateCameraVectors();

		view = glm::mat4(1.0f);
		view = glm::translate(view, pos);
		view = glm::lookAt(pos, pos + Front, Up);

		return view;
	}

	// allows for easy camera movement
	void updateCameraVectors()
	{
		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		// calculate the new Front vector
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		Front = glm::normalize(front);

		// also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	}

	// control rotation with mouse through GLFW window
	void mouseInputPOV(GLFWwindow* window, double xpos, double ypos) {
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
		lastX = xpos;
		lastY = ypos;

		// float sensitivity = 0.1f; // change this value to your liking
		xoffset *= sensitivity * senseMultiplier;
		yoffset *= sensitivity * senseMultiplier;
		yaw += xoffset;
		pitch += yoffset;
	}

	void lookAtTarget(glm::vec3 target) {
		double pi = 3.14159265358979;

		double dx = pos.x - target.x;
		double dy = pos.y - target.y;
		double dz = pos.z - target.z;

		yaw = atan2(dz, dx) - pi;
		pitch = -atan2(dy, sqrt(dx*dx + dz*dz));

		yaw *= 180 / pi;
		pitch *= 180 / pi;

		updateCameraVectors();
	}

	// default input control
	// process all input: ask GLFW whether relevant keys are pressed/released this frame and react accordingly
	void processInput(GLFWwindow *window)
	{

		float accel = 0.02 * speedMultiplier;
		deceleration = 0.01 * speedMultiplier;

		// speed controls
		// up
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			speedMultiplier *= speedIncrement;
		}
		// down
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			speedMultiplier /= speedIncrement;
		}

		// mouse controls
		// right
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			senseMultiplier *= senseIncrement;
		}

		// left
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			senseMultiplier /= senseIncrement;
		}

		// camera controls
		// W
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			vel += accel * Front;
		}
		// A
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			vel -= accel * Right;
		}
		// S
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			vel -= accel * Front;
		}
		// D
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			vel += accel * Right;
		}

		// SPACE
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			vel += accel * glm::vec3(0.0f, 1.0f, 0.0f);
		}
		// CTRL
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
			vel -= accel * glm::vec3(0.0f, 1.0f, 0.0f);
		}

		updatePhysics();
	}
};

#endif
