/*
  CSCI 420 Computer Graphics, Computer Science, USC
  Assignment f Height Fields with Shaders.
  C/C++ starter code

  Student username: <type your USC username here>
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <glm/glm.hpp>

#include <iostream>
#include <cstring>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

enum class RenderMode : uint8_t
{
	Point,
	Line,
	Triangle,
	Smooth,
	Mixed
};

RenderMode renderMode = RenderMode::Point;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f };
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

const float TerrainHeightScale = 0.16f;
const float TerrainSmoothHeightScale = 1.0f / 255.0f;

float scale = 1.0f;
float exponent = 1.0f;

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 1";

// Stores the image loaded from disk.
ImageIO* heightmapImage;
ImageIO* textureImage;

GLuint terrainTexture = 0;

// Number of vertices in the single triangle (starter code).
int numPointVertices;
int numLineVertices;
int numTriangleVertices;
int numSmoothVertices;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram* pipelineProgram = nullptr;
VBO* pointPositionsVBO = nullptr;
VBO* linePositionsVBO = nullptr;
VBO* trianglePositionsVBO = nullptr;
VBO* pointColorsVBO = nullptr;
VBO* lineColorsVBO = nullptr;
VBO* triangleColorsVBO = nullptr;
VBO* triangleTexcoordsVBO = nullptr;
VBO* triangleNormalsVBO = nullptr;

VBO* smoothTrianglePositionsVBO = nullptr;

VBO* leftPositionsVBO = nullptr;
VBO* rightPositionsVBO = nullptr;
VBO* upPositionsVBO = nullptr;
VBO* downPositionsVBO = nullptr;

VAO* pointVAO = nullptr;
VAO* lineVAO = nullptr;
VAO* triangleVAO = nullptr;

VAO* smoothTriangleVAO = nullptr;

std::vector<glm::vec3> terrainPointPositions;
std::vector<glm::vec3> terrainLinePositions;
std::vector<glm::vec3> terrainTrianglePositions;
std::vector<glm::vec3> terrainSmoothTrianglePositions;
std::vector<glm::vec4> terrainPointColors;
std::vector<glm::vec4> terrainLineColors;
std::vector<glm::vec4> terrainTriangleColors;
std::vector<glm::vec2> terrainTriangleTexcoords;
std::vector<glm::vec3> terrainTriangleNormals;

std::vector<glm::vec3> leftPositions;
std::vector<glm::vec3> rightPositions;
std::vector<glm::vec3> upPositions;
std::vector<glm::vec3> downPositions;

glm::vec3 eye{ 128.0f, 128.0f, 128.0f };
glm::vec3 target{ 0.0f, 0.0f, 0.0f };

glm::vec3 lightDir{ -1.0f, -1.0f, -1.0f };
glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };

const float CameraMoveSpeed = 1.0f;

bool useTexture = false;
bool toggleLight = false;

// Write a screenshot to the specified filename.
void saveScreenshot(const char* filename)
{
	unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

void idleFunc()
{
	// Do some stuff... 
	// For example, here, you can save the screenshots to disk (to make the animation).
	if (GetAsyncKeyState('W') & 0x8000)
	{
		eye.z -= CameraMoveSpeed;
		target.z -= CameraMoveSpeed;
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		eye.z += CameraMoveSpeed;
		target.z += CameraMoveSpeed;
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		eye.x -= CameraMoveSpeed;
		target.x -= CameraMoveSpeed;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		eye.x += CameraMoveSpeed;
		target.x += CameraMoveSpeed;
	}

	if (GetAsyncKeyState('Q') & 0x8000)
	{
		eye.y -= CameraMoveSpeed;
		target.y -= CameraMoveSpeed;
	}

	if (GetAsyncKeyState('E') & 0x8000)
	{
		eye.y += CameraMoveSpeed;
		target.y += CameraMoveSpeed;
	}

	// Notify GLUT that it should call displayFunc.
	glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	// When the window has been resized, we need to re-set our projection matrix.
	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.LoadIdentity();
	// You need to be careful about setting the zNear and zFar. 
	// Anything closer than zNear, or further than zFar, will be culled.
	const float zNear = 0.1f;
	const float zFar = 10000.0f;
	const float humanFieldOfView = 60.0f;
	matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
	// Mouse has moved, and one of the mouse buttons is pressed (dragging).

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	switch (controlState)
	{
		// translate the terrain
	case TRANSLATE:
		if (leftMouseButton)
		{
			// control x,y translation via the left mouse button
			terrainTranslate[0] += mousePosDelta[0] * 0.01f;
			terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z translation via the middle mouse button
			terrainTranslate[2] += mousePosDelta[1] * 0.01f;
		}
		break;

		// rotate the terrain
	case ROTATE:
		if (leftMouseButton)
		{
			// control x,y rotation via the left mouse button
			terrainRotate[0] += mousePosDelta[1];
			terrainRotate[1] += mousePosDelta[0];
		}
		if (middleMouseButton)
		{
			// control z rotation via the middle mouse button
			terrainRotate[2] += mousePosDelta[1];
		}
		break;

		// scale the terrain
	case SCALE:
		if (leftMouseButton)
		{
			// control x,y scaling via the left mouse button
			terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
			terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z scaling via the middle mouse button
			terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
	// Mouse has moved.
	// Store the new mouse position.
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
	// A mouse button has has been pressed or depressed.

	// Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_MIDDLE_BUTTON:
		middleMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_RIGHT_BUTTON:
		rightMouseButton = (state == GLUT_DOWN);
		break;
	}

	// Keep track of whether CTRL and SHIFT keys are pressed.
	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		controlState = TRANSLATE;
		break;

	case GLUT_ACTIVE_SHIFT:
		controlState = SCALE;
		break;

		// If CTRL and SHIFT are not pressed, we are in rotate mode.
	default:
		controlState = ROTATE;
		break;
	}

	// Store the new mouse position.
	mousePos[0] = x;
	mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // ESC key
		exit(0); // exit the program
		break;

	case ' ':
		cout << "You pressed the spacebar." << endl;
		break;

	case 'x':
		// Take a screenshot.
		saveScreenshot("screenshot.jpg");
		break;
	case '1':
		renderMode = RenderMode::Point;
		break;
	case '2':
		renderMode = RenderMode::Line;
		break;
	case '3':
		renderMode = RenderMode::Triangle;
		break;
	case '4':
		renderMode = RenderMode::Smooth;
		break;
	case '5':
		renderMode = RenderMode::Mixed;
		break;
	case '+':
		scale *= 2.0f;
		break;
	case '-':
		scale *= 0.5f;
		break;
	case '9':
		exponent *= 2.0f;
		break;
	case '0':
		exponent *= 0.5f;
		break;
	case 't':
		useTexture = !useTexture;
		break;
	case 'l':
		toggleLight = !toggleLight;
		break;
	}
}

void displayFunc()
{
	// This function performs the actual rendering.

	// First, clear the screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up the camera position, focus point, and the up vector.
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.LoadIdentity();
	matrix.LookAt(eye.x, eye.y, eye.z, target.x, target.y, target.z, 0.0, 1.0, 0.0);

	// In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
	// ...

	// Read the current modelview and projection matrices from our helper class.
	// The matrices are only read here; nothing is actually communicated to OpenGL yet.
	float modelViewMatrix[16];

	matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
	matrix.Rotate(terrainRotate[0], 1.0, 0.0, 0.0);
	matrix.Rotate(terrainRotate[1], 0.0, 1.0, 0.0);
	matrix.Rotate(terrainRotate[2], 0.0, 0.0, 1.0);
	matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.GetMatrix(modelViewMatrix);

	float projectionMatrix[16];
	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.GetMatrix(projectionMatrix);

	// Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
	// Important: these matrices must be uploaded to *all* pipeline programs used.
	// In hw1, there is only one pipeline program, but in hw2 there will be several of them.
	// In such a case, you must separately upload to *each* pipeline program.
	// Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
	pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
	pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

	pipelineProgram->SetUniformVariablei("mode", 0);
	pipelineProgram->SetUniformVariablei("useTexture", useTexture);
	pipelineProgram->SetUniformVariablei("toggleLight", toggleLight);

	pipelineProgram->SetUniformVariableVec3("lightDir", &lightDir[0]);
	pipelineProgram->SetUniformVariableVec3("lightColor", &lightColor[0]);

	// Execute the rendering.
	// Bind the VAO that we want to render. Remember, one object = one VAO. 
	pointVAO->Bind();

	switch (renderMode)
	{
	case RenderMode::Point:
		pointVAO->Bind();
		glDrawArrays(GL_POINTS, 0, numPointVertices); // Render the VAO, by rendering "numVertices", starting from vertex 0.
		break;
	case RenderMode::Line:
		lineVAO->Bind();
		glDrawArrays(GL_LINES, 0, numLineVertices); // Render the VAO, by rendering "numVertices", starting from vertex 0.
		break;
	case RenderMode::Triangle:
		triangleVAO->Bind();

		glDrawArrays(GL_TRIANGLES, 0, numTriangleVertices); // Render the VAO, by rendering "numVertices", starting from vertex 0.
		break;
	case RenderMode::Smooth:
		smoothTriangleVAO->Bind();

		pipelineProgram->SetUniformVariablei("mode", 1);

		pipelineProgram->SetUniformVariablef("scale", scale);
		pipelineProgram->SetUniformVariablef("exponent", exponent);

		glDrawArrays(GL_TRIANGLES, 0, numSmoothVertices);
		break;

	case RenderMode::Mixed:
		triangleVAO->Bind();

		glDrawArrays(GL_TRIANGLES, 0, numTriangleVertices); // Render the VAO, by rendering "numVertices", starting from vertex 0.

		glPolygonOffset(-1.0, -1.0);

		lineVAO->Bind();

		pipelineProgram->SetUniformVariablei("useTexture", false);

		glDrawArrays(GL_LINES, 0, numLineVertices); // Render the VAO, by rendering "numVertices", starting from vertex 0.
		break;
	default:
		break;
	}

	// Swap the double-buffers.
	glutSwapBuffers();
}

glm::vec4 getHeightColor(float height)
{
	if (height >= 180.0f && height < 255.0f)
	{
		return glm::vec4(1.0f);
	}
	else if (height >= 80.0f && height < 180.0f)
	{
		return glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);
	}
	else if (height >= 0.0f && height < 80.0f)
	{
		return glm::vec4(108 / 255.0f, 81 / 255.0f, 60 / 255.0f, 1.0f);
	}

	return glm::vec4(0.0f);
}

void createTerrainLineData()
{
	terrainPointPositions.clear();
	terrainPointColors.clear();

	int imageWidth = heightmapImage->getWidth();
	int imageHeight = heightmapImage->getHeight();

	int halfImageWidth = imageWidth / 2;
	int halfImageHeight = imageHeight / 2;

	int startPointX = -halfImageWidth;
	int startPointZ = -halfImageHeight;

	int terrainWidth = halfImageWidth - 1;
	int terrainHeight = halfImageHeight - 1;

	for (int i = startPointX; i < terrainWidth; i++)
	{
		for (int j = startPointZ; j < terrainHeight; j++)
		{
			float height = heightmapImage->getPixel(i + halfImageWidth, j + halfImageHeight, 0);
			
			auto color = glm::vec4(height / 255.0f);

			color = getHeightColor(height) * height / 255.0f;

			height *= TerrainHeightScale;

			terrainPointPositions.push_back({ i, height, -j });
			terrainPointColors.push_back(color);

			if (i < terrainWidth - 1)
			{
				terrainLinePositions.push_back({ i, height, -j });
				terrainLineColors.push_back(color);

				float height = heightmapImage->getPixel((i + 1) + halfImageWidth, j + halfImageHeight, 0);

				auto color = glm::vec4(height / 255.0f);

				color = getHeightColor(height) * height / 255.0f;;

				height *= TerrainHeightScale;

				terrainLinePositions.push_back({ i + 1, height, -j });
				terrainLineColors.push_back(color);
			}

			if (j < terrainHeight - 1)
			{
				terrainLinePositions.push_back({ i, height, -j });
				terrainLineColors.push_back(color);

				float height = heightmapImage->getPixel(i + halfImageWidth, (j + 1) + halfImageHeight, 0);

				auto color = glm::vec4(height / 255.0f);

				color = getHeightColor(height) * height / 255.0f;;

				height *= TerrainHeightScale;

				terrainLinePositions.push_back({ i, height, -(j + 1) });
				terrainLineColors.push_back(color);
			}
		}
	}

	numPointVertices = static_cast<int>(terrainPointPositions.size());
	numLineVertices = static_cast<int>(terrainLinePositions.size());
}

void createTerrainTriangleData(std::vector<glm::vec3>& positions, int mode)
{
	int imageWidth = heightmapImage->getWidth();
	int imageHeight = heightmapImage->getHeight();

	int halfImageWidth = imageWidth / 2;
	int halfImageHeight = imageHeight / 2;

	int startPointX = -halfImageWidth;
	int startPointZ = -halfImageHeight;

	int terrainWidth = halfImageWidth - 1;
	int terrainHeight = halfImageHeight - 1;

	float scale = 1.0f;

	if (mode == 0)
	{
		scale = TerrainHeightScale;
	}
	else
	{
		scale = TerrainSmoothHeightScale;
	}

	glm::vec2 texelSize = { 1.0f / imageWidth, 1.0f / imageHeight };

	for (int i = startPointX; i < terrainWidth; i++)
	{
		for (int j = startPointZ; j < terrainHeight; j++)
		{
			int x = i + halfImageWidth;
			int y = j + halfImageHeight;

			float height = heightmapImage->getPixel(x, y, 0);

			auto color = glm::vec4(height / 255.0f);

			color = getHeightColor(height) * height / 255.0f;;

			height *= scale;

			glm::vec3 position0 = { i, height, -j };

			positions.push_back(position0);
			terrainTriangleColors.push_back(color);
			terrainTriangleTexcoords.push_back({ texelSize.x * x, texelSize.y * y });

			x = (i + 1) + halfImageWidth;
			y = j + halfImageHeight;

			height = heightmapImage->getPixel(x, y, 0);

			color = glm::vec4(height / 255.0f);

			color = getHeightColor(height) * height / 255.0f;;

			height *= scale;

			glm::vec3 position1 = { (i + 1), height, -j };

			positions.push_back(position1);
			terrainTriangleColors.push_back(color);
			terrainTriangleTexcoords.push_back({ texelSize.x * x, texelSize.y * y });

			x = (i + 1) + halfImageWidth;
			y = (j + 1) + halfImageHeight;

			height = heightmapImage->getPixel(x, y, 0);

			color = glm::vec4(height / 255.0f);

			color = getHeightColor(height) * height / 255.0f;;

			height *= scale;

			glm::vec3 position2 = { (i + 1), height, -(j + 1) };

			positions.push_back(position2);
			terrainTriangleColors.push_back(color);
			terrainTriangleTexcoords.push_back({ texelSize.x * x, texelSize.y * y });

			auto e01 = position1 - position0;
			auto e02 = position2 - position0;

			auto normal = glm::normalize(glm::cross(e01, e02));

			terrainTriangleNormals.emplace_back(normal);
			terrainTriangleNormals.emplace_back(normal);
			terrainTriangleNormals.emplace_back(normal);

			x = i + halfImageWidth;
			y = j + halfImageHeight;

			height = heightmapImage->getPixel(x, y, 0);

			color = glm::vec4(height / 255.0f);

			color = getHeightColor(height) * height / 255.0f;;

			height *= scale;

			position0 = { i, height, -j };

			positions.push_back(position0);
			terrainTriangleColors.push_back(color);
			terrainTriangleTexcoords.push_back({ texelSize.x * x, texelSize.y * y });

			x = (i + 1) + halfImageWidth;
			y = (j + 1) + halfImageHeight;

			height = heightmapImage->getPixel(x, y, 0);

			color = glm::vec4(height / 255.0f);

			color = getHeightColor(height) * height / 255.0f;;

			height *= scale;

			position1 = { (i + 1), height, -(j + 1) };

			positions.push_back(position1);
			terrainTriangleColors.push_back(color);
			terrainTriangleTexcoords.push_back({ texelSize.x * x, texelSize.y * y });

			x = i + halfImageWidth;
			y = (j + 1) + halfImageHeight;

			height = heightmapImage->getPixel(x, y, 0);

			color = glm::vec4(height / 255.0f);

			color = getHeightColor(height) * height / 255.0f;;

			height *= scale;

			position2 = { i, height, -(j + 1) };

			positions.push_back(position2);
			terrainTriangleColors.push_back(color);
			terrainTriangleTexcoords.push_back({ texelSize.x * x, texelSize.y * y });

			e01 = position1 - position0;
			e02 = position2 - position0;

			normal = glm::normalize(glm::cross(e01, e02));

			terrainTriangleNormals.emplace_back(normal);
			terrainTriangleNormals.emplace_back(normal);
			terrainTriangleNormals.emplace_back(normal);
		}
	}

	numTriangleVertices = static_cast<int>(positions.size());
}

void createTerrainSmoothTriangleData()
{
	int imageWidth = heightmapImage->getWidth();
	int imageHeight = heightmapImage->getHeight();

	int halfImageWidth = imageWidth / 2;
	int halfImageHeight = imageHeight / 2;

	int startPointX = -halfImageWidth;
	int startPointZ = -halfImageHeight;

	int terrainWidth = halfImageWidth - 1;
	int terrainHeight = halfImageHeight - 1;

	for (int i = startPointX; i < terrainWidth; i++)
	{
		for (int j = startPointZ; j < terrainHeight; j++)
		{
			float height = 0.0f;

			// P(i, j)
			if (i == startPointX)
			{
				height = heightmapImage->getPixel(i + halfImageWidth, j + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i - 1) + halfImageWidth, j + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			leftPositions.push_back({ i - 1, height, -j });

			height = heightmapImage->getPixel((i + 1) + halfImageWidth, j + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			rightPositions.push_back({ i + 1, height, -j });

			if (j == startPointZ)
			{
				height = heightmapImage->getPixel(i + halfImageWidth, j + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel(i + halfImageWidth, (j - 1) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			downPositions.push_back({ i, height, j - 1 });

			height = heightmapImage->getPixel(i + halfImageWidth, (j + 1) + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			upPositions.push_back({ i, height, j + 1 });

			// P(i + 1, j)
			height = heightmapImage->getPixel(i + halfImageWidth, j + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			leftPositions.push_back({ i, height, -j });

			if (i == startPointX - 2)
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, j + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i + 2) + halfImageWidth, j + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			rightPositions.push_back({ i + 2, height, -j });

			if (j == -startPointZ)
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, j + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j - 1) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			downPositions.push_back({ i + 1, height, -(j - 1) });

			height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 1) + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			upPositions.push_back({ i + 1, height, -(j + 1)});

			// P(i + 1, j + 1)
			height = heightmapImage->getPixel(i + halfImageWidth, (j + 1) + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			leftPositions.push_back({ i, height, -(j + 1) });

			if (i == halfImageWidth - 2)
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i + 2) + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			rightPositions.push_back({ i + 2, height, -(j + 1) });

			height = heightmapImage->getPixel((i + 1) + halfImageWidth, j + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			downPositions.push_back({ i + 1, height, -j });

			if (j == halfImageHeight - 2)
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 2) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			upPositions.push_back({ i + 1, height, -(j + 2) });

			// P(i, j)
			if (i == startPointX)
			{
				height = heightmapImage->getPixel(i + halfImageWidth, j + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i - 1) + halfImageWidth, j + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			leftPositions.push_back({ i - 1, height, -j });

			height = heightmapImage->getPixel((i + 1) + halfImageWidth, j + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			rightPositions.push_back({ i + 1, height, -j });

			if (j == startPointZ)
			{
				height = heightmapImage->getPixel(i + halfImageWidth, j + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel(i + halfImageWidth, (j - 1) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			downPositions.push_back({ i, height, -(j - 1) });

			height = heightmapImage->getPixel(i + halfImageWidth, (j + 1) + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			upPositions.push_back({ i, height, -(j + 1) });

			// P(i + 1, j + 1)
			height = heightmapImage->getPixel(i + halfImageWidth, (j + 1) + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			leftPositions.push_back({ i, height, -(j + 1) });

			if (i == halfImageWidth - 2)
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i + 2) + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			rightPositions.push_back({ 1 + 2, height, -(j + 1) });

			height = heightmapImage->getPixel((i + 1) + halfImageWidth, j + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			downPositions.push_back({ i + 1, height, -j });

			if (j == halfImageHeight - 2)
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 2) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			upPositions.push_back({ i + 1, height, -(j + 2) });

			// P(i, j + 1)
			if (i == startPointX)
			{
				height = heightmapImage->getPixel(i + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel((i - 1) + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			leftPositions.push_back({ i - 1, height, -(j + 1) });

			height = heightmapImage->getPixel((i + 1) + halfImageWidth, (j + 1) + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			rightPositions.push_back({ i + 1, height, -(j + 1) });

			height = heightmapImage->getPixel(i + halfImageWidth, j + halfImageHeight, 0);

			height *= TerrainSmoothHeightScale;

			downPositions.push_back({ i, height, -j });

			if (j == halfImageHeight - 2)
			{
				height = heightmapImage->getPixel(i + halfImageWidth, (j + 1) + halfImageHeight, 0);
			}
			else
			{
				height = heightmapImage->getPixel(i + halfImageWidth, (j + 2) + halfImageHeight, 0);
			}

			height *= TerrainSmoothHeightScale;

			upPositions.push_back({ i, height, -(j + 2) });
		}
	}

	numSmoothVertices = leftPositions.size();
}

GLuint createTexture(const std::string& path)
{
	textureImage = new ImageIO();

	if (textureImage->loadJPEG(path.c_str()) != ImageIO::OK)
	{
		cout << "Error reading image " << path << "." << endl;
		exit(EXIT_FAILURE);
	}

	GLuint texture = 0;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureImage->getWidth(), textureImage->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, textureImage->getPixels());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return texture;
}

void initScene(int argc, char* argv[])
{
	// Load the image from a jpeg disk file into main memory.
	heightmapImage = new ImageIO();
	if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
	{
		cout << "Error reading image " << argv[1] << "." << endl;
		exit(EXIT_FAILURE);
	}

	terrainTexture = createTexture("./heightmap/photo.jpg");

	// Set the background color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

	// Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
	glEnable(GL_DEPTH_TEST);

	// Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
	// A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
	// In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
	// In hw2, we will need to shade different objects with different shaders, and therefore, we will have
	// several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
	pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
	// Load and set up the pipeline program, including its shaders.
	if (pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
	{
		cout << "Failed to build the pipeline program." << endl;
		throw 1;
	}
	cout << "Successfully built the pipeline program." << endl;

	// Bind the pipeline program that we just created. 
	// The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
	// any object rendered from that point on, will use those shaders.
	// When the application starts, no pipeline program is bound, which means that rendering is not set up.
	// So, at some point (such as below), we need to bind a pipeline program.
	// From that point on, exactly one pipeline program is bound at any moment of time.
	pipelineProgram->Bind();

	// Prepare the triangle position and color data for the VBO. 
	// The code below sets up a single triangle (3 vertices).
	// The triangle will be rendered using GL_TRIANGLES (in displayFunc()).

	createTerrainLineData();
	createTerrainTriangleData(terrainTrianglePositions, 0);
	createTerrainTriangleData(terrainSmoothTrianglePositions, 1);
	createTerrainSmoothTriangleData();

	// Create the VBOs. 
	// We make a separate VBO for vertices and colors. 
	// This operation must be performed BEFORE we initialize any VAOs.
	pointPositionsVBO = new VBO(numPointVertices, 3, terrainPointPositions.data(), GL_STATIC_DRAW); // 3 values per position
	pointColorsVBO = new VBO(numPointVertices, 4, terrainPointColors.data(), GL_STATIC_DRAW); // 4 values per color
	linePositionsVBO = new VBO(numLineVertices, 3, terrainLinePositions.data(), GL_STATIC_DRAW);
	lineColorsVBO = new VBO(numLineVertices, 4, terrainLineColors.data(), GL_STATIC_DRAW);
	trianglePositionsVBO = new VBO(numTriangleVertices, 3, terrainTrianglePositions.data(), GL_STATIC_DRAW);
	triangleColorsVBO = new VBO(numTriangleVertices, 4, terrainTriangleColors.data(), GL_STATIC_DRAW);

	triangleTexcoordsVBO = new VBO(numTriangleVertices, 2, terrainTriangleTexcoords.data(), GL_STATIC_DRAW);

	triangleNormalsVBO = new VBO(numTriangleVertices, 3, terrainTriangleNormals.data(), GL_STATIC_DRAW);

	smoothTrianglePositionsVBO = new VBO(numTriangleVertices, 3, terrainSmoothTrianglePositions.data(), GL_STATIC_DRAW);

	leftPositionsVBO = new VBO(numSmoothVertices, 3, leftPositions.data(), GL_STATIC_DRAW);
	rightPositionsVBO = new VBO(numSmoothVertices, 3, rightPositions.data(), GL_STATIC_DRAW);
	upPositionsVBO = new VBO(numSmoothVertices, 3, upPositions.data(), GL_STATIC_DRAW);
	downPositionsVBO = new VBO(numSmoothVertices, 3, downPositions.data(), GL_STATIC_DRAW);

	// Create the VAOs. There is a single VAO in this example.
	// Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
	// A VAO contains the geometry for a single object. There should be one VAO per object.
	// In this homework, "geometry" means vertex positions and colors. In homework 2, it will also include
	// vertex normal and vertex texture coordinates for texture mapping.
	pointVAO = new VAO();

	// Set up the relationship between the "position" shader variable and the VAO.
	// Important: any typo in the shader variable name will lead to malfunction.
	pointVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, pointPositionsVBO, "position");

	// Set up the relationship between the "color" shader variable and the VAO.
	// Important: any typo in the shader variable name will lead to malfunction.
	pointVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, pointColorsVBO, "color");

	lineVAO = new VAO();

	// Set up the relationship between the "position" shader variable and the VAO.
	// Important: any typo in the shader variable name will lead to malfunction.
	lineVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, linePositionsVBO, "position");

	// Set up the relationship between the "color" shader variable and the VAO.
	// Important: any typo in the shader variable name will lead to malfunction.
	lineVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, lineColorsVBO, "color");

	triangleVAO = new VAO();

	// Set up the relationship between the "position" shader variable and the VAO.
	// Important: any typo in the shader variable name will lead to malfunction.
	triangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, trianglePositionsVBO, "position");

	// Set up the relationship between the "color" shader variable and the VAO.
	// Important: any typo in the shader variable name will lead to malfunction.
	triangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, triangleColorsVBO, "color");

	triangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, triangleTexcoordsVBO, "inTexcoord");

	triangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, triangleNormalsVBO, "inNormal");

	smoothTriangleVAO = new VAO();

	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, smoothTrianglePositionsVBO, "position");
	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, triangleColorsVBO, "color");
	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, triangleTexcoordsVBO, "inTexcoord");

	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, triangleNormalsVBO, "inNormal");

	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, leftPositionsVBO, "leftPosition");
	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, rightPositionsVBO, "rightPosition");
	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, upPositionsVBO, "upPosition");
	smoothTriangleVAO->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, downPositionsVBO, "downPosition");

	// Check for any OpenGL errors.
	std::cout << "GL error status is: " << glGetError() << std::endl;
}

void cleanup()
{
	delete pointVAO;
	delete lineVAO;
	delete triangleVAO;

	delete pointPositionsVBO;
	delete linePositionsVBO;
	delete trianglePositionsVBO;

	delete pointColorsVBO;
	delete lineColorsVBO;
	delete triangleColorsVBO;

	delete smoothTriangleVAO;

	delete leftPositionsVBO;
	delete rightPositionsVBO;
	delete upPositionsVBO;
	delete downPositionsVBO;

	delete smoothTrianglePositionsVBO;

	delete triangleTexcoordsVBO;

	delete triangleNormalsVBO;

	delete heightmapImage;

	delete textureImage;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout << "The arguments are incorrect." << endl;
		cout << "usage: ./hw1 <heightmap file>" << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

	cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
	// This is needed on recent Mac OS X versions to correctly display the window.
	glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

	// Tells GLUT to use a particular display function to redraw.
	glutDisplayFunc(displayFunc);
	// Perform animation inside idleFunc.
	glutIdleFunc(idleFunc);
	// callback for mouse drags
	glutMotionFunc(mouseMotionDragFunc);
	// callback for idle mouse movement
	glutPassiveMotionFunc(mouseMotionFunc);
	// callback for mouse button changes
	glutMouseFunc(mouseButtonFunc);
	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);
	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);

	// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
	GLint result = glewInit();
	if (result != GLEW_OK)
	{
		cout << "error: " << glewGetErrorString(result) << endl;
		exit(EXIT_FAILURE);
	}
#endif

	// Perform the initialization.
	initScene(argc, argv);

	// Sink forever into the GLUT loop.
	glutMainLoop();

	cleanup();
}

