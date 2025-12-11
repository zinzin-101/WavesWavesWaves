#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
//enum Camera_Movement {
//    FORWARD,
//    BACKWARD,
//    LEFT,
//    RIGHT,
//    UP,
//    DOWN
//};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Forward;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;


    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // lerp option
    bool UseLerp;
    float LerpSpeed;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix();

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    //void ProcessKeyboard(Camera_Movement direction, float deltaTime);

    void ProcessKeyboard(glm::vec3 movement, float dt);

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);

    void SetForwardVector(glm::vec3 forward);

    void UpdateLerp(float dt);

    glm::vec3 getPosition();

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();

    glm::vec3 currentLerpPosition;
    glm::vec3 currentLerpForward;
    glm::vec3 currentLerpUp;
    glm::vec3 currentLerpRight;
    glm::vec3 currentLerpWorldUp;
};