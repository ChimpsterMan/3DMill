#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <shader.h>

#include <chrono>
#include <thread>
#include <future>
#include <iostream>

#include "TextManager.h"

// graphics tools
#include "GraphicsEngine.h"
#include "Camera.h"
#include "Model.h"
#include "Board.h"
#include "Piece.h"

// prototypes

// manages the graphics, controls, and game data of 3d Four Connect
class GameManager {
public:
	// callbacks
	void(*winCallback)(Piece::Color) = nullptr;
	void(*placePieceCallback)(Piece::Color, glm::vec4) = nullptr;
	void(*clearBoardCallback)() = nullptr;
	void(*outlinePieceMoveCallback) (bool, glm::vec3) = nullptr;

	// options
	// will prevent placing a piece (1 is red, 2 is blue) unless the int is set to zero in which both moves can be done.
	int placeOnlyOnTurn;

	GraphicsEngine *graphics = nullptr;
	Camera *camera = nullptr;

	Board board;

	// all score texts will use the tagformat "score#"
	int score1;
	int score2;

	// number of pieces left for each player
	int piecesLeft1;
	int piecesLeft2;

	// flags
	// the pause after somebody wins where everyone looks at the board
	bool winPause;

	// mouse stuff
	bool rightClickStatus;
	bool leftClickStatus;
	glm::vec2 mousePos;

	float camFollowDistance;

	// Per-Turn Status Vars
	Piece::Color currentTurn;

	// If the player needs to select two positions, the first selection is shoved into here
	glm::vec4 selectedPieceBuffer;

	// The number of mills achieved by a person on their turn (how many pieces of their opponent they get to remove)
	int mills;

	glm::vec3 mouseRay;

	// -1 means not selecting anything
	glm::vec4 selectedPiece;

	enum Stage { TESTING, SETUP, PLAY, DATA, END };
	Stage stage;

	Piece previewPiece;
	Piece outlinePiece;

	// for multiplayer only
	Piece opponentPiece;

	// testing
	Piece testPiece;

	// neutral game manager that is meant to mainly handle sheer events and status of the board rather than graphics and controls
	// optimal for a server storing data
	GameManager() {
		board = Board();

		placeOnlyOnTurn = 0;
		currentTurn = Piece::Color::RED;

		score1 = 0;
		score2 = 0;

		piecesLeft1 = 23;
		piecesLeft2 = 23;

		selectedPiece = glm::vec4(-1);
		selectedPieceBuffer = glm::vec4(-1);
		mills = 0;

		stage = Stage::DATA;
	}

	GameManager(GraphicsEngine &graphics, Camera &camera, glm::vec3 pos) {
		this->graphics = &graphics;
		this->camera = &camera;

		// default values init
		camFollowDistance = 55.0f;

		placeOnlyOnTurn = 0;

		score1 = 0;
		score2 = 0;

		piecesLeft1 = 23;
		piecesLeft2 = 23;

		winPause = false;

		rightClickStatus = false;
		leftClickStatus = false;

		mousePos = glm::vec2(0);

		selectedPiece = glm::vec4(-1);
		selectedPieceBuffer = glm::vec4(-1);
		mills = 0;

		// construct the game setup
		board = Board(graphics, pos);

		currentTurn = Piece::Color::RED;

		stage = Stage::PLAY;

		// play setup
		if (stage == Stage::PLAY) {
			// setup text
			// score
			this->graphics->addText("Player 1: " + to_string(score1), "score1", 1, 95, 1.0f, glm::vec3(0.75,0.1,0.1));
			this->graphics->addText("Player 2: " + to_string(score2), "score2", 1, 90, 1.0f, glm::vec3(0.1,0.1,0.75));

			// pieces left
			this->graphics->addText("Reserve Pieces: " + to_string(piecesLeft1), "piecesLeft1", 20, 95, 1.0f, glm::vec3(0.75, 0.1, 0.1));
			this->graphics->addText("Reserve Pieces: " + to_string(piecesLeft2), "piecesLeft2", 20, 90, 1.0f, glm::vec3(0.1, 0.1, 0.75));
		}

		// testing setup
		if (stage == Stage::TESTING) {
			testPiece = Piece(&graphics, Piece::RED, glm::vec3(0));
		}
	}

	void update() {
		// std::cout << (*graphics).scene.size() << std::endl;

		// non-graphical data based game
		if (stage == Stage::DATA) {
			// check win case
			Piece::Color win = checkWin();
			if (win != Piece::Color::NONE) {
				string text;

				cout << endl;
				if (win == Piece::Color::RED) {
					// cout << "RED WINS!" << endl;
					text = "Red Wins!";

					score1 += 1;
				}
				else if (win == Piece::Color::BLUE) {
					// cout << "BLUE WINS!" << endl;
					text = "Blue Wins!";

					score2 += 1;
				}
				// nobody wins
				else if (board.fullBoard()) {
					// cout << "NOBODY WINS" << endl;
					text = "Nobody Wins!";
				}

				// graphics->textManager.addText(text, 25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
				if (winCallback != nullptr) {
					winCallback(win);
				}

				board.clearBoard();
			}
		}

		// main game info
		if (stage == Stage::PLAY) {
			// basically if not multiplayer online then keep the preview color the same
			if (placeOnlyOnTurn == 0) {
				// if the preview piece is not set then make it
				if (previewPiece.type != currentTurn) {
					graphics->removeAsset(previewPiece.asset);
					previewPiece = Piece(graphics, currentTurn, glm::vec3(0));
				}

				// if the outline piece is not set then make it
				if (outlinePiece.type != currentTurn) {
					graphics->removeAsset(outlinePiece.asset);
					outlinePiece = Piece(graphics, currentTurn, glm::vec3(0));
					outlinePiece.asset->gradient.enabled = true;
					outlinePiece.asset->visible = false;
				}

				// Manage outline piece
				// change the strength of gradient on the outline piece if the win pause is active
				if (winPause) {
					outlinePiece.asset->gradient.colorStrength = 1.0f;
					outlinePiece.asset->setOverrideColor(glm::vec3(1));
				}
				else {
					outlinePiece.asset->gradient.colorStrength = 0.5f;
					outlinePiece.asset->overrideColorEnabled = false;
				}
			}
			else {
				// set if not set
				if (previewPiece.type != placeOnlyOnTurn) {
					graphics->removeAsset(previewPiece.asset);
					previewPiece = Piece(graphics, Piece::Color(placeOnlyOnTurn), glm::vec3(0));
				}

				// outline piece
				if (outlinePiece.type != placeOnlyOnTurn && !winPause) {
					graphics->removeAsset(outlinePiece.asset);
					outlinePiece = Piece(graphics, Piece::Color(placeOnlyOnTurn), glm::vec3(0));
					outlinePiece.asset->gradient.enabled = true;
					outlinePiece.asset->visible = false;
				}

				// setup opponents marker if they are not setup already
				if (opponentPiece.asset == nullptr) {
					graphics->removeAsset(opponentPiece.asset);
					opponentPiece = Piece(graphics, Piece::Color(((placeOnlyOnTurn + 0) % 2) + 1), glm::vec3(0));
					opponentPiece.asset->gradient.enabled = true;
					opponentPiece.asset->visible = false;
				}

				// change the strength of gradient on the outline piece if it is not their turn
				if (currentTurn != placeOnlyOnTurn || winPause) {
					outlinePiece.asset->gradient.colorStrength = 1.0f;
					outlinePiece.asset->setOverrideColor(glm::vec3(1));
				}
				else {
					outlinePiece.asset->gradient.colorStrength = 0.5f;
					outlinePiece.asset->overrideColorEnabled = false;
				}
			}

			// check key input
			checkGameInput();

			// make cam look at target and stay locked at a set distance
			(*camera).setPos(board.getCenter() + normalize((*camera).pos - board.getCenter()) * camFollowDistance);
			(*camera).lookAtTarget(board.getCenter());

			// update vectors
			(*camera).updateCameraVectors();

			// update mouse ray
			updateMouseRay();

			// store the current state of outline piece so we don't send status of it every single frame to server
			bool tempBool = outlinePiece.asset->visible;
			glm::vec3 tempVec3 = outlinePiece.asset->position;

			// SELECT PIECES
			// DO this when you get back
			// fix mill selection
			// If a piece slot on the board is selected
			if (selectedPiece != glm::vec4(-1)) {
				// var to stop processing future actions of the current loop
				bool restrictAction = false;

				// if multiplayer is not enabled or if the multiplayer assigned turn is equal to the current turn
				if (!restrictAction && (placeOnlyOnTurn == 0 || placeOnlyOnTurn == currentTurn)) {
					// unselect selected pieces
					if (selectedPiece == selectedPieceBuffer && selectedPiece != glm::vec4(-1) && leftClickStatus && !winPause) {
						board.getPiece(selectedPiece).asset->disableGradientEffect();
						selectedPieceBuffer = glm::vec4(-1);
					}

					// if the selected slot is your color (ignore if unselecting)
					else if (board.getPiece(selectedPiece).type == currentTurn) {
						// choose piece to move
						if (selectedPieceBuffer == glm::vec4(-1)) {
							// check click
							if (!winPause && leftClickStatus) {
								board.getPiece(selectedPiece).asset->enableGradientEffect();

								selectedPieceBuffer = selectedPiece;
							}
						}
					}

					// if the selected slot is your opponents color (if red make it blue, if blue make it red)
					// and if there are mills left and the opponents piece is not already part of a mill
					else if (board.getPiece(selectedPiece).type == currentTurn % 2 + 1 && mills > 0 && checkMill(selectedPiece) == 0) {
						// check click
						if (!winPause && leftClickStatus) {
							board.removePiece(selectedPiece);

							mills--;
							restrictAction = true;

							if (mills == 0) {
								switchTurn();
							}
							// if there are simply no more pieces to remove that are not in mills
							else if (getNonMillPieces(Piece::Color(currentTurn % 2 + 1)) == 0) {
								mills = 0;
								switchTurn();
							}

							// update remove across clients
							if (placePieceCallback != nullptr) {
								// std::cout << "called callback" << std::endl;
								placePieceCallback(board.data[(int)selectedPiece.w][(int)selectedPiece.x][(int)selectedPiece.y][(int)selectedPiece.z].type, selectedPiece);
							}
						}
					}
				}

				// if an unfilled slot and the player does not currently have to set mills (priorety)
				if (!restrictAction && board.getPiece(selectedPiece).type == Piece::Color::NONE && mills == 0) {
					// set outline piece location and visibility
					// you can only see outline if it is a possible place to move

					// if the person is trying to move a piece and it is in an invalid place then don't outline
					if (!(!winPause && (currentTurn == placeOnlyOnTurn || placeOnlyOnTurn == 0) && selectedPieceBuffer != glm::vec4(-1) && 
						(!(validMoveLocation(selectedPieceBuffer, selectedPiece) || getPiecesOnBoard(currentTurn) + *getPiecesLeftFromTurn(currentTurn) <= 3)))) {
						outlinePiece.asset->setPosition(board.getPiecePosFromCoord((int)selectedPiece.x, (int)selectedPiece.y, (int)selectedPiece.z, (int)selectedPiece.w));
						outlinePiece.asset->visible = true;
					}

					// check for right click or left click events to set piece (does not activate when win pause activates).
					if (!winPause && leftClickStatus && (currentTurn == placeOnlyOnTurn || placeOnlyOnTurn == 0)) {
						bool placedPiece = false;

						// if the person is trying to move a piece
						if (selectedPieceBuffer != glm::vec4(-1)) {
							// check if the end location is in a valid move position
							// override this valid move location if the player only has 3 pieces left on the field and their reserves.
							if (validMoveLocation(selectedPieceBuffer, selectedPiece) || getPiecesOnBoard(currentTurn) + *getPiecesLeftFromTurn(currentTurn) <= 3) {
								board.addPiece(currentTurn, (int)selectedPiece.x, (int)selectedPiece.y, (int)selectedPiece.z, (int)selectedPiece.w);
								placedPiece = true;

								// deactivate effect
								board.getPiece(selectedPieceBuffer).asset->gradient.enabled = false;

								// remove old piece
								board.removePiece(selectedPieceBuffer);

								//selectedPieceBuffer = glm::vec4(-1);
							}
						}
						// normal placing piece
						else {
							// get pieces left in pocket
							int* piecesLeft = nullptr;
							piecesLeft = getPiecesLeftFromTurn(currentTurn);

							if (*piecesLeft > 0) {
								board.addPiece(currentTurn, (int)selectedPiece.x, (int)selectedPiece.y, (int)selectedPiece.z, (int)selectedPiece.w);
								placedPiece = true;
							}
							else {
								std::cout << "Alert: You are out of reserve pieces and can no longer put any on the board. You may only move existing pieces." << std::endl;
							}
						}

						if (placedPiece) {
							// update text for pieces left
							// if not moving a piece (placing new piece)
							if (selectedPieceBuffer == glm::vec4(-1)) {
								int *var = nullptr;
								var = getPiecesLeftFromTurn(currentTurn);

								*var -= 1;

								// std::cout << "piecesLeft" + to_string(currentTurn) << std::endl;
								graphics->setText("piecesLeft" + to_string(currentTurn), "Reserve Pieces: " + to_string(*var));
							}

							// test for mills and apply them
							if (board.getPiece(selectedPiece).type != Piece::Color::NONE) {
								mills = checkMill(selectedPiece);
							}

							// switch the turn if the current player does not have mills to use.
							if (mills == 0) {
								switchTurn();
							}

							// if there are simply no more pieces to remove that are not in mills
							else if (getNonMillPieces(Piece::Color(currentTurn % 2 + 1)) == 0) {
								mills = 0;
								switchTurn();
							}

							// std::cout << "placed piece" << std::endl;
							// callback
							if (placePieceCallback != nullptr) {
								// std::cout << "called callback" << std::endl;
								placePieceCallback(board.data[(int)selectedPiece.w][(int)selectedPiece.x][(int)selectedPiece.y][(int)selectedPiece.z].type, selectedPiece);
							}
						}
					}
				}
			}
			else {
				outlinePiece.asset->visible = false;
			}

			// check if the outline piece changed at all and if so, then activate callback
			if (tempBool != outlinePiece.asset->visible || tempVec3 != outlinePiece.asset->position) {
				if (placePieceCallback != nullptr) {
					outlinePieceMoveCallback(outlinePiece.asset->visible, outlinePiece.asset->position);
				}
			}

			// bind and rotate preview piece
			bindPreviewPiece();

			// check win case
			Piece::Color win = checkWin();
			if (win != Piece::Color::NONE && !winPause){
				string text;

				cout << endl;
				if (win == Piece::Color::RED) {
					cout << "RED WINS!" << endl;
					text = "Red Wins!";

					score1 += 1;
					graphics->setText("score1", "Player 1: " + to_string(score1));
				}
				else if (win == Piece::Color::BLUE) {
					cout << "BLUE WINS!" << endl;
					text = "Blue Wins!";

					score2 += 1;
					graphics->setText("score2", "Player 2: " + to_string(score2));
				}
				// nobody wins
				else if (board.fullBoard()) {
					cout << "NOBODY WINS" << endl;
					text = "Nobody Wins!";
				}

				text = text + " (Press Enter to Continue)";
				graphics->textManager.addText(text, "win_msg", 26.0f, 70.0f, 1.0f, glm::vec3(0));

				winPause = true;
				
				if (winCallback != nullptr) {
					// std::cout << "callback call" << std::endl;
					winCallback(win);
				}
			}
		}

		if (stage == Stage::TESTING) {
			(*camera).processInput((*graphics).window);
			(*camera).lookAtTarget(board.getCenter());
			testPiece.asset->setPosition((*camera).pos + mouseRay * 20.0f);
		}

		// reset mouse buttons
		rightClickStatus = false;
		leftClickStatus = false;
	}

	void checkGameInput() {
		float accel = 0.02;
		(*camera).deceleration = 0.01;

		// camera controls
		// up
		if (glfwGetKey((*graphics).window, GLFW_KEY_UP) | glfwGetKey((*graphics).window, GLFW_KEY_W) == GLFW_PRESS) {
			(*camera).vel += accel * (*camera).Up;
		}
		// down
		if (glfwGetKey((*graphics).window, GLFW_KEY_DOWN) | glfwGetKey((*graphics).window, GLFW_KEY_S) == GLFW_PRESS) {
			(*camera).vel -= accel * (*camera).Up;
		}
		// right
		if (glfwGetKey((*graphics).window, GLFW_KEY_RIGHT) | glfwGetKey((*graphics).window, GLFW_KEY_D) == GLFW_PRESS) {
			(*camera).vel += accel * (*camera).Right;
		}
		// left
		if (glfwGetKey((*graphics).window, GLFW_KEY_LEFT) | glfwGetKey((*graphics).window, GLFW_KEY_A) == GLFW_PRESS) {
			(*camera).vel -= accel * (*camera).Right;
		}

		// move on to next round if the game is in the win-pause stage
		if (winPause) {
			if (glfwGetKey((*graphics).window, GLFW_KEY_ENTER) | glfwGetKey((*graphics).window, GLFW_KEY_ENTER) == GLFW_PRESS) {
				winPause = false;

				// remove the banner
				graphics->textManager.removeText("win_msg");

				// reset
				board.clearBoard();

				piecesLeft1 = 23;
				piecesLeft2 = 23;

				selectedPiece = glm::vec4(-1);
				selectedPieceBuffer = glm::vec4(-1);
				mills = 0;

				if (clearBoardCallback != nullptr) {
					clearBoardCallback();
				}
			}
		}

		// update physics
		(*camera).updatePhysics();
	}

	bool validMoveLocation(glm::vec4 initial, glm::vec4 finalLocation) {
		std::vector<glm::vec4> valid = std::vector<glm::vec4>();

		// move along the cube
		for (int x = -1; x < 2; x++) {
			for (int y = -1; y < 2; y++) {
				for (int z = -1; z < 2; z++) {
					if (initial.x + x >= 0 && initial.x + x <= 2 && initial.y + y >= 0 && initial.y + y <= 2 && initial.z + z >= 0 && initial.z + z <= 2) {
						int oneCount = 0;
						if (initial.x + x == 1)
							oneCount++;
						if (initial.y + y == 1)
							oneCount++;
						if (initial.z + z == 1)
							oneCount++;

						// if not part of the centers (edge or corner) and if it is not the original position
						glm::vec4 temp = glm::vec4(initial.x + x, initial.y + y, initial.z + z, initial.w);
						if (oneCount <= 1 && temp != initial) {
							// they need to share atleast two axis
							int twoCount = 0;
							if (temp.x == initial.x)
								twoCount++;
							if (temp.y == initial.y)
								twoCount++;
							if (temp.z == initial.z)
								twoCount++;

							if (twoCount == 2) {
								valid.push_back(temp);
							}
						}
					}
				}
			}
		}

		// handle across multiple cube levels
		for (int w = -1; w <= 1; w += 2) {
			// and not travelling across corners
			if (initial.w + w >= 0 && initial.w + w <= 2 && (initial.x == 1 || initial.y == 1 || initial.z == 1)) {
				valid.push_back(glm::vec4(initial.x, initial.y, initial.z, initial.w + w));
			}
		}

		for (int i = 0; i < valid.size(); i++) {
			if (valid[i] == finalLocation) {
				return true;
			}
		}

		return false;
	}

	// get the current number of pieces with a certain color on the board
	int getPiecesOnBoard(Piece::Color color) {
		//traverse the entire board searching for a red piece
		int count = 0;

		for (int c = 0; c < 3; c++) {
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = 0; z < 3; z++) {
						//if a valid position (not in center of cubes)
						if (board.data[c][x][y][z].type == color) {
							count++;
						}
					}
				}
			}
		}

		return count;
	}

	// check if somebody won
	Piece::Color checkWin() {
		// Check if Blue Wins
		if (piecesLeft1 + getPiecesOnBoard(Piece::Color::RED) < 3) {
			return Piece::Color::BLUE;
		}
	
		// Check if Red Wins
		if (piecesLeft2 + getPiecesOnBoard(Piece::Color::BLUE) < 3) {
			return Piece::Color::RED;
		}

		// default
		return Piece::Color::NONE;
	}

	// Return the number of mills a piece is connected to. Enter a piece pos (x,y,z,c) and the function will return if the piece selected is part of a mill (3 pieces of the same color in a row)
	int checkMill(glm::vec4 pos) {
		int millsCounted = 0;

		if (board.getPiece(pos).type != Piece::Color::NONE | Piece::Color::EMPTY) {
			// check each cube layer for horizontal or vertical mills
			for (int c = 0; c < 3; c++) {
				// check x lines
				for (int y = 0; y < 3; y += 2) {
					for (int z = 0; z < 3; z += 2) {
						// goal of getting this to equal 3
						int count = 0;
						bool partOfMill = false;

						for (int x = 0; x < 3; x++) {
							// if the piece pos inputed has the same type as the current searching index
							if (board.getPiece(pos).type == board.data[c][x][y][z].type) {
								count++;
							}

							// and is a part of the mill
							if (pos == glm::vec4(x, y, z, c)) {
								partOfMill = true;
							}
						}

						if (count >= 3 && partOfMill) {
							millsCounted++;
						}
					}
				}

				// check y lines
				for (int x = 0; x < 3; x += 2) {
					for (int z = 0; z < 3; z += 2) {
						// goal of getting this to equal 3
						int count = 0;
						bool partOfMill = false;

						for (int y = 0; y < 3; y++) {
							// if the piece pos inputed has the same type as the current searching index
							if (board.getPiece(pos).type == board.data[c][x][y][z].type) {
								count++;
							}

							// and is a part of the mill
							if (pos == glm::vec4(x, y, z, c)) {
								partOfMill = true;
							}
						}

						if (count >= 3 && partOfMill) {
							millsCounted++;
						}
					}
				}

				// check z lines
				for (int x = 0; x < 3; x += 2) {
					for (int y = 0; y < 3; y += 2) {
						// goal of getting this to equal 3
						int count = 0;
						bool partOfMill = false;

						for (int z = 0; z < 3; z++) {
							//if the piece pos inputed has the same type as the current searching index
							if (board.getPiece(pos).type == board.data[c][x][y][z].type) {
								count++;
							}

							// and is a part of the mill
							if (pos == glm::vec4(x, y, z, c)) {
								partOfMill = true;
							}
						}

						if (count >= 3 && partOfMill) {
							millsCounted++;
						}
					}
				}
			}

			// check for diagonal mills
			// Two Rules must be followed to make sure that only center edge pieces are found
			// 1. There can be no more than one 1 in xyz
			// 2. There must be one 1 in xyz
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = 0; z < 3; z++) {
						int oneCount = 0;
						if (x == 1)
							oneCount++;
						if (y == 1)
							oneCount++;
						if (z == 1)
							oneCount++;

						if (oneCount == 1) {
							// yay, everything in here is a center edge position

							int count = 0;
							bool partOfMill = false;

							for (int c = 0; c < 3; c++) {
								// if the piece pos inputed has the same type as the current searching index
								if (board.getPiece(pos).type == board.data[c][x][y][z].type) {
									count++;
								}

								// and is a part of the mill
								if (pos == glm::vec4(x, y, z, c)) {
									partOfMill = true;
								}
							}

							if (count >= 3 && partOfMill) {
								millsCounted++;
							}
						}
					}
				}
			}
		}


		if (millsCounted > 0) {
			// std::cout << "mills found: " << millsCounted << std::endl;
		}

		return millsCounted;
	}

	// returns the number of pieces that are not part of mills with a corresponding color
	int getNonMillPieces(Piece::Color color) {
		std::vector<glm::vec4> pieces = std::vector<glm::vec4>();

		// find pieces with the same color
		for (int c = 0; c < 3; c++) {
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = 0; z < 3; z++) {
						if (board.data[c][x][y][z].type == color) {
							pieces.push_back(glm::vec4(x, y, z, c));
						}
					}
				}
			}
		}

		int piecesInMill = 0;

		for (int i = 0; i < pieces.size(); i++) {
			piecesInMill += checkMill(pieces[i]);
		}

		// std::cout << "non-mill pieces found: " << pieces.size() - piecesInMill << std::endl;
		return pieces.size() - piecesInMill;
	}

	void switchTurn() {
		if (currentTurn == Piece::Color::BLUE) {
			currentTurn = Piece::Color::RED;
		}
		else if (currentTurn == Piece::Color::RED) {
			currentTurn = Piece::Color::BLUE;
		}

		// reset turn vars
		selectedPieceBuffer = glm::vec4(-1);
		mills = 0;
	}

	void setTurnToInt(int num) {
		if (num == Piece::Color::RED) {
			currentTurn = Piece::Color::RED;
		}
		else if (num == Piece::Color::BLUE) {
			currentTurn = Piece::Color::BLUE;
		}

		// reset turn vars
		selectedPieceBuffer = glm::vec4(-1);
		//mills = 0;
	}

	void setScores(int score1, int score2) {
		this->score1 = score1;
		this->score2 = score2;
	}

	void setPiecesLeft(int piecesLeft1, int piecesLeft2) {
		this->piecesLeft1 = piecesLeft1;
		this->piecesLeft2 = piecesLeft2;

		if (graphics != nullptr) {
			graphics->setText("piecesLeft1", "Reserve Pieces: " + to_string(this->piecesLeft1));
			graphics->setText("piecesLeft2", "Reserve Pieces: " + to_string(this->piecesLeft2));
		}
	}

	int* getPiecesLeftFromTurn(Piece::Color color) {
		switch (color) {
		case 1:
			return &piecesLeft1;
			break;
		case 2:
			return &piecesLeft2;
			break;
		}
	}

	void leftClick() {
		leftClickStatus = true;
	}

	void rightClick() {
		rightClickStatus = true;
	}

	// set the opponent piece visibility and pos (pos is board coordinated not world
	void setOpponentOutlinePiece(bool visible, glm::vec3 pos) {
		opponentPiece.asset->visible = visible;

		if (visible) {
			opponentPiece.asset->setPosition(pos);
		}
	}
	
	// returns int positions of where the pieces are if they are selected
	glm::vec4 checkSelectPiece() {
		float closestLength = -1;
		glm::vec4 closestPos = glm::vec4(-1);
		for (int c = 0; c < 3; c++) {
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = 0; z < 3; z++) {
						//if a valid position (not in center of cubes)
						if (board.data[c][x][y][z].type != Piece::Color::EMPTY) {
							// if the piece is not filled already and is being intersected by the line get the distance between the line and the piece
							float length = checkLinePieceIntersection(board.data[c][x][y][z], mouseRay);

							// if a piece is selected (not -1) then add it if it is either the closest or the first selected piece.
							if (length != -1 && (closestLength == -1 || length < closestLength)) {
								closestLength = length;
								closestPos = glm::vec4(x, y, z, c);
							}
						}
					}
				}
			}
		}

		return closestPos;
	}

	// find the distance between the piece targeted and the camera. Then multiply this by the normalized vector of the line from the camera position and compare if they are close enough.
	// returns the distance
	float checkLinePieceIntersection(Piece piece, glm::vec3 line) {
		float dist = glm::length(camera->pos - piece.asset->position);
		glm::vec3 linePoint = line * dist + camera->pos;

		float vecLength = glm::length(linePoint - piece.asset->position);
		if (vecLength < piece.colliderRadius) {
			return vecLength;
		}
		return -1;
	}

	// update mouse pos
	void mouseUpdate(double x, double y) {
		mousePos = glm::vec2(x, y);

		selectedPiece = checkSelectPiece();
	}

	// find the vector ray where the mouse is looking
	void updateMouseRay() {
		// calculate the vector displacement of the cursor on the screen from the cameras perspective
		float nearPlaneWidth = (tan((16.0f / 9.0f) * (*camera).fov * (3.1415926535 / 180)) / (*camera).nearPlane) * 2;
		float nearPlaneHeight = (tan((*camera).fov * (3.1415926535 / 180)) / (*camera).nearPlane) * 2;

		glm::vec3 dist = 1 * (*camera).nearPlane * (*camera).Front;
		glm::vec3 xOffset = (*camera).Right * float(mousePos.x * (nearPlaneWidth / *(*graphics).SCR_WIDTH) - (nearPlaneWidth / 2));
		glm::vec3 yOffset = -(*camera).Up * float(mousePos.y * (nearPlaneHeight / *(*graphics).SCR_HEIGHT) - (nearPlaneHeight / 2));

		// these numbers are scaling values to compensate for the oblong window and the clipping pane distance
		glm::vec3 scaler = glm::vec3((1 / 7.7) / 100, (1 / 2.425) / 100, 1);
		mouseRay = glm::normalize(dist*scaler.z + xOffset * scaler.x + yOffset * scaler.y);

		// std::cout << float(x / *(*graphics).SCR_WIDTH) << std::endl;
		// printVector((*camera).pos + mouseRay * 20.0f);
		// printVector(mouseRay);
		// printVector((*graphics).getModel(0).position - (*camera).pos);
	}

	void bindPreviewPiece() {
		// update position
		glm::vec3 displacement = glm::vec3(9.0f, 4.5f, 15.0f);

		previewPiece.asset->setPosition((*camera).pos + displacement.z * (*camera).Front + displacement.x * (*camera).Right + displacement.y * (*camera).Up);
		// printVector((*camera).pos);

		// update rotation
		glm::vec3 rotationDisplacement = glm::vec3(1.0f, 2.0f, 3.0f);
		previewPiece.asset->setRotation(previewPiece.asset->rotation + rotationDisplacement);
	}

	// set callbacks
	void setWinCallback(void f (Piece::Color)) {
		winCallback = f;
	}

	void setPiecePlaceCallback(void f (Piece::Color, glm::vec4)) {
		placePieceCallback = f;
	}

	void setClearBoardCallback(void f ()) {
		clearBoardCallback = f;
	}

	// visible, position
	void setOutlinePieceMoveCallback(void f(bool, glm::vec3)) {
		outlinePieceMoveCallback = f;
	}

	// utility
	void printVector(glm::vec3 vec) {
		std::cout << vec.x << " " << vec.y << " " << vec.z << std::endl;
	}
};

#endif