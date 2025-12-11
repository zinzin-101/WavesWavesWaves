#pragma once
#include <string>

const float SKYBOX_VERTICES[] =
{
	//   Coordinates
	-1.0f, -1.0f,  1.0f,//        7--------6
	 1.0f, -1.0f,  1.0f,//       /|       /|
	 1.0f, -1.0f, -1.0f,//      4--------5 |
	-1.0f, -1.0f, -1.0f,//      | |      | |
	-1.0f,  1.0f,  1.0f,//      | 3------|-2
	 1.0f,  1.0f,  1.0f,//      |/       |/
	 1.0f,  1.0f, -1.0f,//      0--------1
	-1.0f,  1.0f, -1.0f
};

unsigned int SKYBOX_INDICES[] =
{
	// Right
	1, 2, 6,
	6, 5, 1,
	// Left
	0, 4, 7,
	7, 3, 0,
	// Top
	4, 5, 6,
	6, 7, 4,
	// Bottom
	0, 3, 2,
	2, 1, 0,
	// Back
	0, 1, 5,
	5, 4, 0,
	// Front
	3, 7, 6,
	6, 2, 3
};

const float CUBE_VERTICES[] = {
	-0.5f, -0.5f, -0.5f, // 0
	 0.5f, -0.5f, -0.5f, // 1
	 0.5f,  0.5f, -0.5f, // 2
	-0.5f,  0.5f, -0.5f, // 3
	-0.5f, -0.5f,  0.5f, // 4
	 0.5f, -0.5f,  0.5f, // 5
	 0.5f,  0.5f,  0.5f, // 6
	-0.5f,  0.5f,  0.5f  // 7
};

const unsigned int CUBE_INDICES[] = {
	// bottom square
	0, 1, 1, 2, 2, 3, 3, 0,
	// top square
	4, 5, 5, 6, 6, 7, 7, 4,
	// vertical edges
	0, 4, 1, 5, 2, 6, 3, 7
};