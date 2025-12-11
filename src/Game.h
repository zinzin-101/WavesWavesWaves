#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>

#include "Camera.h"
#include "Model.h"

#include <queue>
#include <map>
#include <vector>

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;
const int TARGET_FPS = 144;
const double MIN_TIME_PER_FRAME = 1.0 / (double)TARGET_FPS;

const double PI = 3.14159265358979323846;

// Game settings
const unsigned int WAVES_VERTS_WIDTH_NUM = 5000;
const float WAVES_VERTS_SCALE = 0.25f;
const float WAVES_SPEEDS[4] = { 3.0f, 5.0f, 3.0f, 6.0f };
const float WAVES_AMPLITUDES[4] = { 2.0f, 2.0f, 0.5f, 0.25f };
//const float WAVES_LENGTH = 0.25f;
const glm::vec3 WAVES_DIRECTIONS[4] = { glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.f), glm::vec3(-1.0f, 0.0f, -0.8f) };
const float WAVES_LENGTHS[4] = { 20.0f, 10.0f, 5.0f, 2.5f };
const float BOAT_HEIGHT_DAMPING_FACTOR = 0.95f;
const float BOAT_HEIGHT_FLOATING_OFFSET = -0.5f;
const float BOAT_HEIGHT_LERP_SPEED = 5.0f;
const float BOAT_ROTATION_SPEED = 0.125f;
const unsigned int WAVES_SAMPLE_GRID_SIZE = 5;
const float WAVES_SAMPLE_SPACING = 0.25f;
const unsigned int MAX_OTHER_BOATS_COUNT = 6;
const unsigned int MAX_OTHER_BOATS_FOLLOW_COUNT = 1;
const float MIN_OTHER_BOAT_START_DISTANCE_FROM_PLAYER = 50.0f;
const float MAX_OTHER_BOAT_START_DISTANCE_FROM_PLAYER = 400.0f;
const float MAX_OTHER_BOAT_DISTANCE_TO_CHANGE_DIRECTION = 600.0f;
const float MIN_DISTANCE_BETWEEN_OTHER_BOATS = 45.0f;
const float MIN_DISTANCE_FROM_PLAYER = 50.0f;
const float OTHER_BOAT_SPEED = 8.0f;
const float BOAT_COLLISION_DISTANCE = 28.0f;
const float BOAT_FLIP_SPEED = 1.0f;

// Player settings
const float FOV = 60;
const float FREE_CAM_FAST_MOVE_SPEED = 50;
const float FREE_CAM_MOVE_SPEED = 10;
const float CAM_LERP_SPEED = 10.0f;
const float MAX_CAM_DISTANCE = 125.0f;
const float MIN_CAM_DISTANCE = 20.0f;
const float DEFAULT_CAM_DISTANCE = (MAX_CAM_DISTANCE - MIN_CAM_DISTANCE) * 0.5f + MIN_CAM_DISTANCE;
const float BOAT_TURN_RATE = 0.5f;
const float BOAT_SPEED = 12.0f;
const float BOAT_DRAG = 5.0f;

struct Boat {
	Boat(): 
		position(glm::vec3(0.0f)), forward(glm::vec3(0.0f, 0.0f, 1.0f)), right(glm::vec3(1.0f, 0.0f, 0.0f)), up(glm::vec3(0.0f, 1.0f, 0.0f)), 
		currentBearing(glm::vec3(0.0f, 0.0f, 1.0f)), speed(0.0f), isFlipped(false), t_flip(0.0f), followPlayer(false), destDir(glm::vec3(0.0f, 0.0f, 1.0f)) {}
	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 currentBearing;
	float speed;
	bool isFlipped;
	float t_flip;
	bool followPlayer;
	glm::vec3 destDir;
};

struct BoxCollider {
	BoxCollider(): ownerId(-1), offset(0.0f), size(1.0f) {}
	int ownerId;
	glm::vec3 offset;
	glm::vec3 size;
};

struct Physics {
	Physics(): ownerId(-1), lastPosition(0.0f), acceleration(0.0f) {}
	int ownerId;
	glm::vec3 lastPosition;
	glm::vec3 acceleration;
};

class Game {
	private:
		Shader wavesShader;
		Shader outlineShader;
		Shader skyboxShader;
		Shader objectShader;
		Shader flatShader;

		GLuint cubeVAO, cubeVBO, cubeEBO;

		unsigned int cubeMapTexture;
		GLuint skyboxVAO, skyboxVBO, skyboxEBO;
		GLuint outlineVAO, outlineVBO, outlineEBO;

		unsigned int wavesStripCount, wavesVertsPerStrip;
		GLuint wavesVAO, wavesVBO, wavesEBO;
		float wavesTime;
		glm::vec3 waveDirections[12];

		glm::mat4 boatToWorld;

		Model boatModel;
		glm::vec3 boatPosition;
		glm::vec3 boatForward;
		glm::vec3 boatRight;
		glm::vec3 boatUp;

		glm::vec3 currentBoatBearing;
		float boatSpeed;

		std::vector<Boat> otherBoats;

		float dt;

		Camera freeCamera;
		Camera boatCamera;
		Camera* currentCamera;
		float boatCameraDistance;
		float boatCameraHeight;
		bool isAdjustingHeight;
		void updateBoatCamera();
		
		std::map<unsigned int, bool> keyDown;
		bool handleKeyDown(GLFWwindow* window, unsigned int key);

		void initOtherBoats();
		void updateOtherBoats();
		void renderOtherBoats();

		void initSkybox();
		void drawSkybox();

		void initWaves();
		void drawWaves();

		void initColliderOutline();
		void init();

		void initCube();
		void drawCube();

		glm::vec3 getBoatPositionFromWaves(glm::vec3 position, glm::vec3& normal);
		glm::vec3 getAverageBoatPositionFromWaves(glm::vec3& normal);
		glm::vec3 getAverageBoatPositionFromWaves(glm::vec3 position, glm::vec3& normal);

		void moveBoat(glm::vec3 direction);
		void moveBoat(Boat& boat, glm::vec3 direction);

		glm::mat4 getProjection() const;

		void accelerate(Physics& phys, glm::vec3 a);
		void setVelocity(Physics& phys, glm::vec3 vel, float dt);
		void addVelocity(Physics& phys, glm::vec3 vel, float dt);
		glm::vec3 getVelocity(Physics& phys, float dt);
		void computePhysics(float dt);
		
	public:
		Game();
		unsigned int getCubeMapTexture(std::string cubeMapPath[]);
		
		void render(float dt);
		void update(float dt);

		void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
		void processMouseScroll(float yoffset);
		void processMouseButton(int button, int action);
		void processKeyboard(GLFWwindow* window, float dt);
};