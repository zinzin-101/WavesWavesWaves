#include "Camera.h"

// constructor with vectors
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Forward(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), UseLerp(false)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;

    updateCameraVectors();
    
    currentLerpPosition = Position;
    currentLerpWorldUp = WorldUp;
    currentLerpUp = Up;
    currentLerpForward = Forward;
    currentLerpRight = Right;
}
// constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Forward(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), UseLerp(false)
{
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;

    updateCameraVectors();

    currentLerpPosition = Position;
    currentLerpWorldUp = WorldUp;
    currentLerpUp = Up;
    currentLerpForward = Forward;
    currentLerpRight = Right;
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 Camera::GetViewMatrix()
{
    if (UseLerp) {
        return glm::lookAt(currentLerpPosition, currentLerpPosition + currentLerpForward, currentLerpUp);
    }
    return glm::lookAt(Position, Position + Forward, Up);
}

// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
//void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
//{
//    float velocity = MovementSpeed * deltaTime;
//    if (direction == FORWARD)
//        Position += Forward * velocity;
//    if (direction == BACKWARD)
//        Position -= Forward * velocity;
//    if (direction == LEFT)
//        Position -= Right * velocity;
//    if (direction == RIGHT)
//        Position += Right * velocity;
//    if (direction == UP)
//        Position += Up * velocity;
//    if (direction == DOWN)
//        Position -= Up * velocity;
//}

void Camera::ProcessKeyboard(glm::vec3 movement, float dt) {
    Position += Forward * movement.z * dt;
    Position += Right * movement.x * dt;
    Position += Up * movement.y * dt;
}

// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 90.0f)
        Zoom = 90.0f;
}

void Camera::SetForwardVector(glm::vec3 forward) {
    Forward = glm::normalize(forward);
    Right = glm::normalize(glm::cross(Forward, WorldUp));
    Up = glm::normalize(glm::cross(Right, Forward));
}

void Camera::UpdateLerp(float dt) {
    if (!UseLerp) return;

    glm::vec3 forward = (Forward - currentLerpForward) * LerpSpeed * dt;
    currentLerpForward = glm::normalize(currentLerpForward + forward);

    glm::vec3 right = (Right - currentLerpRight) * LerpSpeed * dt;
    currentLerpRight = glm::normalize(currentLerpRight + right);

    glm::vec3 up = (Up - currentLerpUp) * LerpSpeed * dt;
    currentLerpUp = glm::normalize(currentLerpUp + up);

    glm::vec3 worldUp = (WorldUp - currentLerpWorldUp) * LerpSpeed * dt;
    currentLerpWorldUp = glm::normalize(currentLerpWorldUp + worldUp);

    glm::vec3 move = (Position - currentLerpPosition) * LerpSpeed * dt;
    currentLerpPosition = currentLerpPosition + move;
}

void Camera::updateCameraVectors()
{
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Forward = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Forward, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up = glm::normalize(glm::cross(Right, Forward));
}

glm::vec3 Camera::getPosition() {
    if (UseLerp) return currentLerpPosition;
    return Position;
}
