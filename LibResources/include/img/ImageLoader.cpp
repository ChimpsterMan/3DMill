#define STB_IMAGE_IMPLEMENTATION
#include "ImageLoader.h"
#include "img/stb_image.h"
#include <glad/glad.h>
#include <GLFW\glfw3.h>
#include <string>
#include <iostream>

class DataLoader {
	static bool load(std::string url) {
		bool toReturn;
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		// set the texture wrapping/filtering options (on the currently bound texture object)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load and generate the texture
		int width, height, nrChannels;
		unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			toReturn = true;
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
			toReturn = false;
		}
		stbi_image_free(data);
		return toReturn;
	}
};