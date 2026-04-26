#pragma once 

struct Player {

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	float yaw = 0.0f;
	float pitch = 0.0f;
	float sensitivity = 0.002f;
	float speed = 0.05f;
	bool debugMode = false; // Toggled by F3
	void Update();
	void HandleMouse(float dx, float dy); // New method for rotation
	void GetViewMatrix(float* view);
	void GetMVP(float* mvp);



};