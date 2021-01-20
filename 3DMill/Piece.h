#ifndef PIECE_H
#define PIECE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <shader.h>

#include <iostream>

// graphics tools
#include "GraphicsEngine.h"
#include "Camera.h"
#include "Asset.h"

// if the Piece has type NONE then it is an empty space.
class Piece {
public:
	GraphicsEngine *graphics;
	Asset *asset;

	float colliderRadius = 0.75f;

	// whether the piece is blue or red
	enum Color { NONE = -1, OUTLINE = 0, RED = 1, BLUE = 2 };
	Color type;

	Piece() {
		asset = nullptr;
		type = Color::NONE;
	}

	Piece(GraphicsEngine *graphics, Color type, glm::vec3 pos) {
		this->graphics = graphics;
		this->type = type;

		// if the graphics is not enabled
		if (graphics != nullptr) {
			// make models based on color, set the position, and set the pointer to the model.
			if (this->type == Color::NONE) {
				asset = new Asset(pos);
			}
			else if (this->type == Color::RED) {
				asset = new Asset((*graphics).getModel("redball.obj"), pos, glm::vec3(0), glm::vec3(1));
				// std::cout << "red" << std::endl;
			}
			else if (this->type == Color::BLUE) {
				asset = new Asset((*graphics).getModel("blueball.obj"), pos, glm::vec3(0), glm::vec3(1));
				// std::cout << "blue" << std::endl;
			}
			else if (this->type == Color::OUTLINE) {
				asset = new Asset((*graphics).getModel("outlineball.obj"), pos, glm::vec3(0), glm::vec3(1));
				// std::cout << "blue" << std::endl;
			}

			(*graphics).addAsset(asset);
		}
		else {
			asset = new Asset(pos);
		}
	}
};

#endif