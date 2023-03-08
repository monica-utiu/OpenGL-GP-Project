#include "Camera.hpp"

namespace gps {

	//Camera constructor
	Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUpDirection) {
		//TODO

		this->cameraPosition = cameraPosition;
		this->cameraTarget = cameraTarget;
		this->cameraFrontDirection = glm::normalize(cameraPosition - cameraTarget); //cameraDirection
		this->cameraRightDirection = glm::normalize(glm::cross(cameraUpDirection, cameraFrontDirection));
		this->cameraUpDirection = glm::cross(cameraFrontDirection, cameraRightDirection);

	}

	//return the view matrix, using the glm::lookAt() function
	glm::mat4 Camera::getViewMatrix() {
		return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);

	}

	glm::vec3 Camera::getCameraTarget()
	{
		return cameraTarget;
	}

	//update the camera internal parameters following a camera move event
	void Camera::move(MOVE_DIRECTION direction, float speed) {
		//TODO
		switch (direction) {
		case MOVE_FORWARD:
			cameraPosition += cameraFrontDirection * speed;
			break;

		case MOVE_BACKWARD:
			cameraPosition -= cameraFrontDirection * speed;
			break;

		case MOVE_RIGHT:
			cameraPosition -= cameraRightDirection * speed;
			break;

		case MOVE_LEFT:
			cameraPosition += cameraRightDirection * speed;
			break;
		}
	}

	// update the camera internal parameters following a camera rotate event
	// yaw - camera rotation around the Y axis
	// pitch - camera rotation around the X axis
	void Camera::rotate(float pitch, float yaw) {
		//TODO
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		cameraFrontDirection = glm::normalize(front);
		cameraRightDirection = glm::cross(cameraUpDirection, cameraFrontDirection);
	}

}