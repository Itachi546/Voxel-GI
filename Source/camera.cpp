#include "Camera.h"

Camera::Camera()
{
	mTargetPosition = mPosition = glm::vec3(0.0f, 1.0f, -3.0f);
	mTargetRotation = mRotation = glm::vec3(0.0f, 0.0f, 0.0f);

	mFov = glm::radians(60.0f);
	mAspect = 4.0f / 3.0f;
	mNearPlane = 0.5f;
	mFarPlane = 1000.0f;

	mSpeed = 2.0f;
	mSensitivity = 0.3f;

	CalculateProjection();
	CalculateView();
}

void Camera::Update(float dt)
{
	mPosition.x += (mTargetPosition.x - mPosition.x) * 0.8f * dt;
	mPosition.y += (mTargetPosition.y - mPosition.y) * 0.8f * dt;
	mPosition.z += (mTargetPosition.z - mPosition.z) * 0.8f * dt;
	mRotation += (mTargetRotation - mRotation) * 0.8f * dt;

	CalculateView();
	mVP = mProjection * mView;
}

glm::vec4 Camera::ComputeNDCCoordinate(const glm::vec3& p)
{
	// Calculate NDC Coordinate
	glm::vec4 ndc = mProjection * mView * glm::vec4(p, 1.0f);
	ndc /= ndc.w;

	// Convert in range 0-1
	ndc = ndc * 0.5f + 0.5f;
	// Invert Y-Axis
	ndc.y = 1.0f - ndc.y;
	return ndc;
}

glm::vec4 Camera::ComputeViewSpaceCoordinate(const glm::vec3& p)
{
	return mView * glm::vec4(p, 1.0f);
}

void Camera::CalculateProjection()
{
	mProjection = glm::perspective(mFov, mAspect, mNearPlane, mFarPlane);
	mInvProjection  = glm::inverse(mProjection);

	mVP = mProjection * mView;
}

void Camera::CalculateView()
{
	glm::mat3 rotation = glm::yawPitchRoll(mRotation.y, mRotation.x, mRotation.z);

	mForward = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
	mUp = glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));

	mView = glm::lookAt(mPosition, mPosition + mForward, mUp);
	mInvView = glm::inverse(mView);

	mRight   = glm::vec3(mView[0][0], mView[1][0], mView[2][0]);
	mUp      = glm::vec3(mView[0][1], mView[1][1], mView[2][1]);
	mForward = glm::vec3(mView[0][2], mView[1][2], mView[2][2]);
}
