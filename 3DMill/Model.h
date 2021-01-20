#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "img/ImageLoader.h"
#include "img/stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <shader.h>

#include "Mesh.h"
#include "Camera.h"

#include <vector>
#include <map>
#include <iostream>

inline unsigned int TextureFromFile(const char *path, const string &directory, int samples = 1, bool gamma = false);

//Do not reinitialize the model
class Model {
public:
	//model data
	vector<Texture> textures_loaded;
	vector<Mesh> meshes;
	string directory;
	string name;
	bool gammaCorrection;

	//multisampling
	int samples;

	//expects file path to 3d model with multisampling
	Model(string const &path, int samples, bool gamma = false) : gammaCorrection(gamma)
	{
		this->samples = samples;

		loadModel(path);
	}

	//expects file path to 3d model
	Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
	{
		//default 1 sample
		this->samples = 1;

		loadModel(path);
	}

	//draws the model and all meshes with it according to the shader
	void Draw(Shader &shader, Camera &camera) {
		for (unsigned int i = 0; i < meshes.size(); i++) {
			meshes[i].Draw(shader);
		}
	}

	//sort the meshes before drawing based on camera position (furthest first)
	void DrawSorted(Shader &shader, Camera &camera) {
		vector<int> sorted = vector<int>();
		for (unsigned int i = 0; i < meshes.size(); i++) {
			sorted.push_back(i);
		}

		//ignore the sort if there is only 1 item anyways
		if (meshes.size() > 1) {
			//sort the list of components so the largest distance is first
			bool swapped;
			for (unsigned int i = 0; i < meshes.size(); i++) {
				swapped = false;
				for (unsigned int j = 0; j < meshes.size() - 1 - i; j++) {
					if (glm::distance(meshes[sorted[j]].avgPos, camera.pos) > glm::distance(meshes[sorted[j + 1]].avgPos, camera.pos)) {
						int temp = sorted[j];
						sorted[j] = sorted[j + 1];
						sorted[j + 1] = temp;

						swapped = true;
					}
				}

				//exit search
				if (swapped == false) {
					break;
				}
			}
		}

		for (unsigned int i = 0; i < sorted.size(); i++) {
			meshes[sorted[i]].Draw(shader);
		}
	}

private:
	void loadModel(string const &path)
	{
		//read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		//check for errors
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}

		//retrieve the directory path of the filepath
		directory = path.substr(0, path.find_last_of('\\'));

		//retrieve the name of the model file
		std::size_t found = path.find_last_of('\\');
		name = path.substr(found + 1);
		//std::cout << "Name: " << name << std::endl;

		//process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	//seperate and process each mesh from the nodes in the scene
	void processNode(aiNode *node, const aiScene *scene)
	{
		//process all the node's meshes (if any)
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		//do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh *mesh, const aiScene *scene)
	{
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;
		vector<Material> materials;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			//process vertex positions, normals and texture coordinates
			//position
			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			//normals
			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			
			//texture coords
			if (mesh->mTextureCoords[0]) //does the mesh contain texture coordinates?
			{
				//usual coords
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;

				//tangent
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.Tangent = vector;

				//bitangent
				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vertex.Bitangent = vector;
			}
			else {
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vertex);
		}

		//process indices
		//go through each face and retrieve vertex indicies
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		//process material
		if (mesh->mMaterialIndex >= 0)
		{
			//assimp material
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			//setup material data for shader
			Material mat;

			//diffuse
			aiColor3D diffuse(0.0f, 0.0f, 0.0f);
			material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			mat.diffuse = glm::vec3(diffuse.r, diffuse.g, diffuse.b);

			//specular
			aiColor3D specular(0.0f, 0.0f, 0.0f);
			material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
			mat.specular = glm::vec3(specular.r, specular.g, specular.b);

			//specular shininess
			float shine;
			material->Get(AI_MATKEY_SHININESS, shine);
			mat.shine = shine;

			//specular strength
			float specularStrength;
			material->Get(AI_MATKEY_SHININESS_STRENGTH, specularStrength);
			mat.specularStrength = specularStrength;

			//ambient
			aiColor3D ambient(0.0f, 0.0f, 0.0f);
			material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
			mat.ambient = glm::vec3(ambient.r, ambient.g, ambient.b);

			//opacity
			float opacity;
			material->Get(AI_MATKEY_OPACITY, opacity);
			mat.opacity = opacity;

			//cout << mat.ambient.x << " " << mat.ambient.y << " " << mat.ambient.z << endl;

			//add material to mesh
			materials.push_back(mat);
			
			//material map setups
			//diffuse maps
			vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			//specular maps
			vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			//normal maps
			std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
			//height maps
			std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
		}


		return Mesh(vertices, indices, textures, materials, samples);
	}

	//unpacks the textures from assimp into the texture struct so it's more manipulatable
	vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName) {
		vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}
			if (!skip)
			{   // if texture hasn't been loaded already, load it
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), directory, samples);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture); // add to loaded textures
			}
		}
		return textures;
	}
};

//method from stb_image.h
unsigned int TextureFromFile(const char *path, const string &directory, int samples, bool gamma)
{
	string filename = string(path);
	filename = directory + "\\" + filename;
	cout << filename << endl;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		//multisampling (samples)x
		if (samples > 1) {
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_TRUE);
			glGenerateMipmap(GL_TEXTURE_2D_MULTISAMPLE);

			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureID, 0);
			cout << "here" << endl;
		}
		else {
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
};

#endif
