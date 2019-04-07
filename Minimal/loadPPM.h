#define _CRT_SECURE_NO_DEPRECATE

#ifndef PPMLOADER_H
#define PPMLOADER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h> 
#include <iostream> 

using namespace std;

unsigned char* loadPPM(const char* filename, int& width, int& height);
void loadTexture();

#endif
