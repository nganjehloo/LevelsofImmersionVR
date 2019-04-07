#include "cube.h"

GLfloat cube_vertices[] = {
	// Positions          // Texture Coords

	// front
	-0.5f, -0.5f, 0.5f,  0.0f, 1.0f,
	0.5f, -0.5f, 0.5f,  1.0f, 1.0f,
	0.5f,  0.5f, 0.5f,  1.0f, 0.0f,
	0.5f,  0.5f, 0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f, 0.5f,  0.0f, 0.0f,
	-0.5f, -0.5f, 0.5f,  0.0f, 1.0f,

	// back
	-0.5f, -0.5f,  -0.5f,  1.0f, 1.0f,
	0.5f, -0.5f,  -0.5f,  0.0f, 1.0f,
	0.5f,  0.5f,  -0.5f,  0.0f, 0.0f,
	0.5f,  0.5f,  -0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f,  -0.5f,  1.0f, 0.0f,
	-0.5f, -0.5f,  -0.5f,  1.0f, 1.0f,

	// left
	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	// right
	0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	0.5f,  0.5f, -0.5f,  1.0f, 0.0f,
	0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
	0.5f,  0.5f,  0.5f,  0.0f, 0.0f,

	// bottom
	-0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
	0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  1.0f, 1.0f,

	// top
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

Cube::Cube()
{
	this->toWorld = glm::mat4(1.0f);
	this->currentScale = 1.0f;

	// create the buffers/arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	//bind the VAO
	glBindVertexArray(VAO);
	//copy the vertices in a vertex buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	//set the vertex positions attribute pointer
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	//unbind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//unbind the VAO
	glBindVertexArray(0);

	//setup the texturemap for the cube
	setupTexturemap();

	// move it in front of the user and scale it down a little
	this->toWorld = glm::translate(toWorld, glm::vec3(0.0f, -0.4f, -0.6f));
	this->toWorld = glm::scale(toWorld, glm::vec3(0.5f, 0.5f, 0.5f));
}

Cube::~Cube()
{
	//deallocate all resources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

void Cube::setupTexturemap()
{
	int width, height;
	unsigned char* image;

	//load the image
	image = loadPPM("cube/vr_test_pattern.ppm", width, height);

	//create ID for the texture
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE0);

	//setup the texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
		0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	//set texturemap properties
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Cube::draw(Shader shader)
{
	//supply the model, view, and projection matrices to the shader
	GLuint modelLoc = glGetUniformLocation(shader.Program, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &toWorld[0][0]);

	//draw the skybox cube
	glBindVertexArray(VAO);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(shader.Program, "textureSampler"), 0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void Cube::scale(float amount)
{
	//scale the object by the specified amount
	float newScale = currentScale * amount;
	if (newScale > 0.1f && newScale < 2.0f)
	{
		currentScale = newScale;
		this->toWorld = glm::scale(toWorld, glm::vec3(amount, amount, amount));
	}
}

void Cube::translate(glm::vec3 amount)
{
	// translate the object by the specified amount
	this->toWorld = glm::translate(toWorld, amount);
}