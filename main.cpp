#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>


const unsigned int SHADOW_WIDTH = 8192;
const unsigned int SHADOW_HEIGHT = 4096;

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 rmat;
glm::mat4 lightDirMatrix;

//for moving objects -- modif
glm::mat4 model_bridge;
glm::mat3 normalMatrixBridge;
glm::mat4 model_hour;
glm::mat3 normalMatrixHour;
glm::mat4 model_min;
glm::mat3 normalMatrixMin;
glm::mat4 model_sec;
glm::mat3 normalMatrixSec;
glm::mat4 model_trumpet;
glm::mat3 normalMatrixTrumpet;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint lightDirMatrixLoc;

bool day = true, waspressed = false, waspressed_fog = false, waspressed_point = false, onPoint = false;

// camera
gps::Camera myCamera(
    glm::vec3(3.0f, 1.0f, 3.0f),
    glm::vec3(0.0f, 1.0f, 10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;
float rotationSpeed = 1.0f;
float mouse_sensitivity = 0.05f;

GLboolean pressedKeys[1024];

// models -- moodif
//gps::Model3D teapot;
gps::Model3D scene;
gps::Model3D hour;
gps::Model3D min;
gps::Model3D sec;
gps::Model3D trumpet;
gps::Model3D bridge;

//angles of rotation -- modif
GLfloat angle;
GLfloat angleX;
GLfloat angleY;
GLfloat angleHour = 0;
GLfloat angleMin = 0;
GLfloat angleSec = 0;
GLfloat angleTrumpet = 0;
GLfloat angleBridge = 0;

float cameraAngle = 270;
float yaw = -90, pitch;

// shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

//point lights
glm::vec3 lightPos1; 
glm::vec3 pointLightColor;
GLuint pointLightColorLoc;
GLuint lightPos1Loc;

//skybox
gps::SkyBox skyBoxDay, skyBoxNight;

//fog
float fogDensity = 0;
GLint fogDensityLoc;

//shadows
GLuint shadowMapFBO;
GLuint shadowMapFBO2;
GLuint depthMapTexture;
GLuint depthMapTexture2;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
	myBasicShader.useShaderProgram();

	// set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 5000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	
	// set Viewport transform
	glViewport(0, 0, width, height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
	int window_width, window_height;

	glfwGetWindowSize(window, &window_width, &window_height);

	//compute the mouse offset from the center of the screen
	float xoffset = xpos - window_width/2;
	float yoffset = window_height/2 - ypos; // reversed since y-coordinates range from bottom to top

	xoffset *= mouse_sensitivity;
	yoffset *= mouse_sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	myCamera.rotate(pitch, yaw);

	view = myCamera.getViewMatrix();
	myBasicShader.useShaderProgram();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	// compute normal matrix for teapot
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));

	//set the cursor back to the middle of the window
	glfwSetCursorPos(window, window_width/2, window_height/2);
}

void processMovement() {
	// rotate camera to the right
	if (pressedKeys[GLFW_KEY_C]) {
		cameraAngle += rotationSpeed;
		if (cameraAngle > 360.0f)
			cameraAngle -= 360.0f;

		myCamera.rotate(0, cameraAngle);

		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	// rotate camera to the left
	if (pressedKeys[GLFW_KEY_Z]) {
		cameraAngle -= rotationSpeed;
		if (cameraAngle < 0.0f)
			cameraAngle += 360.0f;
		myCamera.rotate(0, cameraAngle);

		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}


	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}
	rmat = glm::mat4(1.0f);

	if (pressedKeys[GLFW_KEY_2]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (pressedKeys[GLFW_KEY_3]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
	if (pressedKeys[GLFW_KEY_4]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (pressedKeys[GLFW_KEY_1]) {
		waspressed_fog = true;
	}
	else {
		if (waspressed_fog) {
			fogDensity += 0.015;
			if (fogDensity > 0.046)
			{
				fogDensity = 0;
			}
			myBasicShader.useShaderProgram();

			//fog density location
			fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
			glUniform1f(fogDensityLoc, (GLfloat)fogDensity);

		}
		waspressed_fog = false;
	}

	if (pressedKeys[GLFW_KEY_5]) {
		waspressed_point = true;
	}
	else
		if (waspressed_point) {

			myBasicShader.useShaderProgram();

			if (onPoint) {
				pointLightColor = glm::vec3(0.0f, 0.0f, 0.0f); //no light for off lights

				pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
				glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));
				onPoint = false;
			}
			else {
				onPoint = true;
				pointLightColor = glm::vec3(0.2f, 0.2f, 0.0f); //yellow light

				pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
				glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));

			}
			waspressed_point = false;
		}

	if (pressedKeys[GLFW_KEY_N]) {
		waspressed = true;
	}
	else
		if (waspressed) {

			myBasicShader.useShaderProgram();

			if (day) {
				lightColor = glm::vec3(0.05f, 0.05f, 0.3f); //blueish dim night light

				lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
				glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
				day = false;
			}
			else {
				day = true;
				lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light

				lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
				glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
			}
			waspressed = false;
		}

	rmat = glm::rotate(rmat, glm::radians(angleX), glm::vec3(0, 1, 0));
	rmat = glm::rotate(rmat, glm::radians(angleY), glm::vec3(1, 0, 0));

	if (pressedKeys[GLFW_KEY_Q] || pressedKeys[GLFW_KEY_E] || pressedKeys[GLFW_KEY_R] || pressedKeys[GLFW_KEY_F])
		model = glm::rotate(rmat, glm::radians((GLfloat)0), glm::vec3(1, 0, 0));

}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
	glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    // teapot.LoadModel("models/teapot/teapot20segUT.obj");
	scene.LoadModel("models/my_scene/my_Plane.obj");
	hour.LoadModel("models/my_scene/my_hour.obj");
	min.LoadModel("models/my_scene/my_min.obj");
	sec.LoadModel("models/my_scene/my_sec.obj");
	trumpet.LoadModel("models/my_scene/my_trumpet_placed.obj");
	bridge.LoadModel("models/my_scene/my_bridge_placed.obj");
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
	skyboxShader.loadShader(
		"shaders/skyboxShader.vert", 
		"shaders/skyboxShader.frag");
	lightShader.loadShader(
		"shaders/lightShader.vert",
		"shaders/lightShader.frag");
	depthMapShader.loadShader(
		"shaders/depthMapShader.vert", 
		"shaders/depthMapShader.frag");
}

void initFBO()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initFBO2()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO2);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture2);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO2);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture2, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initUniforms() {
	myBasicShader.useShaderProgram();


	lightDirMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDirMatrix");

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 300.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 150.0f, 150.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//point light 1 position
	//lightPos1 = glm::vec3(-24.96f, 3.414f, 19.47f); // position of light pole
	lightPos1 = glm::vec3(6.358771,3.479532,3.922943); // position of light pole
	lightPos1Loc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPosition");
	glUniform3fv(lightPos1Loc, 1, glm::value_ptr(lightPos1));
	
	//point light color
	pointLightColor = glm::vec3(0.0f, 0.0f, 0.0f); //point lights start as off
	pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
	// send light color to shader
	glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));

	//fog density location
	fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
	glUniform1f(fogDensityLoc, (GLfloat)fogDensity);

}

glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = 50.0f, far_plane = 300.0f;
	glm::mat4 lightProjection = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, near_plane, far_plane);

	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

glm::mat4 computePosLightSpaceTrMatrix()
{
	const GLfloat near_plane = 1.0f, far_plane = 100.0f;
	glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f),
		(float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
		near_plane, far_plane);

	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightPos1, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initSkyBox()
{
	std::vector<const GLchar*> faces;
	faces.push_back("textures/skybox/day/right.jpg");
	faces.push_back("textures/skybox/day/left.jpg");
	faces.push_back("textures/skybox/day/top.jpg");
	faces.push_back("textures/skybox/day/bottom.jpg");
	faces.push_back("textures/skybox/day/back.jpg");
	faces.push_back("textures/skybox/day/front.jpg");
	

	skyBoxDay.Load(faces);

	std::vector<const GLchar*> faces2;
	faces2.push_back("textures/skybox/night/nightsky0.png");
	faces2.push_back("textures/skybox/night/nightsky0.png");
	faces2.push_back("textures/skybox/night/nightsky0.png");
	faces2.push_back("textures/skybox/night/nightsky0.png");
	faces2.push_back("textures/skybox/night/nightsky0.png");
	faces2.push_back("textures/skybox/night/nightsky0.png");

	skyBoxNight.Load(faces2);
}
/*
void renderTeapot(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send teapot normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    //teapot.Draw(shader);
}
*/
void renderScene_env(gps::Shader shader) {
	// select active shader program
	shader.useShaderProgram();

	//send scene model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//send scene normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	// draw teapot
	scene.Draw(shader);
}

void renderHour(gps::Shader shader)
{

	shader.useShaderProgram();

	//send scene model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_hour));

	//send scene normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixHour));

	hour.Draw(shader);
}

void renderMin(gps::Shader shader)
{

	shader.useShaderProgram();

	//send scene model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_min));

	//send scene normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixMin));

	min.Draw(shader);
}

void renderSec(gps::Shader shader)
{

	shader.useShaderProgram();

	//send scene model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_sec));

	//send scene normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixSec));

	sec.Draw(shader);
}

void renderBridge(gps::Shader shader)
{

	shader.useShaderProgram();

	//send scene model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_bridge));

	//send scene normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixBridge));

	bridge.Draw(shader);
}

void renderTrumpet(gps::Shader shader)
{
	shader.useShaderProgram();

	//send scene model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_trumpet));

	//send scene normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrixTrumpet));

	trumpet.Draw(shader);
}

int br = 0, tr = 0;

void renderScene() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	depthMapShader.useShaderProgram();
	
	// compute shadows for directional light
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);

	glClear(GL_DEPTH_BUFFER_BIT);

	//compute shadows for objects
	//shadow for scene environment
	depthMapShader.useShaderProgram();
	glm::mat4 model_shadow = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0, 1.0, 0.0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model_shadow));
	renderScene_env(depthMapShader);

	////// rotatiiiiiiii
	glm::vec3 ax_tr, ax_br, ax_clk;
	ax_clk = glm::vec3(7.000524,2.800706,-2.404608)- glm::vec3(7.000524,2.800706,-2.400560);
	//shadow for hour tongue
	
	model_hour = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	model_hour = glm::translate(model_hour, glm::vec3(-6.528f, 0.0f, -5.305f));
	model_hour = glm::rotate(model_hour, glm::radians(angleHour), glm::vec3(0.0f, 1.0f, 0.0f));
	model_hour = glm::translate(model_hour, glm::vec3(6.528f, 0.0, 5.305f));
	normalMatrixHour = glm::mat3(glm::inverseTranspose(view * model_hour));
	
	renderHour(depthMapShader);

	//shadow for min tongue
	
	model_min = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	model_min = glm::translate(model_min, glm::vec3(-6.528f, 0.0f, -5.305f));
	model_min = glm::rotate(model_min, glm::radians(angleMin), glm::vec3(0.0f, 1.0f, 0.0f));
	model_min = glm::translate(model_min, glm::vec3(6.528f, 0.0, 5.305f));
	normalMatrixMin = glm::mat3(glm::inverseTranspose(view * model_min));

	renderMin(depthMapShader);

	//shadow for sec tongue
	model_sec = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	model_sec = glm::translate(model_sec, glm::vec3(-6.528f, 0.0f, -5.305f));
	model_sec = glm::rotate(model_sec, glm::radians(angleSec), glm::vec3(0.0f, 1.0f, 0.0f));
	model_sec = glm::translate(model_sec, glm::vec3(6.528f, 0.0, 5.305f));
	normalMatrixSec = glm::mat3(glm::inverseTranspose(view * model_sec));

	renderSec(depthMapShader);

	//shadow for trumpet
	
	ax_tr = glm::vec3(0.0, 0.0, 1.0);
	model_trumpet = glm::translate(glm::mat4(1.0f), glm::vec3(-2.181660, -4.080528, -5.187709));
	model_trumpet = glm::rotate(model_trumpet, glm::radians(angleTrumpet), ax_tr);
	model_trumpet = glm::translate(model_trumpet, glm::vec3(2.181660, 4.080528, 5.187709));
	normalMatrixTrumpet = glm::mat3(glm::inverseTranspose(view * model_trumpet));
	renderTrumpet(depthMapShader);

	//shadow for bridge
	ax_br = glm::vec3(-7.989387, 0.281455, 8.464755) - glm::vec3(-7.711543, 0.270463, 9.117801);
	model_bridge = glm::translate(glm::mat4(1.0f), glm::vec3(-7.989387, 0.281455, 8.464755));
	model_bridge = glm::rotate(model_bridge, glm::radians(angleBridge), ax_br);
	model_bridge = glm::translate(model_bridge, glm::vec3(7.989387, -0.281455, -8.464755));
	normalMatrixBridge = glm::mat3(glm::inverseTranspose(view * model_bridge));
	

	renderBridge(depthMapShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2nd step: render the scene

	myBasicShader.useShaderProgram();

	// send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

	// send view matrix to shader
	view = myCamera.getViewMatrix();

	glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	// compute light direction transformation matrix
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	// send lightDir matrix data to shader

	lightDirMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDirMatrix");
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

	glViewport(0, 0, (int)myWindow.getWindowDimensions().width, (int)myWindow.getWindowDimensions().height);
	myBasicShader.useShaderProgram();

	// bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

	//render the scene

	model = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
	renderScene_env(myBasicShader);

	// aici erau pt translatare aka miscare pe axa , nu cred ca mi trebe
	
	// update normal matrix for hour
	//model_hour = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	/*
	model_hour = glm::translate(glm::mat4(1.0f), glm::vec3(-7.003437,-2.802224,2.423554));
	model_hour = glm::rotate(model_hour, glm::radians(angleHour), ax_clk);
	model_hour = glm::translate(model_hour, glm::vec3(7.003437, 2.802224, -2.423554));
	normalMatrixHour = glm::mat3(glm::inverseTranspose(view * model_hour));
	
	//compute next angle for hour

	angleHour += 0.3f;
	if (angleHour > 360.0)
		angleHour -= 360;
		*/
	renderHour(myBasicShader);

	// update normal matrix for min
	//model_min = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), ax_clk);
	/*
	model_min = glm::translate(glm::mat4(1.0f), glm::vec3(-7.000524,-2.800706,2.400560));
	model_min = glm::rotate(model_min, glm::radians(angleMin), ax_clk);
	model_min = glm::translate(model_min, glm::vec3(7.000524, 2.800706, -2.400560));
	normalMatrixMin = glm::mat3(glm::inverseTranspose(view * model_min));

	//compute next angle for min

	angleMin += 0.3f;
	if (angleMin > 360.0)
		angleMin -= 360;
	*/
	renderMin(myBasicShader);

	// update normal matrix for sec
	/*
	model_sec = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	model_sec = glm::translate(model_sec, glm::vec3(-6.528f, 0.0f, -5.305f));
	model_sec = glm::rotate(model_sec, glm::radians(angleSec), glm::vec3(0.0f, 1.0f, 0.0f));
	model_sec = glm::translate(model_sec, glm::vec3(6.528f, 0.0, 5.305f));
	normalMatrixSec = glm::mat3(glm::inverseTranspose(view * model_sec));

	//compute next angle for sec

	angleSec += 0.3f;
	if (angleSec > 360.0)
		angleSec -= 360;
		*/
	renderSec(myBasicShader);

	// update normal matrix for bridge
	//model_bridge = glm::translate(model_bridge, glm::vec3(-7.989387, 0.281455, 8.464755)) * glm::translate(model_bridge, glm::vec3(7.989387, -0.281455, -8.464755));
	
	//model_bridge = glm::rotate(glm::mat4(1.0f), glm::radians((GLfloat)0), glm::vec3(0.0f, 1.0f, 0.0f));
	model_bridge = glm::translate(glm::mat4(1.0f), glm::vec3(-7.989387, 0.281455, 8.464755));
	model_bridge = glm::rotate(model_bridge, glm::radians(angleBridge), ax_br);
	model_bridge = glm::translate(model_bridge, glm::vec3(7.989387, -0.281455, -8.464755));
	normalMatrixBridge = glm::mat3(glm::inverseTranspose(view * model_bridge));
	//compute next angle for bridge

	if (br == 0) {
		angleBridge -= 0.1f;
		if (angleBridge < -75)
			br = 1;
	}
	else {
		angleBridge += 0.1f;
		if (angleBridge >= 0)
			br = 0;
	}

	renderBridge(myBasicShader);

	// update normal matrix for trumpet

	//glm::vec3 ax = glm::vec3(-7.989387, 0.281455, 8.464755) - glm::vec3(2.181660,4.080528,5.187709);
	model_trumpet = glm::translate(glm::mat4(1.0f), glm::vec3(-2.181660,-4.080528,-5.187709));
	model_trumpet = glm::rotate(model_trumpet, glm::radians(angleTrumpet), ax_tr);
	model_trumpet = glm::translate(model_trumpet, glm::vec3(2.181660, 4.080528, 5.187709));
	normalMatrixTrumpet = glm::mat3(glm::inverseTranspose(view * model_trumpet));

	//compute next angle for trumpet
	if (tr == 0) {
		angleTrumpet -= 0.01f;
		if (angleTrumpet < -1)
			tr = 1;
	}
	else {
		angleTrumpet += 0.01f;
		if (angleTrumpet >= 1)
			tr = 0;
	}

	renderTrumpet(myBasicShader);

	//render skybox
	if(day)
		skyBoxDay.Draw(skyboxShader, view, projection, fogDensity);
	else
		skyBoxNight.Draw(skyboxShader, view, projection, fogDensity);
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	rmat = glm::mat4(1.0f);

    initOpenGLState();
	initFBO();
	//initFBO2();
	initModels();
	initSkyBox();
	initShaders();
	initUniforms();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
