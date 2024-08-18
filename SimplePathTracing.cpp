﻿#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "Shader.h"
#include "Camera.h"
#include "Shape.h"
#include <iostream>
#include <random>
#include <iomanip>

const float rad2deg = 180.0f / 3.1415926535f;
const int WIDTH = 1920;
const int HEIGHT = 1080;

const int CHUNKS_X = 2;
const int CHUNKS_Y = 2;
const bool FULLSCREEN = true;
const bool SKYBOX_ACTIVE = true;


int main()
{
	GLFWwindow* window = nullptr;
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(WIDTH, HEIGHT, "Path Tracing", nullptr, nullptr);
	if (!window)
	{
		std::cout << "Failed to create window" << std::endl;
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Creating the shaders
	Shader<ShaderType::COMPUTE> compshdr("./shaders/pathtracing.comp");
	Shader<ShaderType::RENDER> drawshdr("./shaders/pathtracing.vert", "./shaders/pathtracing.frag");

	float quad[] = {
		-1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f
	};

	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Generate the Texture storing the raytracer output
	const int TEX_W = WIDTH, TEX_H = HEIGHT;
	unsigned int tex_output;
	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEX_W, TEX_H, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	// SSBO for random engine
	// the each pixel store a random number
	std::default_random_engine rng;
	std::uniform_int_distribution<uint32_t> distr;
	std::vector<unsigned int> init_rng(WIDTH * HEIGHT);
	for (int i = 0; i < WIDTH; i++)
		for (int j = 0; j < HEIGHT; j++)
			init_rng[j * WIDTH + i] = distr(rng);

	compshdr.use();
	unsigned int SSBO_rng;
	glGenBuffers(1, &SSBO_rng);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO_rng);
	glBufferData(GL_SHADER_STORAGE_BUFFER, init_rng.size() * sizeof(unsigned int), init_rng.data(), GL_STATIC_DRAW);

	//The object store the list of Sphere and trans data to the shader
	std::vector<Shape> obj
	{
		//5wall, 1 Cube and 1 Sphere
		(Rect( // 前墙面
			glm::vec3(-5,-3,-2),
			glm::vec3(10,6,0),
			glm::vec3(0.2f, 0.3f, 0.6f),
			1.0f,
			MaterialType::LAMBERTIAN
		)),
		(Rect( // 左墙面
			glm::vec3(-5,-3,-2),
			glm::vec3(0,6,6),
			glm::vec3(0.75f, 0.35f, 0.25f),
			1.0f,
			MaterialType::LAMBERTIAN
		)),
		(Rect( // 右墙面
			glm::vec3(5,-3,-2),
			glm::vec3(0,6,6),
			glm::vec3(0.12f, 0.45f, 0.15f),
			1.0f,
			MaterialType::LAMBERTIAN,
			-1.0f
		)),
		(Rect( // 地面
			glm::vec3(-5,-3,-2),
			glm::vec3(10,0,6),
			glm::vec3(0.8f, 0.8f, 0.8f),
			1.0f,
			MaterialType::LAMBERTIAN,
			1.0f
		)),
		(Rect( // 天花板
			glm::vec3(-5, 3,-2),
			glm::vec3(10,0,6),
			glm::vec3(1.0f, 1.0f, 1.0f),
			1.0f,
			MaterialType::LAMBERTIAN,
			-1.0f
		)),
		(Rect( // 光源
			glm::vec3(-3.0f, 2.99f,-1.0f),
			glm::vec3(6,0,4),
			glm::vec3(0.8f, 0.8f, 0.8f),
			glm::vec3(2.0f),
			1.0f,
			MaterialType::LAMBERTIAN,
			-1.0f
		)),
		(Sphere( // 光源
			glm::vec3(-4.0f, 0.0f,1.0f),
			0.2f,
			glm::vec3(0.4f, 0.8f, 0.8f),
			glm::vec3(3.0f, 9.0f, 6.5f),
			1.0f,
			MaterialType::LAMBERTIAN
		)),
		(Cube( // 立方体
			glm::vec3(-2.5, -2, -1),
			glm::vec3(2.5,2.5, 2),
			20.0f,
			glm::vec3(0.8f),
			0.1f,
			MaterialType::LAMBERTIAN
		)),
		(Cube( // 金属立方体
			glm::vec3(-2.5, 1.5, 0.5),
			glm::vec3(1,1,1),
			20.0f,
			glm::vec3(0.8f),
			0.1f,
			MaterialType::METALLIC
		)),
		(Cube( // 金属立方体2
			glm::vec3(2.3, 1.5, 0.5),
			glm::vec3(1,1,1),
			45.0f,
			glm::vec3(1.0, 0.4, 0.3),
			0.1f,
			MaterialType::METALLIC
		)),
		(Sphere( // 透明球
			glm::vec3(2.5f, -1.0f, 3.0f),
			1.0f,
			glm::vec3(1.0f),
			1.5f,
			MaterialType::DIELECTRIC
		)),
	};

	compshdr.use();
	unsigned int SSBO_objects;
	glGenBuffers(1, &SSBO_objects);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, SSBO_objects);
	glBufferData(GL_SHADER_STORAGE_BUFFER, obj.size() * sizeof(Sphere), obj.data(), GL_STATIC_DRAW);

	/*unsigned int skyimg;
	glGenBuffers(1, &skyimg);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, skyimg);
	glBufferData(GL_SHADER_STORAGE_BUFFER, , , GL_STATIC_DRAW);*/


	// Camera
	Camera cam({ 0,0,16 }, { 0,0,0 }, { 0,1,0 }, 30.0f, (float)WIDTH / (float)HEIGHT);
	cam.Bind(compshdr);

	// Variables.
	int iteration = 0;
	bool wait_after_quit = false;
	double start = glfwGetTime();
	const int CHUNK_W = TEX_W / CHUNKS_X;
	const int CHUNK_H = TEX_H / CHUNKS_Y;
	const int N_CHUNKS = CHUNKS_X * CHUNKS_Y;
	compshdr.setBool("skybox_active", SKYBOX_ACTIVE);

	double prev = start;
	while (!glfwWindowShouldClose(window)) {
		//draw the picture following the left_bottom -> right_bottom -> left_top -> right-top
		int chunk_x, chunk_y;
		chunk_x = iteration % CHUNKS_X;
		chunk_y = (iteration / CHUNKS_X) % CHUNKS_Y;

		compshdr.use();
		compshdr.setInt("iteration", (iteration++ / N_CHUNKS));
		compshdr.setVector("chunk", glm::ivec2(chunk_x * CHUNK_W, chunk_y * CHUNK_H));
		glDispatchCompute(static_cast<unsigned int>(TEX_W / CHUNKS_X), static_cast<unsigned int>(TEX_H / CHUNKS_Y), 1);

		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_SPACE) || glfwGetKey(window, GLFW_KEY_ESCAPE) || glfwGetKey(window, GLFW_KEY_ENTER))
			glfwSetWindowShouldClose(window, GLFW_TRUE);

		// Rendering the texture to screen
		glClear(GL_COLOR_BUFFER_BIT);
		drawshdr.use();
		glBindVertexArray(VAO);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glfwSwapBuffers(window);

		if (iteration % N_CHUNKS == 0)
		{
			double time = glfwGetTime();
			std::cout << "Number of iterations: " << std::setw(4) << iteration / N_CHUNKS << " FPS: " << std::setw(7) << iteration / (N_CHUNKS * (time - start)) << " Delta: " << std::setw(7) << time - prev << "\t\t\t\r";
			prev = time;
		}
	}

	// Cleanup
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &SSBO_rng);
	glDeleteBuffers(1, &SSBO_objects);
	glDeleteTextures(1, &tex_output);
	glDeleteVertexArrays(1, &VAO);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}