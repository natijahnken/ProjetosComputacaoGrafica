/* Hello Triangle - c�digo adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gr�fico - Jogos Digitais - Unisinos
 * Vers�o inicial: 7/4/2017
 * �ltima atualiza��o em 12/05/2023
 *
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <thread>
using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "Hermite.h"
#include "Bezier.h"
#include "CatmullRom.h"

// Assinatura de fun��es
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void loadOBJ(string path);
void loadMTL(string path);
int setupSprite();
int loadTexture(string path);

vector <glm::vec3> generateControlPointsSet(string path);

// Constantes
const GLuint WIDTH = 800, HEIGHT = 600;
vector<GLfloat> positions;
vector<GLfloat> textureCoords;
vector<GLfloat> normals;
vector<GLfloat> ka;
vector<GLfloat> ks;
float ns;
string mtlFilePath = "";
string textureFilePath = "";

Camera camera;

int main()
{
	// Inicializa��o da GLFW
	glfwInit();

	//Muita aten��o aqui: alguns ambientes n�o aceitam essas configura��es
	//Voc� deve adaptar para a vers�o do OpenGL suportada por sua placa
	//Sugest�o: comente essas linhas de c�digo para desobrir a vers�o e
	//depois atualize (por exemplo: 4.5 com 4 e 5)
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Essencial para computadores da Apple
//#ifdef __APPLE__
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif
	
	// Criacao da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "M6 Trajetorias - Natalia Jahnke", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da funcao de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLAD: carrega todos os ponteiros d funcoes da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	// Obtendo as informacoes de versao
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Compilando e buildando o programa de shader
	Shader shader("sprite.vs", "sprite.fs");

	//Arquivos OBJ e MTL
	loadOBJ("SuzanneTriTextured.obj");
	loadMTL(mtlFilePath);

	// Setando VAO e VBO com textura
	GLuint textureID = loadTexture(textureFilePath);
	GLuint VAO = setupSprite();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	glUseProgram(shader.ID);

	camera.initialize(&shader, width, height);

	Mesh suzanne;
	suzanne.initialize(VAO, positions.size() / 3, &shader, textureID);

	// Propriedades de material
	shader.setVec3("ka", ka[0], ka[1], ka[2]);
	shader.setFloat("kd", 0.5);
	shader.setVec3("ks", ks[0], ks[1], ks[2]);
	shader.setFloat("q", ns);

	// Definindo a fonte de luz pontual
	shader.setVec3("lightPos", -2.0f, 100.0f, 2.0f);
	shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);

	std::vector<glm::vec3> controlPoints = generateControlPointsSet("config.txt");

	Bezier bezier;
	bezier.setControlPoints(controlPoints);
	bezier.setShader(&shader);
	bezier.generateCurve(100);
	int nbCurvePoints = bezier.getNbCurvePoints();
	int i = 0;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		camera.update();

		glm::vec3 pointOnCurve = bezier.getPointOnCurve(i);
		suzanne.updatePosition(pointOnCurve);
		suzanne.update();
		suzanne.draw();

		i = (i + 1) % nbCurvePoints;

		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	camera.move(window, key, action);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	camera.rotate(window, xpos, ypos);
}

void loadMTL(string path)
{
	string line, readValue;
	ifstream mtlFile(path);

	while (!mtlFile.eof())
	{
		getline(mtlFile, line);

		istringstream iss(line);

		if (line.find("map_Kd") == 0)
		{
			iss >> readValue >> textureFilePath;
		}
		else if (line.find("Ka") == 0)
		{
			float ka1, ka2, ka3;
			iss >> readValue >> ka1 >> ka2 >> ka3;
			ka.push_back(ka1);
			ka.push_back(ka2);
			ka.push_back(ka3);
		}
		else if (line.find("Ks") == 0)
		{
			float ks1, ks2, ks3;
			iss >> readValue >> ks1 >> ks2 >> ks3;
			ks.push_back(ks1);
			ks.push_back(ks2);
			ks.push_back(ks3);
		}
		else if (line.find("Ns") == 0)
		{
			iss >> readValue >> ns;
		}
	}
	mtlFile.close();
}

int setupSprite()
{
	GLuint VAO, VBO[3];

	glGenVertexArrays(1, &VAO);
	glGenBuffers(2, VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, textureCoords.size() * sizeof(GLfloat), textureCoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);

	return VAO;
}

void loadOBJ(string path)
{
	vector<glm::vec3> vertexIndices;
	vector<glm::vec2> textureIndices;
	vector<glm::vec3> normalIndices;

	ifstream file(path);
	if (!file.is_open())
	{
		cerr << "Failed to open file: " << path << endl;
		return;
	}

	string line;
	while (getline(file, line))
	{
		istringstream iss(line);
		string prefix;
		iss >> prefix;

		if (prefix == "mtllib")
		{
			iss >> mtlFilePath;
		}
		else if (prefix == "v")
		{
			float x, y, z;
			iss >> x >> y >> z;
			vertexIndices.push_back(glm::vec3(x, y, z));
		}
		else if (prefix == "vt")
		{
			float u, v;
			iss >> u >> v;
			textureIndices.push_back(glm::vec2(u, v));
		}
		else if (prefix == "vn")
		{
			float x, y, z;
			iss >> x >> y >> z;
			normalIndices.push_back(glm::vec3(x, y, z));
		}
		else if (prefix == "f")
		{
			string v1, v2, v3;
			iss >> v1 >> v2 >> v3;

			glm::ivec3 vIndices, tIndices, nIndices;
			istringstream(v1.substr(0, v1.find('/'))) >> vIndices.x;
			istringstream(v1.substr(v1.find('/') + 1, v1.rfind('/') - v1.find('/') - 1)) >> tIndices.x;
			istringstream(v1.substr(v1.rfind('/') + 1)) >> nIndices.x;
			istringstream(v2.substr(0, v2.find('/'))) >> vIndices.y;
			istringstream(v2.substr(v2.find('/') + 1, v2.rfind('/') - v2.find('/') - 1)) >> tIndices.y;
			istringstream(v2.substr(v2.rfind('/') + 1)) >> nIndices.y;
			istringstream(v3.substr(0, v3.find('/'))) >> vIndices.z;
			istringstream(v3.substr(v3.find('/') + 1, v3.rfind('/') - v3.find('/') - 1)) >> tIndices.z;
			istringstream(v3.substr(v3.rfind('/') + 1)) >> nIndices.z;

			for (int i = 0; i < 3; i++)
			{
				const glm::vec3& vertex = vertexIndices[vIndices[i] - 1];
				const glm::vec2& texture = textureIndices[tIndices[i] - 1];
				const glm::vec3& normal = normalIndices[nIndices[i] - 1];

				positions.push_back(vertex.x);
				positions.push_back(vertex.y);
				positions.push_back(vertex.z);
				textureCoords.push_back(texture.x);
				textureCoords.push_back(texture.y);
				normals.push_back(normal.x);
				normals.push_back(normal.y);
				normals.push_back(normal.z);
			}
		}
	}

	file.close();
}

int loadTexture(string path)
{
	GLuint texID;

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		cout << "Failed to load texture" << endl;
	}
	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texID;
}

vector<glm::vec3> generateControlPointsSet(string path)
{
	vector <glm::vec3> controlPoints;
	string line;
	ifstream configFile(path);

	while (getline(configFile, line))
	{
		istringstream iss(line);

		float x, y, z;
		iss >> x >> y >> z;
		controlPoints.push_back(glm::vec3(x, y, z));
	}

	configFile.close();

	return controlPoints;
}