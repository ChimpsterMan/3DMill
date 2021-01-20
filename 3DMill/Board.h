#ifndef BOARD_H
#define BOARD_H

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
#include "GraphicsEngine.h"
#include "Camera.h"
#include "Model.h"

// other pieces
#include "Piece.h"

// Networking
#include "Tools.h"

class Board {
public:
	GraphicsEngine *graphics = nullptr;
	Asset *asset = nullptr;

	//cube level (0 is outside, 2 is inside), x,y,z
	Piece data[3][3][3][3];

	glm::vec3 piecePosScalar = glm::vec3(12.6, 8.9, 12.6);
	glm::vec3 layerScalar = glm::vec3(4, 2.3, 4);
	glm::vec3 bottomLeft = glm::vec3(-12.6, -8, -12.6);

	Board() {
		graphics = nullptr;

		asset = new Asset(nullptr, glm::vec3(0), glm::vec3(0), glm::vec3(1));

		// do initial setup for board
		clearBoard();
	}

	Board(GraphicsEngine &graphics, glm::vec3 pos) {
		this->graphics = &graphics;

		asset = new Asset((*(this->graphics)).getModel("3dmill.obj"), pos, glm::vec3(0), glm::vec3(1));
		(*(this->graphics)).addAsset(asset);
		
		// do initial setup for board
		clearBoard();
	}

	// range (0-2) inclusive (integers)
	bool addPiece(Piece::Color color, int x, int y, int z, int c) {
		// check if another piece is already there
		if (data[c][x][y][z].type != Piece::Color::NONE) {
			return false;
		}

		// remove old asset
		if (graphics != nullptr)
		graphics->removeAsset(data[c][x][y][z].asset);
		// since the origin of the board is at the bottom center, we just find the bottom left corner and work relatively.
		glm::vec3 pos = getPiecePosFromCoord(x,y,z,c);
		data[c][x][y][z] = Piece(graphics, color, pos);

		return true;
	}

	// converts 0-2 cube coordinates that are relative to the board into real world space coordinates.
	glm::vec3 getPiecePosFromCoord(int x, int y, int z, int c) {
		return asset->position + (bottomLeft) + glm::vec3(x, y, z) * (glm::vec3(piecePosScalar) - layerScalar * float(c)) + layerScalar * float(c);
	}

	// finds the layer of the cube that a world position is.
	int findLayer(glm::vec3 pos) {
		// Work inside cube to outside cube
		for (int c = 2; c >= 0; c--) {
			glm::vec3 leftCorner = getPiecePosFromCoord(0, 0, 0, c);
			glm::vec3 rightCorner = getPiecePosFromCoord(2, 2, 2, c);
			if ((pos.x >= leftCorner.x && pos.y >= leftCorner.y && pos.z >= leftCorner.z) && (pos.x >= rightCorner.x && pos.y >= rightCorner.y && pos.z >= rightCorner.z)) {
				return c;
			}
		}

		//return not in board
		return -1;
	}

	void clearBoard() {
		for (int c = 0; c < 3; c++) {
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = 0; z < 3; z++) {
						if (graphics != nullptr) {
							graphics->removeAsset(data[c][x][y][z].asset);
						}
						// since the origin of the board is at the bottom center, we just find the bottom left corner and work relatively.
						glm::vec3 pos = getPiecePosFromCoord(x, y, z, c);
						data[c][x][y][z] = Piece(graphics, Piece::Color::NONE, pos);
					}
				}
			}
		}
	}

	bool fullBoard() {
		int pieces = 0;
		for (int c = 0; c < 3; c++) {
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = 0; z < 3; z++) {
						if (data[c][x][y][z].type != Piece::Color::NONE) {
							pieces += 1;
						}
					}
				}
			}
		}

		if (pieces == 3*3*3*3) {
			return true;
		}
		return false;
	}

	// utility
	// since the model's origin is at the bottom center, we just get the conversion rate of the y axis and multiply it by 1.5 since there are 4 plates
	glm::vec3 getCenter() {
		if (asset->model != nullptr) {
			return asset->position;
		}

		//return glm::vec3(0, piecePosScalar.y * 1.5, 0);

		return glm::vec3(0);
	}

	// sets all the board positons and current to whatever the datapacked says
	void setBoardToData(DataPacket *datapacket) {
		// std::cout << "set board to data" << std::endl;
		clearBoard();
		for (int c = 0; c < 3; c++) {
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = 0; z < 3; z++) {
						if (datapacket->board[x][y][z] == 0) {
							addPiece(Piece::Color::NONE, c, x, y, z);
						}
						if (datapacket->board[x][y][z] == 1) {
							addPiece(Piece::Color::RED, c, x, y, z);
						}
						if (datapacket->board[x][y][z] == 2) {
							addPiece(Piece::Color::BLUE, c, x, y, z);
						}
					}
				}
			}
		}
	}
};

#endif