#include "Game.h"
#include "Random.h"
#include "VerticesData.h"
#include <learnopengl/filesystem.h>
#include <vector>
#include <cmath>
#include <ostream>

static ostream& operator<<(ostream& out, const glm::vec3& v);

//bool RenderComparator::operator()(const RenderingObject& obj1, const RenderingObject& obj2) {
//    return obj1.distanceFromCamera > obj2.distanceFromCamera;
//}

Game::Game() :
    wavesShader("waves.vs", "waves.fs"),
    outlineShader("collider_outline.vs", "collider_outline.fs"),
    skyboxShader("skybox.vs", "skybox.fs"),
    objectShader("vertex.vs", "fragment.fs"),
    flatShader("flat.vs", "flat.fs"),
    boatModel(FileSystem::getPath("resources/objects/boat/boat.dae"))
{
    Random::init();
    init();
}

unsigned int Game::getCubeMapTexture(std::string cubeMapPath[]) {
    unsigned int cubeMapTex;
    glGenTextures(1, &cubeMapTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // prevent seam
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    for (unsigned int i = 0; i < 6; i++)
    {
        int width, height, nrChannels;
        unsigned char* data = stbi_load(cubeMapPath[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D
            (
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                GL_RGB,
                width,
                height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load texture: " << cubeMapPath[i] << std::endl;
            stbi_image_free(data);
        }
    }

    return cubeMapTex;
}

void Game::initOtherBoats() {
    for (int i = 0; i < MAX_OTHER_BOATS_COUNT; i++) {
        float x = Random::randFloat(MAX_OTHER_BOAT_START_DISTANCE_FROM_PLAYER) + boatPosition.x;
        float z = Random::randFloat(MAX_OTHER_BOAT_START_DISTANCE_FROM_PLAYER) + boatPosition.z;
        glm::vec3 spawnPos = glm::vec3(x, 0.0f, z);
        glm::vec3 playerPos = boatPosition;
        playerPos.y = 0.0f;
        float distance = glm::length(playerPos - spawnPos);
        while (distance < MIN_OTHER_BOAT_START_DISTANCE_FROM_PLAYER) {
            x = Random::randFloat(MAX_OTHER_BOAT_START_DISTANCE_FROM_PLAYER) + boatPosition.x;
            z = Random::randFloat(MAX_OTHER_BOAT_START_DISTANCE_FROM_PLAYER) + boatPosition.z;
            spawnPos = glm::vec3(x, 0.0f, z);
            distance = glm::length(playerPos - spawnPos);
        }

        Boat boat;
        boat.position = spawnPos;
        boat.currentBearing = playerPos - spawnPos;
        boat.currentBearing.y = 0.0f;
        glm::vec3 forwardYaw = glm::normalize(boat.currentBearing);
        boat.right = glm::normalize(glm::cross(forwardYaw, glm::vec3(0.0f, 1.0f, 0.0f)));
        boat.forward = glm::normalize(glm::cross(boat.up, boat.right));

        if (i < MAX_OTHER_BOATS_FOLLOW_COUNT) boat.followPlayer = true;
        else {
            glm::vec3 destDir = glm::vec3(
                Random::randFloat(1.0f),
                0.0f,
                Random::randFloat(1.0f)
            );
            boat.destDir = destDir;
        }

        otherBoats.emplace_back(boat);
    }
}

void Game::initSkybox() {
    std::string cubeMapFaces[6] =
    {
        FileSystem::getPath("resources/objects/skybox3/right.jpg"),
        FileSystem::getPath("resources/objects/skybox3/left.jpg"),
        FileSystem::getPath("resources/objects/skybox3/top.jpg"),
        FileSystem::getPath("resources/objects/skybox3/bottom.jpg"),
        FileSystem::getPath("resources/objects/skybox3/front.jpg"),
        FileSystem::getPath("resources/objects/skybox3/back.jpg")
    };

    cubeMapTexture = getCubeMapTexture(cubeMapFaces);

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SKYBOX_VERTICES), &SKYBOX_VERTICES, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(SKYBOX_INDICES), &SKYBOX_INDICES, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Game::initColliderOutline() {
    glGenVertexArrays(1, &outlineVAO);
    glGenBuffers(1, &outlineVBO);
    glGenBuffers(1, &outlineEBO);

    glBindVertexArray(outlineVAO);

    glBindBuffer(GL_ARRAY_BUFFER, outlineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outlineEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Game::initWaves() {
    // create flat plane vertex and indices
    std::vector<float> verts;
    float offset = (float)WAVES_VERTS_WIDTH_NUM / 2.0f;
    for (int x = 0; x < WAVES_VERTS_WIDTH_NUM; x++) {
        for (float z = 0; z < WAVES_VERTS_WIDTH_NUM; z++) {
            verts.emplace_back(((float)x - offset) * WAVES_VERTS_SCALE);
            verts.emplace_back(0.0f);
            verts.emplace_back(((float)z - offset) * WAVES_VERTS_SCALE);
        }
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < WAVES_VERTS_WIDTH_NUM - 1; i++) {
        for (unsigned int j = 0; j < WAVES_VERTS_WIDTH_NUM; j++) {
            for (unsigned int k = 0; k < 2; k++) {
                indices.emplace_back(j + WAVES_VERTS_WIDTH_NUM * (i + k));
            }
        }
    }

    wavesStripCount = WAVES_VERTS_WIDTH_NUM - 1;
    wavesVertsPerStrip = WAVES_VERTS_WIDTH_NUM * 2;

    // bind VAO
    glGenVertexArrays(1, &wavesVAO);
    glBindVertexArray(wavesVAO);

    // generate VBO
    glGenBuffers(1, &wavesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, wavesVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        verts.size() * sizeof(float),
        verts.data(),
        GL_STATIC_DRAW
    );

    // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    // normals
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    //glEnableVertexAttribArray(1);

    // generate EBO
    glGenBuffers(1, &wavesEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wavesEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    wavesTime = 0.0f;
    for (int i = 0; i < 12; i++) {
        waveDirections[i] = glm::vec3(Random::randFloat(1.0f), 0.0f, Random::randFloat(1.0f));
    }
}

void Game::drawWaves() {
    glBindVertexArray(wavesVAO);
    for (unsigned int i = 0; i < wavesStripCount; i++) {
        glDrawElements(
            GL_TRIANGLE_STRIP,
            wavesVertsPerStrip,
            GL_UNSIGNED_INT,
            (void*)(sizeof(unsigned int) * wavesVertsPerStrip * i)
        );
    }
}

void Game::init() {
    //GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    //const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    //int refreshRate = mode->refreshRate;

    initSkybox();
    initColliderOutline();
    
    initWaves();

    initCube();

    boatPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    boatForward = glm::vec3(0.0f, 0.0f, 1.0f);
    boatRight = glm::vec3(1.0f, 0.0f, 0.0f);
    boatUp = glm::vec3(0.0f, 1.0f, 0.0f);

    freeCamera.Position = glm::vec3(0.0f, 0.0f, 0.0f);
    boatCamera.Position = boatPosition;
    boatCamera.LerpSpeed = CAM_LERP_SPEED;
    boatCamera.UseLerp = true;
    boatCameraDistance = DEFAULT_CAM_DISTANCE;
    boatCameraHeight = 0.0f;

    currentCamera = &boatCamera;

    currentBoatBearing = glm::vec3(0.0f, 0.0f, 1.0f);

    updateBoatCamera();

    initOtherBoats();

    boatToWorld =
        glm::scale(glm::mat4(1.0f), glm::vec3(0.015f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
}

void Game::initCube() {
    // bind VAO
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);

    // generate VBO
    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        8 * sizeof(float) * 3,
        CUBE_VERTICES,
        GL_STATIC_DRAW
    );

    // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // generate EBO
    glGenBuffers(1, &cubeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        36 * sizeof(unsigned int),
        SKYBOX_INDICES,
        GL_STATIC_DRAW
    );
}

void Game::drawCube() {
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

glm::vec3 Game::getBoatPositionFromWaves(glm::vec3 position, glm::vec3& normal) {
    const int NUM_OF_SINE_WAVES = 36;
    glm::vec3 pos = position;

    float height = 0.0f;
    float dx = 0.0f;
    float dz = 0.0f;

    float b_a = 1.0f;
    float b_f = 1.0f;

    for (int i = 0; i < NUM_OF_SINE_WAVES; i++) {
        glm::vec3 dir = normalize(waveDirections[i % 12]);
        float frequency = 2.0f / WAVES_LENGTHS[i % 4];
        float phase = WAVES_SPEEDS[i % 4] * (2.0f / WAVES_LENGTHS[i % 4]);

        float a = b_a * WAVES_AMPLITUDES[i % 4];
        float f = b_f * frequency;

        float dotPhase = (dir.x * pos.x + dir.z * pos.z) * f + wavesTime * phase;
        float sine = sin(dotPhase);
        float cosine = cos(dotPhase);
        float exponent = exp(sine - 1.0f);

        height += a * exponent;
        dx += dir.x * a * cosine * exponent * f;
        dz += dir.z * a * cosine * exponent * f;

        b_a *= 0.92f;
        b_f *= 1.08f;
    }

    //std::cout << "wave height: " << height << std::endl;
    
    //return glm::vec3(0.0f, -1.5f, 0.0f);

    pos.x += dx;
    pos.z += dz;

    height *= BOAT_HEIGHT_DAMPING_FACTOR;
    height += BOAT_HEIGHT_FLOATING_OFFSET;

    normal = glm::normalize(glm::vec3(-dx, 1.0f, -dz));

    return glm::vec3(pos.x, height, pos.z);
}

glm::vec3 Game::getAverageBoatPositionFromWaves(glm::vec3& normal) {
    glm::vec3 sum = glm::vec3(0.0f);
    int sampleCount = 0;

    glm::vec3 normalSum = glm::vec3(0.0f);
    glm::vec3 tempNormal;
    //getBoatPositionFromWaves(boatPosition, normal);
    //normalSum += tempNormal;
    //sampleCount++;
    
    float startingOffset = -WAVES_SAMPLE_SPACING * ((float)(WAVES_SAMPLE_GRID_SIZE - 1) / 2.0f);
    for (int i = 0; i < WAVES_SAMPLE_GRID_SIZE; i++) {
        for (int j = 0; j < WAVES_SAMPLE_GRID_SIZE; j++) {
            float x = startingOffset + WAVES_SAMPLE_SPACING * (float)i + boatPosition.x;
            float z = startingOffset + WAVES_SAMPLE_SPACING * (float)j + boatPosition.z;
            glm::vec3 tempPos = glm::vec3(x, 0.0f, z);
            sum += getBoatPositionFromWaves(tempPos, tempNormal);
            normalSum += tempNormal;
            sampleCount++;
        }
    }

    float height = sum.y / (float)sampleCount;
    glm::vec3 pos = glm::vec3(boatPosition.x, height, boatPosition.z);
    normal = glm::normalize(normalSum / (float)sampleCount);

    return pos;
}

glm::vec3 Game::getAverageBoatPositionFromWaves(glm::vec3 position, glm::vec3& normal) {
    glm::vec3 sum = glm::vec3(0.0f);
    int sampleCount = 0;

    glm::vec3 normalSum = glm::vec3(0.0f);
    glm::vec3 tempNormal;
    //getBoatPositionFromWaves(boatPosition, normal);
    //normalSum += tempNormal;
    //sampleCount++;

    float startingOffset = -WAVES_SAMPLE_SPACING * ((float)(WAVES_SAMPLE_GRID_SIZE - 1) / 2.0f);
    for (int i = 0; i < WAVES_SAMPLE_GRID_SIZE; i++) {
        for (int j = 0; j < WAVES_SAMPLE_GRID_SIZE; j++) {
            float x = startingOffset + WAVES_SAMPLE_SPACING * (float)i + position.x;
            float z = startingOffset + WAVES_SAMPLE_SPACING * (float)j + position.z;
            glm::vec3 tempPos = glm::vec3(x, 0.0f, z);
            sum += getBoatPositionFromWaves(tempPos, tempNormal);
            normalSum += tempNormal;
            sampleCount++;
        }
    }

    float height = sum.y / (float)sampleCount;
    glm::vec3 pos = glm::vec3(position.x, height, position.z);
    normal = glm::normalize(normalSum / (float)sampleCount);

    return pos;
}

void Game::moveBoat(glm::vec3 direction) {
    direction.y = 0.0f;
    glm::vec3 currentForward = currentBoatBearing;
    currentForward.y = 0.0f;
    direction = glm::normalize(direction);
    currentForward = glm::normalize(currentForward);
    float dirDot = glm::dot(direction, currentForward);
    if (dirDot < 0.0f) {
        float angle = acos(dirDot);
        glm::vec3 currentRight = glm::normalize(glm::cross(currentForward, glm::vec3(0.0f, 1.0f, 0.0f)));
        float angleFromRight = acos(glm::dot(currentRight, direction));
        float angleFromLeft = acos(glm::dot(-currentRight, direction));

        direction = glm::normalize((abs(angleFromRight) < abs(angleFromLeft) ? currentRight : -currentRight));
    }
    glm::vec3 v = direction - currentForward;
    float difference = glm::length(v);
    glm::vec3 normalized = difference < 0.0001f ? direction : glm::normalize(v);
    currentForward += normalized * glm::clamp(difference, 0.0f, BOAT_TURN_RATE * dt);
    currentForward = glm::normalize(currentForward);
    currentBoatBearing = currentForward;
    //boatPosition += currentForward * BOAT_SPEED * dt;
    boatSpeed = BOAT_SPEED;
}

void Game::moveBoat(Boat& boat, glm::vec3 direction) {
    direction.y = 0.0f;
    glm::vec3 currentForward = boat.currentBearing;
    currentForward.y = 0.0f;
    direction = glm::normalize(direction);
    currentForward = glm::normalize(boat.forward);
    float dirDot = glm::dot(direction, currentForward);
    if (dirDot < 0.0f) {
        float angle = acos(dirDot);
        glm::vec3 currentRight = glm::normalize(glm::cross(currentForward, glm::vec3(0.0f, 1.0f, 0.0f)));
        float angleFromRight = acos(glm::dot(currentRight, direction));
        float angleFromLeft = acos(glm::dot(-currentRight, direction));

        direction = glm::normalize((abs(angleFromRight) < abs(angleFromLeft) ? currentRight : -currentRight));
    }
    glm::vec3 v = direction - currentForward;
    float difference = glm::length(v);
    glm::vec3 normalized = difference < 0.0001f ? direction : glm::normalize(v);
    currentForward += normalized * glm::clamp(difference, 0.0f, BOAT_TURN_RATE * dt);
    currentForward = glm::normalize(currentForward);
    boat.currentBearing = currentForward;
    boat.speed = BOAT_SPEED;
}

void Game::updateOtherBoats() {
    for (Boat& boat : otherBoats) {
        glm::vec3 surfaceNormal;
        glm::vec3 target = getAverageBoatPositionFromWaves(boat.position, surfaceNormal);
        glm::vec3 current = boat.position;
        glm::vec3 moveVec = target - current;
        float distance = glm::length(moveVec);
        glm::vec3 moveDir = distance > 0.0001f ? glm::normalize(moveVec) : glm::vec3(0.0f);
        float moveAmount = clamp(distance, 0.0f, BOAT_HEIGHT_LERP_SPEED * dt);
        boat.position += moveDir * moveAmount;

        glm::vec3 upMove = surfaceNormal - boat.up;
        float difference = glm::length(upMove);
        boat.up += glm::normalize(upMove) * clamp(difference, 0.0f, BOAT_ROTATION_SPEED * dt);

        glm::vec3 forwardYaw = glm::normalize(boat.currentBearing);
        boat.right = glm::normalize(glm::cross(forwardYaw, boat.up));
        boat.forward = glm::normalize(glm::cross(boat.up , boat.right));

        if (boat.speed > 0.0f) {
            boat.position += glm::normalize(boat.currentBearing) * boat.speed * dt;
            boat.speed -= BOAT_DRAG * dt;
        }

        if (boat.isFlipped && boat.t_flip < 1.0f) boat.t_flip += BOAT_FLIP_SPEED * dt;
        else if (boat.isFlipped && boat.t_flip > 1.0f) boat.t_flip = 1.0f;
    }

    for (int i = 0; i < MAX_OTHER_BOATS_COUNT; i++) {
        Boat& current = otherBoats[i];

        if (current.isFlipped) continue;

        glm::vec3 toPlayer = boatPosition - current.position;
        float distanceFromPlayer = glm::length(toPlayer);

        //float playerDot = glm::dot(glm::normalize(toPlayer), glm::normalize(current.forward));
        //if (distanceFromPlayer < BOAT_COLLISION_DISTANCE && abs(playerDot) < 0.1f) {
        //    current.isFlipped = true;
        //    continue;
        //}

        bool canMoveTowardPlayer = true;
        for (int j = 0; j < MAX_OTHER_BOATS_COUNT; j++) {
            if (i == j) continue;

            Boat& other = otherBoats[j];
            glm::vec3 toOther = other.position - current.position;
            float distance = glm::length(toOther);
            if (distance < MIN_DISTANCE_BETWEEN_OTHER_BOATS) {
                moveBoat(current, -toOther);
                canMoveTowardPlayer = false;
                break;
            }
        }

        if (!current.followPlayer) {
            if (distanceFromPlayer > MAX_OTHER_BOAT_DISTANCE_TO_CHANGE_DIRECTION) {
                current.destDir = toPlayer;
                current.destDir.y = 0.0f;
            }
            else if (distanceFromPlayer < MIN_DISTANCE_FROM_PLAYER) {
                moveBoat(current, -toPlayer);
                continue;
            }

            moveBoat(current, current.destDir);
            continue;
        }

        if (canMoveTowardPlayer && distanceFromPlayer > MIN_DISTANCE_FROM_PLAYER) moveBoat(current, toPlayer);
    }
}

void Game::update(float dt) {
    //std::cout << "cam view dir: " << camera.Forward << std::endl;
    this->dt = dt;
    wavesTime += dt;

    if (currentCamera == &boatCamera) {
        boatCamera.UpdateLerp(dt);
        updateBoatCamera();
    }

    glm::vec3 temp;
    glm::vec3 camPosAtWaves = getBoatPositionFromWaves(currentCamera->getPosition(), temp);
    if (currentCamera->Position.y < camPosAtWaves.y) currentCamera->Position.y = camPosAtWaves.y;

    glm::vec3 surfaceNormal;
    glm::vec3 target = getAverageBoatPositionFromWaves(surfaceNormal);
    glm::vec3 current = boatPosition;
    glm::vec3 moveVec = target - current;
    float distance = glm::length(moveVec);
    glm::vec3 moveDir = distance > 0.0001f ? glm::normalize(moveVec) : glm::vec3(0.0f);
    float moveAmount = clamp(distance, 0.0f, BOAT_HEIGHT_LERP_SPEED * dt);
    boatPosition += moveDir * moveAmount;

    glm::vec3 upMove = surfaceNormal - boatUp;
    float difference = glm::length(upMove);
    boatUp += glm::normalize(upMove) * clamp(difference, 0.0f, BOAT_ROTATION_SPEED * dt);

    glm::vec3 forwardYaw = glm::normalize(currentBoatBearing);
    boatRight = glm::normalize(glm::cross(forwardYaw, boatUp));
    boatForward = glm::normalize(glm::cross(boatUp, boatRight));

    if (boatSpeed > 0.0f) {
        boatPosition += glm::normalize(currentBoatBearing) * boatSpeed * dt;
        boatSpeed -= BOAT_DRAG * dt;
    }

    updateOtherBoats();
}

glm::mat4 Game::getProjection() const {
    return glm::perspective(glm::radians(FOV), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
}

void Game::renderOtherBoats() {
    objectShader.use();
    for (Boat& boat : otherBoats) {
        glm::mat4 projection = getProjection();
        glm::mat4 view = currentCamera->GetViewMatrix();

        glm::mat4 boatRotMat(
            glm::vec4(boat.right, 0.0f),
            glm::vec4(boat.up, 0.0f),
            glm::vec4(-boat.forward, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );
        
        glm::mat4 boatFlipMat = 
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, (boat.isFlipped ? 3.0f : 0.0f), 0.0f)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f * boat.t_flip), boat.forward);

        objectShader.setMat4("projection", projection);
        objectShader.setMat4("view", view);

        objectShader.setMat4("model", glm::translate(glm::mat4(1.0f), boat.position) * boatRotMat * boatFlipMat * boatToWorld);

        objectShader.setVec3("viewPos", currentCamera->getPosition());
        objectShader.setVec3("dirLight.direction", glm::vec3(-0.486897f, -0.0627906f, 0.8712f));
        objectShader.setVec3("dirLight.ambient", glm::vec3(0.4f));
        objectShader.setVec3("dirLight.diffuse", glm::vec3(0.6f));
        objectShader.setVec3("dirLight.specular", glm::vec3(0.9f));

        boatModel.Draw(objectShader);
    }
}

void Game::render(float dt) {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = getProjection();
    glm::mat4 view = currentCamera->GetViewMatrix();

    // boat
    glm::mat4 boatRotMat(
        glm::vec4(boatRight, 0.0f),
        glm::vec4(boatUp, 0.0f),
        glm::vec4(-boatForward, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    );

    objectShader.use();
    objectShader.setMat4("projection", projection);
    objectShader.setMat4("view", view);

    objectShader.setMat4("model", glm::translate(glm::mat4(1.0f), boatPosition) * boatRotMat * boatToWorld);

    objectShader.setVec3("viewPos", currentCamera->getPosition());
    objectShader.setVec3("dirLight.direction", glm::vec3(-0.486897f, -0.0627906f, 0.8712f));
    objectShader.setVec3("dirLight.ambient", glm::vec3(0.4f));
    objectShader.setVec3("dirLight.diffuse", glm::vec3(0.6f));
    objectShader.setVec3("dirLight.specular", glm::vec3(0.9f));

    boatModel.Draw(objectShader);

    renderOtherBoats();

    //objectShader.setMat4("model", glm::mat4(1.0f));
    //woodenBoatModel.Draw(objectShader);

    // skybox
    skyboxShader.use();
    glm::mat4 skyboxView = glm::mat4(1.0f);
    glm::mat4 skyboxProjection = glm::mat4(1.0f);
    skyboxView = glm::mat4((glm::mat3)currentCamera->GetViewMatrix());
    skyboxProjection = getProjection();
    skyboxShader.setMat4("view", skyboxView);
    skyboxShader.setMat4("projection", skyboxProjection);
    drawSkybox();

    glm::vec3 waterColor = glm::vec3(0.11372549019f, 0.63529411764f, 0.84705882352f);
    //glm::vec3 waterColor = glm::vec3(1,1,1);

    wavesShader.use();
    // view/projection transformations
    glm::vec3 camPos = currentCamera->getPosition();
    wavesShader.setVec3("camOffset", camPos);
    wavesShader.setMat4("projection", projection);
    wavesShader.setMat4("view", view);
    wavesShader.setMat4("model", glm::mat4(1.0f) * glm::translate(glm::mat4(1.0f), glm::vec3(camPos.x, 0.0f, camPos.z)));
    wavesShader.setVec3("viewPos", currentCamera->getPosition());
    wavesShader.setVec3("color", waterColor);
    wavesShader.setBool("useLighting", true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
    wavesShader.setInt("skybox", 0);
    wavesShader.setFloat("skyboxBlendAmount", 0.6f);
    wavesShader.setFloat("time", wavesTime);
    for (int i = 0; i < 36; i++) {
        std::string indexString = std::to_string(i);
        std::string amplitude = "amplitude[" + indexString + "]";
        std::string wavelength = "wavelength[" + indexString + "]";
        std::string speed = "speed[" + indexString + "]";
        std::string direction = "direction[" + indexString + "]";
        wavesShader.setVec3(direction, waveDirections[i % 12]);
        wavesShader.setFloat(amplitude, WAVES_AMPLITUDES[i % 4]);
        wavesShader.setFloat(wavelength, WAVES_LENGTHS[i % 4]);
        wavesShader.setFloat(speed, WAVES_SPEEDS[i % 4]);
    }

    //glm::vec3 lightPos(0.0f, 50.0f, 0.0f);
    wavesShader.setVec3("dirLight.direction", glm::vec3(-0.486897f, -0.0627906f, 0.8712f));
    wavesShader.setVec3("dirLight.ambient", glm::vec3(0.4f));
    wavesShader.setVec3("dirLight.diffuse", glm::vec3(0.6f));
    wavesShader.setVec3("dirLight.specular", glm::vec3(0.9f));
    wavesShader.setFloat("shininess", 16.0f);

    wavesShader.setFloat("foamThreshold", 0.0001f);
    wavesShader.setFloat("foamIntensity", 1.0f);
    wavesShader.setBool("showFoam", true);

    drawWaves();

    flatShader.use();
    flatShader.setMat4("model", 
        glm::translate(glm::mat4(1.0), glm::vec3(camPos.x, -50.0f, camPos.z)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(100000.0f, 0.1f, 100000.0f)));
    flatShader.setMat4("view", view);
    flatShader.setMat4("projection", projection);
    flatShader.setVec3("color", waterColor * 0.4f);
    //drawCube();
}

void Game::drawSkybox() {
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Game::updateBoatCamera() {
    glm::vec3 pos = boatPosition - boatCamera.Forward * boatCameraDistance;
    pos.y += boatCameraHeight;
    boatCamera.Position = pos;
}

void Game::processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    if (currentCamera == &boatCamera && isAdjustingHeight) {
        boatCameraHeight += yoffset * dt;
        return;
    }

    freeCamera.ProcessMouseMovement(xoffset, yoffset);
    boatCamera.ProcessMouseMovement(xoffset, yoffset);
}

void Game::processMouseScroll(float yoffset) {
    if (currentCamera != &boatCamera) return;
    boatCameraDistance -= yoffset;
    boatCameraDistance = glm::clamp(boatCameraDistance, MIN_CAM_DISTANCE, MAX_CAM_DISTANCE);
}

void Game::processMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) isAdjustingHeight = true;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) isAdjustingHeight = false;
}

bool Game::handleKeyDown(GLFWwindow* window, unsigned int key) {
    if (keyDown.count(key) == 0) { // for initialization
        keyDown[key] = false;
        return false;
    }

    if (glfwGetKey(window, key) == GLFW_PRESS && !keyDown.at(key)) {
        keyDown[key] = true;
        return true;
    }
    
    if (glfwGetKey(window, key) == GLFW_RELEASE) keyDown[key] = false;
    return false;
}

void Game::processKeyboard(GLFWwindow* window, float dt) {
    if (currentCamera == &freeCamera) {
        glm::vec3 movement = glm::vec3();
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            movement += glm::vec3(0, 0, 1);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            movement += glm::vec3(0, 0, -1);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            movement += glm::vec3(-1, 0, 0);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            movement += glm::vec3(1, 0, 0);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            movement += glm::vec3(0, 1, 0);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            movement += glm::vec3(0, -1, 0);

        movement *= glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? FREE_CAM_FAST_MOVE_SPEED : FREE_CAM_MOVE_SPEED;
        freeCamera.ProcessKeyboard(movement, dt);
    }

    if (currentCamera == &boatCamera) {
        glm::vec3 movement = glm::vec3();
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            movement += currentCamera->Forward;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            movement -= currentCamera->Forward;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            movement -= currentCamera->Right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            movement += currentCamera->Right;
        if (glm::length(movement) > 0.0f) moveBoat(movement);
    }

    if (handleKeyDown(window, GLFW_KEY_V)) {
        Camera* lastCamera = currentCamera;
        currentCamera = currentCamera == &freeCamera ? &boatCamera : &freeCamera;
        currentCamera->SetForwardVector(lastCamera->Forward);

        if (currentCamera == &freeCamera) currentCamera->Position = lastCamera->Position;
    }
        

}

ostream& operator<<(ostream& out, const glm::vec3& v) {
    out << v.x << " " << v.y << " " << v.z;
    return out;
}