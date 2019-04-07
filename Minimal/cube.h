#ifndef CUBE_H
#define CUBE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

using std::vector;

#include <OVR_CAPI.h>

#include "loadPPM.h"
#include "shader.h"

class Cube
{
private:
	GLuint VAO, VBO, EBO;
	GLuint textureID;
	glm::mat4 toWorld;
	float currentScale;

	void setupTexturemap();

public:
	Cube();
	~Cube();
	void draw(Shader shader);
	void scale(float amount);
	void translate(glm::vec3 amount);

	GLuint getTextureID() { return textureID; }
};

#endif