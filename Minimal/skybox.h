#ifndef SKYBOX_H
#define SKYBOX_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

using std::vector;

#include <OVR_CAPI.h>

#include "loadPPM.h"
#include "shader.h"

class Skybox
{
private:
	GLuint VAO, VBO, EBO;
	GLuint textureID;
	vector<const GLchar*> faces;
	glm::mat4 toWorld;
	ovrEyeType eyeType;

	void loadCubemap();

public:
	Skybox(ovrEyeType eyeType, bool custom);
	~Skybox();
	void draw(Shader shader);
	void scale(float amount);

	GLuint getTextureID() { return textureID; }
};

#endif