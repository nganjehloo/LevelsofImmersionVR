#include "skybox.h"

GLfloat sb_vertices[] = {
	// Front vertices
	-0.5, -0.5,  0.5,
	0.5, -0.5,  0.5,
	0.5,  0.5,  0.5,
	-0.5,  0.5,  0.5,
	// Back vertices
	-0.5, -0.5, -0.5,
	0.5, -0.5, -0.5,
	0.5,  0.5, -0.5,
	-0.5,  0.5, -0.5
};

GLuint sb_indices[] = {
	// Front face
	0, 1, 2,
	2, 3, 0,
	// Top face
	1, 5, 6,
	6, 2, 1,
	// Back face
	7, 6, 5,
	5, 4, 7,
	// Bottom face
	4, 0, 3,
	3, 7, 4,
	// Left face
	4, 5, 1,
	1, 0, 4,
	// Right face
	3, 2, 6,
	6, 7, 3
};

Skybox::Skybox(ovrEyeType eyeType, bool custom)
{
	this->toWorld = glm::mat4(1.0f);
	this->eyeType = eyeType;

	// create the buffers/arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	//bind the VAO
	glBindVertexArray(VAO);
	//copy the vertices in a vertex buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sb_vertices), sb_vertices, GL_STATIC_DRAW);

	//copy the face data into an element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sb_indices), sb_indices, GL_STATIC_DRAW);

	//set the vertex positions attribute pointer
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

	//unbind the VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//unbind the VAO
	glBindVertexArray(0);

	//get all of the faces
	if (eyeType == ovrEye_Left)
	{
		if (custom)
		{
			faces.push_back("skybox/custom_left/px.ppm"); // left
			faces.push_back("skybox/custom_left/nx.ppm"); // right
			faces.push_back("skybox/custom_left/py.ppm"); // top
			faces.push_back("skybox/custom_left/ny.ppm"); // bottom
			faces.push_back("skybox/custom_left/pz.ppm"); // back
			faces.push_back("skybox/custom_left/nz.ppm"); // front
		}
		else
		{
			faces.push_back("skybox/left/px.ppm"); // left
			faces.push_back("skybox/left/nx.ppm"); // right
			faces.push_back("skybox/left/py.ppm"); // top
			faces.push_back("skybox/left/ny.ppm"); // bottom
			faces.push_back("skybox/left/pz.ppm"); // back
			faces.push_back("skybox/left/nz.ppm"); // front
		}
	}
	else
	{
		if (custom)
		{
			faces.push_back("skybox/custom_right/px.ppm"); // left
			faces.push_back("skybox/custom_right/nx.ppm"); // right
			faces.push_back("skybox/custom_right/py.ppm"); // top
			faces.push_back("skybox/custom_right/ny.ppm"); // bottom
			faces.push_back("skybox/custom_right/pz.ppm"); // back
			faces.push_back("skybox/custom_right/nz.ppm"); // front
		}
		else
		{
			faces.push_back("skybox/right/px.ppm"); // left
			faces.push_back("skybox/right/nx.ppm"); // right
			faces.push_back("skybox/right/py.ppm"); // top
			faces.push_back("skybox/right/ny.ppm"); // bottom
			faces.push_back("skybox/right/pz.ppm"); // back
			faces.push_back("skybox/right/nz.ppm"); // front
		}
	}

	//setup the cubemap for the skybox
	loadCubemap();

	//make it bigger!
	scale(100.0f);
}

Skybox::~Skybox()
{
	//deallocate all resources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

void Skybox::loadCubemap()
{
	int width, height;
	unsigned char* image;

	//create ID for the texture
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE0);

	//setup the texture
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	//load up all the faces
	for (GLuint i = 0; i < faces.size(); i++)
	{
		image = loadPPM(faces[i], width, height);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	}

	// Use bilinear interpolation:
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Use clamp to edge to hide skybox edges:
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Skybox::draw(Shader shader)
{
	//supply the model, view, and projection matrices to the shader
	GLuint modelLoc = glGetUniformLocation(shader.Program, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &toWorld[0][0]);

	glDepthMask(GL_FALSE);
	glCullFace(GL_FRONT);

	//draw the skybox cube
	glBindVertexArray(VAO);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(shader.Program, "skybox"), 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glCullFace(GL_BACK);
	glDepthMask(GL_TRUE);
}

void Skybox::scale(float amount)
{
	//scale the object by the specified amount
	toWorld = glm::scale(toWorld, glm::vec3(amount, amount, amount));
}