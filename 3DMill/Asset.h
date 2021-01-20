#ifndef ASSET_H
#define ASSET_H

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
#include "Model.h"
#include "Mesh.h"

// the physical instance of the model with transforms
class Asset {
public:
	Model* model;

	bool visible;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	// Note: override disables lighting effects for the object
	bool overrideColorEnabled;
	glm::vec3 overrideColor;

	// effects
	struct Effect {
		string name;
		bool enabled = false;

		glm::vec3 color;
		float colorStrength = 0.5f;

		// speed in animation
		int speed = 20;

		// progress in animation of effect
		int frame = 0;
	};

	// make white gradient
	Effect gradient;

	Asset() {

	}

	Asset(glm::vec3 position) {
		this->model = nullptr;

		this->position = position;
		this->rotation = glm::vec3(0.0f);
		this->scale = glm::vec3(1.0f);

		overrideColorEnabled = false;

		gradient = Effect{"gradient", false, glm::vec3(1, 1, 1) };
	}

	Asset(Model *model) {
		this->model = model;

		visible = true;

		position = glm::vec3(0.0f);
		rotation = glm::vec3(0.0f);
		scale = glm::vec3(1.0f);

		overrideColorEnabled = false;

		gradient = Effect{ "gradient", false, glm::vec3(1, 1, 1) };
	}

	Asset(Model *model, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
		this->model = model;

		visible = true;

		this->position = position;
		this->rotation = rotation;
		this->scale = scale;

		overrideColorEnabled = false;

		gradient = Effect{ "gradient", false, glm::vec3(1, 1, 1) };
	}

	void setPosition(glm::vec3 position) {
		this->position = position;
	}

	void setRotation(glm::vec3 rotation) {
		this->rotation = rotation;

		if (this->rotation.x > 360) {
			this->rotation.x = this->rotation.x - 360;
		}
		if (this->rotation.x < 0) {
			this->rotation.x = 360 + this->rotation.x;
		}

		if (this->rotation.y > 360) {
			this->rotation.y = this->rotation.y - 360;
		}
		if (this->rotation.y < 0) {
			this->rotation.y = 360 + this->rotation.y;
		}

		if (this->rotation.z > 360) {
			this->rotation.z = this->rotation.z - 360;
		}
		if (this->rotation.z < 0) {
			this->rotation.z = 360 + this->rotation.z;
		}
	}

	void setScale(glm::vec3 scale) {
		this->scale = scale;
	}

	// Note: override disables lighting effects for the object
	void setOverrideColor(glm::vec3 color) {
		overrideColor = color;
		overrideColorEnabled = true;
	}

	// effect stuff
	void enableGradientEffect() {
		gradient.enabled = true;
	}

	void disableGradientEffect() {
		gradient.enabled = false;
	}

	void updateEffects(Shader &shader) {
		// set defaults
		shader.use();
		shader.setVec3("effectColor", glm::vec3(1));
		shader.setFloat("effectColorStrength", 0);

		if (gradient.enabled) {
			updateGradientEffect(shader);
		}
	}

	// returns output color of the gradient effect
	void updateGradientEffect(Shader &shader) {
		gradient.frame += gradient.speed;

		if (gradient.frame > 1000 || gradient.frame < 0) {
			gradient.speed *= -1;
			gradient.frame += gradient.speed;
		}

		// width of 100 rgb values
		float width = 200;
		// scale the output color
		glm::vec3 output = (gradient.color - float(((255 - width) / 2) / 255)) * float((width/255) * (float(gradient.frame) / 1000)) + float(((255 - width) / 2) / 255);
		// std::cout << to_string(output) << std::endl;

		shader.use();
		shader.setVec3("effectColor", output);
		shader.setFloat("effectColorStrength", gradient.colorStrength);
	}
};

#endif