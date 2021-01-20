#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <img/stb_image.h>
//idk but manually importing fixes the problem
//#include <img/ImageLoader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <shader.h>

#include "Camera.h"

#include <vector>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Skybox {
public:
	Shader shader;

	//order
	//right
	//left
	//top
	//bottom
	//front
	//back
	std::vector<const char*> textureFaces;

	unsigned int textureID, VBO, VAO;

	int width, height, nrChannels;

	//only enable if textures are loaded for the skybox
	bool enabled;

	Skybox() {
		enabled = false;
	}

	Skybox(const char* vs, const char* fs) {
		shader = Shader(vs, fs);

		enabled = false;
	}

	//include shader files and face images
	//order
	//right
	//left
	//top
	//bottom
	//front
	//back
	Skybox(const char* vs, const char* fs, std::vector<const char*> faces) {
		shader = Shader(vs, fs);
		enabled = false;

		loadSkyBox(faces);
	}

	//load Skybox
	void loadSkyBox(std::vector<const char*> texFaces) {
		//gen and bind cubemap
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		//set skybox vars
		textureFaces = texFaces;

		//load individual faces
		for (unsigned int i = 0; i < texFaces.size(); i++)
		{
			unsigned char *data = stbi_load(texFaces[i], &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

				stbi_image_free(data);
			}
			else
			{
				std::cout << "Cubemap tex failed to load at path: " << texFaces[i] << std::endl;
				stbi_image_free(data);
			}
		}
		//clamp the edges and apply parameters
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		//gen VBO and VAO

		float skyboxVertices[] = {
			// positions          
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		enabled = true;
	}

	void render(glm::mat4 proj, glm::mat4 view) {
		if (enabled) {
			glDepthMask(GL_FALSE);

			//select proper cubemap to use
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

			shader.use();

			shader.setMat4("projection", proj);
			shader.setMat4("view", glm::mat4(glm::mat3(view)));

			glBindVertexArray(VAO);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glDepthMask(GL_TRUE);
		}
	}
};

#endif