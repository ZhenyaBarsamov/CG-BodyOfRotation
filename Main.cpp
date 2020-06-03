#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GL/freeglut.h>
#include <GL/gl.h>

#include <iostream>
#include <vector>
#include "mat4.h"
#include "vec3.h"
#include "lineInterpolation.h"

using namespace std;

GLFWwindow* g_window;


// ������. � OpenGL ������ ������� ���, �� ���� ��� ����������
class Model
{
public:
	GLuint vbo; // vertex buffer object (������ ����������� ������), ������� �� ������ ������ �� ������ (������� ������ �������)
	GLuint ibo; // index buffer object (������ ���������� ������), ������� �� ������ �������� �� ������
	GLuint vao; // vertex array object (��������� ��������), ������� �� ������ �������� ����, ��� ������� � ������ ��������� (�� ����� 5 float, 2 + 3). 
				// ��� ����, ���� ������� ��� �������, ����� �������� �� Memory Layout
	GLsizei indexCount; // GLsizei - ��� int �����-��. � ��� ������ 4, �� �� �������� ������� 0 � 2 ������� ������ ��� ������, ������� ������� �����

	// �����������, ���������� ��� ��������
	Model() {
		vbo = 0;
		ibo = 0;
		vao = 0;
		indexCount = 0;
	}
};

// -------------������------------------
bool g_is3D = false; // ����� ����������� (2D - ������, 3D - ���� ��������)
int g_windowWidth = 800; // ������ ����
int g_windowHeight = 600; // ������ ����

const double g_safePointsDistance = 15; // ����������� ���������� ����� �������
const int g_intermediatePointsCount = 32; // ���������� ������������� ����� ��� ������������ (32)
const int g_3dModelFragmentsCount = 64; // ���������� ��������� ���� �������� (64)

vector<vec3> g_markedPoints; // �����, ���������� �������������
vector<vec3> g_linePoints; // �����, �� ������� ���������� ������ (� �.�. �������������)

GLint g_uMVP2D; // ������� MVP ��� 2D �������
GLint g_uMVP3D; // ������� MVP ��� 3D �������
GLint g_uMV3D; // ������� MV ��� 3D �������

Model g_pointsModel = Model();
Model g_lineModel = Model();
Model g_3dModel = Model();

// ��������� ��� �������� ����� � ������� RBG (�������� �� 0 �� 1)
struct Color {
	float R;
	float G;
	float B;

	Color(float r, float g, float b) {
		R = r;
		G = g;
		B = b;
	}
};
Color g_pointsColor = Color(0.7, 0, 0); // ���� �����
Color g_lineColor = Color(0.188, 0.835, 0.784); // ���� �����
Color g_3dModelColor = Color(0.188, 0.835, 0.784); // ���� 3D ������

GLuint g_2dShaderProgram;
GLuint g_3dShaderProgram;

// -------------------------------------



// ���������� �������. ������������ ��������� ����������
GLuint createShader(const GLchar* code, GLenum type)
{
	GLuint result = glCreateShader(type);

	glShaderSource(result, 1, &code, NULL);
	glCompileShader(result);

	GLint compiled;
	glGetShaderiv(result, GL_COMPILE_STATUS, &compiled);

	// ���� ������ - ��� �� ���� ������, � ������, � ����� ������.
	// ������ �������� - � ��� ������ ���� : )
	if (!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 0)
		{
			char* infoLog = (char*)alloca(infoLen);
			glGetShaderInfoLog(result, infoLen, NULL, infoLog);
			cout << "Shader compilation error" << endl << infoLog << endl;
		}
		glDeleteShader(result);
		return 0;
	}

	return result;
}

// �������� (��������) ��������� ���������
GLuint createProgram(GLuint vsh, GLuint fsh)
{
	GLuint result = glCreateProgram();

	glAttachShader(result, vsh);
	glAttachShader(result, fsh);

	glLinkProgram(result);

	GLint linked;
	glGetProgramiv(result, GL_LINK_STATUS, &linked);

	// �� ���������� ���, ���� �� ������� �����������: � �������, ����������� ������ ���� ���������
	// � ����������� �������, � ���� ��� ������ ��������� - ����. ��� �� ������� ������� / ��������
	if (!linked)
	{
		GLint infoLen = 0;
		glGetProgramiv(result, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 0)
		{
			char* infoLog = (char*)alloca(infoLen);
			glGetProgramInfoLog(result, infoLen, NULL, infoLog);
			cout << "Shader program linking error" << endl << infoLog << endl;
		}
		glDeleteProgram(result);
		return 0;
	}

	return result;
}

// �������� ��������� ���������
bool createShaderProgram()
{
	g_2dShaderProgram = 0;
	g_3dShaderProgram = 0;

	// -------------------������� 2D-----------------

	// ����� ����������� �������
	// in - �������� ����������, �������� � ��������� ��������
	// *��������� �������� ������ ��������� �����, � �� ���� ���� - �������� �� �����
	// layout (location = 0) - ������ � ������������ ����� �����. ����� �� ������, �� ��� �����
	// � ����� ����������: "a_" - attribute, "v_" - vector
	// gl_position - ��� ��� ����������� ����������. ��������� ������� (x � y, z = 0)
	const GLchar vsh2D[] =
		"#version 330\n"
		""
		"layout(location = 0) in vec2 a_position;"
		"layout(location = 1) in vec3 a_color;"
		""
		"uniform mat4 u_mvp;"
		""
		"out vec3 v_color;"
		""
		"void main()"
		"{"
		"    v_color = a_color;"
		"    gl_Position = u_mvp * vec4(a_position, 0, 1.0);"
		"}"
		;

	// ����� ������������ �������
	// o_color - ������������� ������� ���� ���������� (�� ��������� ������� �����)
	// � o_color ���� - ��� �����-�����
	const GLchar fsh2D[] =
		"#version 330\n"
		""
		"in vec3 v_color;"
		""
		"layout(location = 0) out vec4 o_color;"
		""
		"void main()"
		"{"
		"   o_color = vec4(v_color, 1.0);"
		"}"
		;

	// ��� �� ��������
	GLuint vertexShader, fragmentShader;

	// ����������� ���� �������
	vertexShader = createShader(vsh2D, GL_VERTEX_SHADER);
	fragmentShader = createShader(fsh2D, GL_FRAGMENT_SHADER);

	// ������ (��������) ��������� ���������, ������� � ����� �������� �� ������
	g_2dShaderProgram = createProgram(vertexShader, fragmentShader);

	// ����� �������� ��������� ��������� ������� �������
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// �������� ������� �� �������-���������� ������� mvp
	g_uMVP2D = glGetUniformLocation(g_2dShaderProgram, "u_mvp");

	// -------------------������� 3D-----------------

	// ����� ����������� �������
	const GLchar vsh3D[] =
		"#version 330\n"
		""
		"layout(location = 0) in vec3 a_position;"
		"layout(location = 1) in vec3 a_color;"
		"layout(location = 2) in vec3 a_normal;"
		""
		"uniform mat4 u_mvp;"
		"uniform mat4 u_mv;"
		""
		"out vec3 v_color;"
		"out vec3 v_normal;"
		"out vec3 v_position;"
		""
		"void main()"
		"{"
		"	 mat3 n = transpose(inverse(mat3(u_mv)));"
		"	 v_position = vec3(u_mv * vec4(a_position, 1.0));"
		"    v_color = a_color;"
		"	 v_normal = normalize(n * a_normal);"
		"    gl_Position = u_mvp * vec4(a_position, 1.0);"
		"}"
		;

	// ����� ������������ �������
	const GLchar fsh3D[] =
		"#version 330\n"
		""
		"in vec3 v_color;"
		"in vec3 v_normal;"
		"in vec3 v_position;"
		""
		"layout(location = 0) out vec4 o_color;"
		""
		"void main()"
		"{"
		"	vec3 normal = normalize(v_normal);"
		"	vec3 lightPos = vec3(0, 1, 0);"
		"	vec3 lightVec = normalize(v_position - lightPos);"
		"	if (dot(-v_normal,lightVec) < dot(v_normal,lightVec)) normal = - normal;"
		"	float cosAlpha = dot(-lightVec, normal);"
		"	float diffuse = max(cosAlpha, 0.1);"
		"	vec3 reflectedLight = reflect(lightVec, normal);"
		"	vec3 eyeVec = normalize(-v_position);"
		"	float cosPhi = dot(reflectedLight, eyeVec);"
		"	cosPhi = max(cosPhi, 0);"
		"	float specular = pow(cosPhi, 5.0) * int(cosAlpha >= 0);"
		"   o_color = vec4(diffuse * v_color + vec3(specular), 1.0);"
		"}"
		;

	// ����������� ���� �������
	vertexShader = createShader(vsh3D, GL_VERTEX_SHADER);
	fragmentShader = createShader(fsh3D, GL_FRAGMENT_SHADER);

	// ������ (��������) ��������� ���������, ������� � ����� �������� �� ������
	g_3dShaderProgram = createProgram(vertexShader, fragmentShader);

	// ����� �������� ��������� ��������� ������� �������
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// �������� ������� �� �������-���������� ������� mvp
	g_uMVP3D = glGetUniformLocation(g_3dShaderProgram, "u_mvp");
	g_uMV3D = glGetUniformLocation(g_3dShaderProgram, "u_mv");


	return g_2dShaderProgram != 0 && g_3dShaderProgram != 0;
}


// �������� ������ ��� ��������� �����
bool createPointsModel()
{
	// � ������, ���� ������ ������������ - ������ ������
	if (g_pointsModel.vbo != 0)
		glDeleteBuffers(1, &g_pointsModel.vbo);
	if (g_pointsModel.ibo != 0)
		glDeleteBuffers(1, &g_pointsModel.ibo);
	if (g_pointsModel.vao != 0)
		glDeleteVertexArrays(1, &g_pointsModel.vao);

	// ������ ������

	// ������ ������. �� ������ ����� 3 �������, �� ������ ������� 5 ��������� - �� ����� ���������� ������� ������������� � ��� � ��������
	int verticesLength = g_markedPoints.size() * 3 * 5; // ����� ������� vertices
	GLfloat* vertices = new GLfloat[verticesLength];
	float triangleH = 10; // ����� ������ ������������, ������� �� �������� � �����
	for (int pointIndex = 0; pointIndex < g_markedPoints.size(); pointIndex++) {
		int curPointBeginIndex = 15 * pointIndex; // ������ ������ ������ ������� �����
		// ����� ��� ������� �� �����
		vertices[curPointBeginIndex + 0] = g_markedPoints[pointIndex].x; // ����� x ������� 1
		vertices[curPointBeginIndex + 1] = g_markedPoints[pointIndex].y + triangleH /2; // ����� y ������� 1 - ��������� � ��� ������ �� h/2
		vertices[curPointBeginIndex + 2] = g_pointsColor.R; // ����� r ������� 1
		vertices[curPointBeginIndex + 3] = g_pointsColor.G; // ����� g ������� 1
		vertices[curPointBeginIndex + 4] = g_pointsColor.B; // ����� b ������� 1
		// ---
		vertices[curPointBeginIndex + 5] = g_markedPoints[pointIndex].x - triangleH / 2; // ����� x ������� 2 - ������ � ����� �� ����� �� h/2
		vertices[curPointBeginIndex + 6] = g_markedPoints[pointIndex].y - triangleH / 2; // ����� y ������� 2 - � ���� �� ����� �� h/2
		vertices[curPointBeginIndex + 7] = g_pointsColor.R; // ����� r ������� 2
		vertices[curPointBeginIndex + 8] = g_pointsColor.G; // ����� g ������� 2
		vertices[curPointBeginIndex + 9] = g_pointsColor.B; // ����� b ������� 2
		// ---
		vertices[curPointBeginIndex + 10] = g_markedPoints[pointIndex].x + triangleH / 2; // ����� x ������� 3 - ������ � ������ �� ����� �� h/2
		vertices[curPointBeginIndex + 11] = g_markedPoints[pointIndex].y - triangleH / 2; // ����� y ������� 3 - � ���� �� ����� �� h/2
		vertices[curPointBeginIndex + 12] = g_pointsColor.R; // ����� r ������� 3
		vertices[curPointBeginIndex + 13] = g_pointsColor.G; // ����� g ������� 3
		vertices[curPointBeginIndex + 14] = g_pointsColor.B; // ����� b ������� 3
	}
	
	// ������ �������� - ������� ������ ������ ��� ���������� �������������. �� ������ ����� ����������� => �� ��� ������� �� �����
	int indicesLength = g_markedPoints.size() * 3;
	GLuint* indices = new GLuint[indicesLength];
	for (int pointIndex = 0; pointIndex < g_markedPoints.size(); pointIndex++) {
		int curPointBeginIndex = 3 * pointIndex; // ������ ������ �������� ������� �����
		indices[curPointBeginIndex] = curPointBeginIndex;
		indices[curPointBeginIndex + 1] = curPointBeginIndex + 1;
		indices[curPointBeginIndex + 2] = curPointBeginIndex + 2;
	}

	// ������� ������� �� ������

	// �������, ��� ��� ����� ���� ������ �������� (vao)
	glGenVertexArrays(1, &g_pointsModel.vao);
	// ������ ���. ����� ����� ������� ��������� ������ ����� ������ � ���� vao ������ - Memory Layout
	glBindVertexArray(g_pointsModel.vao);

	// ������ ������ ��� ���������� ����� (vbo), ���� �����
	glGenBuffers(1, &g_pointsModel.vbo);
	// ������ ��� �� ���� "������ ������"
	glBindBuffer(GL_ARRAY_BUFFER, g_pointsModel.vbo);
	// ����������� ������� �������� ������ (vertices) �� ��� �� ������. 
	// �������, 20 float'��, ������ ���� ������. 
	// GL_STATIC_DRAW - ������� � ���, ��� ����� ������ �������� � ������ ������ �� �����. ������ ���� ������.
	glBufferData(GL_ARRAY_BUFFER, verticesLength * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// ������ ������ ��������, ���� �����
	glGenBuffers(1, &g_pointsModel.ibo);
	// ������ ��� �� ��������������� ����
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_pointsModel.ibo);
	// �������� �� ������
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLength * sizeof(GLuint), indices, GL_STATIC_DRAW);
	// �������� ���������� �������� ��� ����
	g_pointsModel.indexCount = indicesLength;

	// ������ �� ������, �� ��� �� �����, ��� �� ����������������. ����� �������� �� �� ����

	// �������� ������� ����� ���� (����������)
	glEnableVertexAttribArray(0);
	// ����������� ������� ����� ����: 2 - ����������������, ��� - float, 
	// GL_FALSE - ����������, ��� ���� (��������� ������������ ��� ��������)
	// 5 * sizeof(float) - ���, � ������� ���� ����������� ���� ������� �� �������
	// ����� - �������� � ������� ��������� ����� ��������, ����������� � ���������
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)0);
	// �������� ������� ����� ���� (����)
	glEnableVertexAttribArray(1);
	// ����������� ������� ����� ����: ����������
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	// * ���� ����������� �� ���-�� ���������, �� ��� ������: ��� ����� ���� (���������� + ����)

	// � ��, ��� �� ������ ������, ������ ����������� vao, � �� �������
	// ������ ��� ��������� ������

	// ������� ���� �������
	delete[] vertices;
	delete[] indices;

	return g_pointsModel.vbo != 0 && g_pointsModel.ibo != 0 && g_pointsModel.vao != 0;
}


// �������� ������ ��� ��������� �����
bool createLineModel()
{
	// � ������, ���� ������ ������������ - ������ ������
	if (g_lineModel.vbo != 0)
		glDeleteBuffers(1, &g_lineModel.vbo);
	if (g_lineModel.ibo != 0)
		glDeleteBuffers(1, &g_lineModel.ibo);
	if (g_lineModel.vao != 0)
		glDeleteVertexArrays(1, &g_lineModel.vao);

	// ������ ������

	// ������ ������. �� ������ ����� �� ������� �� ������� �� ���� ��������
	int verticesLength = g_linePoints.size() * 5; // ����� ������� vertices
	GLfloat* vertices = new GLfloat[verticesLength];
	for (int pointIndex = 0; pointIndex < g_linePoints.size(); pointIndex++) {
		int curPointBeginIndex = 5 * pointIndex; // ������ ������ ������ ������� �����
		// ����� �������� �������
		vertices[curPointBeginIndex + 0] = g_linePoints[pointIndex].x; // ����� x �������
		vertices[curPointBeginIndex + 1] = g_linePoints[pointIndex].y; // ����� y �������
		vertices[curPointBeginIndex + 2] = g_lineColor.R; // ����� r �������
		vertices[curPointBeginIndex + 3] = g_lineColor.G; // ����� g �������
		vertices[curPointBeginIndex + 4] = g_lineColor.B; // ����� b �������
	}

	// ������ �������� - ������� ������ ������ ��� ���������� �������������. �� ������ ����� ������� => �������� ������� ��, ���� �����
	int indicesLength = g_linePoints.size();
	GLuint* indices = new GLuint[indicesLength];
	for (int pointIndex = 0; pointIndex < g_linePoints.size(); pointIndex++) {
		indices[pointIndex] = pointIndex;
	}

	// ������� ������� �� ������

	// �������, ��� ��� ����� ���� ������ �������� (vao)
	glGenVertexArrays(1, &g_lineModel.vao);
	// ������ ���. ����� ����� ������� ��������� ������ ����� ������ � ���� vao ������ - Memory Layout
	glBindVertexArray(g_lineModel.vao);

	// ������ ������ ��� ���������� ����� (vbo), ���� �����
	glGenBuffers(1, &g_lineModel.vbo);
	// ������ ��� �� ���� "������ ������"
	glBindBuffer(GL_ARRAY_BUFFER, g_lineModel.vbo);
	// ����������� ������� �������� ������ (vertices) �� ��� �� ������. 
	// �������, 20 float'��, ������ ���� ������. 
	// GL_STATIC_DRAW - ������� � ���, ��� ����� ������ �������� � ������ ������ �� �����. ������ ���� ������.
	glBufferData(GL_ARRAY_BUFFER, verticesLength * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// ������ ������ ��������, ���� �����
	glGenBuffers(1, &g_lineModel.ibo);
	// ������ ��� �� ��������������� ����
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_lineModel.ibo);
	// �������� �� ������
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLength * sizeof(GLuint), indices, GL_STATIC_DRAW);
	// �������� ���������� �������� ��� ����
	g_lineModel.indexCount = indicesLength;

	// ������ �� ������, �� ��� �� �����, ��� �� ����������������. ����� �������� �� �� ����

	// �������� ������� ����� ���� (����������)
	glEnableVertexAttribArray(0);
	// ����������� ������� ����� ����: 2 - ����������������, ��� - float, 
	// GL_FALSE - ����������, ��� ���� (��������� ������������ ��� ��������)
	// 5 * sizeof(float) - ���, � ������� ���� ����������� ���� ������� �� �������
	// ����� - �������� � ������� ��������� ����� ��������, ����������� � ���������
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)0);
	// �������� ������� ����� ���� (����)
	glEnableVertexAttribArray(1);
	// ����������� ������� ����� ����: ����������
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	// * ���� ����������� �� ���-�� ���������, �� ��� ������: ��� ����� ���� (���������� + ����)

	// � ��, ��� �� ������ ������, ������ ����������� vao, � �� �������
	// ������ ��� ��������� ������

	// ������� ���� �������
	delete[] vertices;
	delete[] indices;

	return g_lineModel.vbo != 0 && g_lineModel.ibo != 0 && g_lineModel.vao != 0;
}


// �������� ������ ��� ��������� 3D ������
bool create3dModel()
{
	// � ������, ���� ������ ������������ - ������ ������
	if (g_3dModel.vbo != 0)
		glDeleteBuffers(1, &g_3dModel.vbo);
	if (g_3dModel.ibo != 0)
		glDeleteBuffers(1, &g_3dModel.ibo);
	if (g_3dModel.vao != 0)
		glDeleteVertexArrays(1, &g_3dModel.vao);

	// ������ ������

	// ������� ������� ��� ����� ������: ����� �������� ������� ������� ������, � ������ � ��� �������������
	vec3* linePointsNormals = new vec3[g_linePoints.size()]; // ������ �������� ����� ������
	for (int i = 0; i < g_linePoints.size(); i++) {
		// ���������� ���������� � ��������� �����. ������������ ��������� �����
		int prevPointIndex = i == 0 ? 0 : i - 1;
		int nextPointIndex = i == g_linePoints.size() - 1 ? g_linePoints.size() - 1 : i + 1;
		vec3 vec = g_linePoints[nextPointIndex] - g_linePoints[prevPointIndex]; // ���������� �������
		linePointsNormals[i] = vec3(-vec.y, vec.x, 0); // ������� � ����
	}


	// �������� ��� ����� ���������� (���� �� ������, � ������� ���������)
	vector<vec3> fragmentsPoints(g_linePoints); // � ������ ���������� ������� ������ ����� �����
	vector<vec3> fragPointsNormals; // ������ �������� ����� ���������
	float rotationDeg = 360.0 / g_3dModelFragmentsCount; // ���� ����� ����� ���������� � �������� � ���
	int fragmentPointsCount = g_linePoints.size(); // ���������� ����� �� ��������� (���-�� ����� � �����)
	// ��������� ������� ������� ��������� (�������� �����)
	for (int i = 0; i < fragmentPointsCount; i++)
		fragPointsNormals.push_back(linePointsNormals[i]);
	// ������ �������� - ���� �����. �������� �� ������� - �.�. � ������ ��������
	for (int rotationIndex = 1; rotationIndex < g_3dModelFragmentsCount; rotationIndex++) {
		mat4 rotationMat = mat4::rotate(mat4(), rotationDeg * rotationIndex, vec3(-1, 0, 0)); // ������� �������� �� ��� X (�.�. �����)
		// ������ ����� ����������� ��������� �������� ����� �� ������� ��������, � ���������� � ������ ����� ����������
		for (int i = 0; i < fragmentPointsCount; i++) {
			vec3 p = rotationMat * g_linePoints[i];
			vec3 n = rotationMat * linePointsNormals[i];
			fragmentsPoints.push_back(p);
			fragPointsNormals.push_back(n);
		}
	}
	// � ��������� ���������� ���������� ������ - ���� ���� ������� ��������� ��������� � ������
	for (int i = 0; i < fragmentPointsCount; i++) {
		fragmentsPoints.push_back(g_linePoints[i]);
		fragPointsNormals.push_back(linePointsNormals[i]);
	}

	// ������ ������. ������ = ���������� �����. �� ������ ������� 9 ���������
	int fragmentsPointsCount = fragmentsPoints.size();
	int verticesLength = fragmentsPoints.size() * 9; // ����� ������� vertices
	GLfloat* vertices = new GLfloat[verticesLength];
	for (int pointIndex = 0; pointIndex < fragmentsPointsCount; pointIndex++) {
		int curVertexBeginIndex = 9 * pointIndex; // ������ ��������� ������� �������
		vertices[curVertexBeginIndex + 0] = fragmentsPoints[pointIndex].x; // ����� x �������
		vertices[curVertexBeginIndex + 1] = fragmentsPoints[pointIndex].y; // ����� y �������
		vertices[curVertexBeginIndex + 2] = fragmentsPoints[pointIndex].z; // ����� z �������
		vertices[curVertexBeginIndex + 3] = g_3dModelColor.R; // ����� r �������
		vertices[curVertexBeginIndex + 4] = g_3dModelColor.G; // ����� g �������
		vertices[curVertexBeginIndex + 5] = g_3dModelColor.B; // ����� b �������
		vertices[curVertexBeginIndex + 6] = fragPointsNormals[pointIndex].x; // ����� x ������� �������
		vertices[curVertexBeginIndex + 7] = fragPointsNormals[pointIndex].y; // ����� y ������� �������
		vertices[curVertexBeginIndex + 8] = fragPointsNormals[pointIndex].z; // ����� z ������� �������
	}

	// ������ �������� - ������� ������ ������ ��� ���������� �������������. ���-�� ������������� �/� ��������� ����������� * 3 ������� * ���-�� ����� ����������
	int indicesLength = (g_linePoints.size() * 2 - 2) * 3 * g_3dModelFragmentsCount;
	GLuint* indices = new GLuint[indicesLength];
	// ��� ������� ���������: ������� �������� ��������� �� ���������
	int indexIndex = 0;
	for (int fragmentIndex = 0; fragmentIndex < g_3dModelFragmentsCount; fragmentIndex++) {
		int curFragmentBeginIndex = fragmentPointsCount * fragmentIndex; // ������ ������ �������� ��� �������� ���������
		// ������ ������������� �� ���� �������������
		for (int fragPointIndex = 0; fragPointIndex < fragmentPointsCount - 1; fragPointIndex++) {
			// ������ �����������
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + 1;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + fragmentPointsCount;
			indexIndex++;
			// ������ �����������
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + fragmentPointsCount;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + 1;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + fragmentPointsCount + 1;
			indexIndex++;
		}
	}

	// ������� ������� �� ������

	// �������, ��� ��� ����� ���� ������ �������� (vao)
	glGenVertexArrays(1, &g_3dModel.vao);
	// ������ ���. ����� ����� ������� ��������� ������ ����� ������ � ���� vao ������ - Memory Layout
	glBindVertexArray(g_3dModel.vao);

	// ������ ������ ��� ���������� ����� (vbo), ���� �����
	glGenBuffers(1, &g_3dModel.vbo);
	// ������ ��� �� ���� "������ ������"
	glBindBuffer(GL_ARRAY_BUFFER, g_3dModel.vbo);
	// ����������� ������� �������� ������ (vertices) �� ��� �� ������. 
	// �������, 20 float'��, ������ ���� ������. 
	// GL_STATIC_DRAW - ������� � ���, ��� ����� ������ �������� � ������ ������ �� �����. ������ ���� ������.
	glBufferData(GL_ARRAY_BUFFER, verticesLength * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// ������ ������ ��������, ���� �����
	glGenBuffers(1, &g_3dModel.ibo);
	// ������ ��� �� ��������������� ����
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_3dModel.ibo);
	// �������� �� ������
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLength * sizeof(GLuint), indices, GL_STATIC_DRAW);
	// �������� ���������� �������� ��� ����
	g_3dModel.indexCount = indicesLength;

	// ������ �� ������, �� ��� �� �����, ��� �� ����������������. ����� �������� �� �� ����

	// �������� ������� ����� ���� (����������)
	glEnableVertexAttribArray(0);
	// ����������� ������� ����� ����: 3 - ���������������, ��� - float, 
	// GL_FALSE - ����������, ��� ���� (��������� ������������ ��� ��������)
	// 5 * sizeof(float) - ���, � ������� ���� ����������� ���� ������� �� �������
	// ����� - �������� � ������� ��������� ����� ��������, ����������� � ���������
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (const GLvoid*)0);
	// �������� ������� ����� ���� (����)
	glEnableVertexAttribArray(1);
	// ����������� ������� ����� ����: ����������
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
	// �������� ������� ����� ��� (�������)
	glEnableVertexAttribArray(2);
	// ����������� ������� ����� ����: ����������
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (const GLvoid*)(6 * sizeof(GLfloat)));
	// * ���� ����������� �� ���-�� ���������, �� ��� ������: ��� ����� ���� (���������� + ����)

	// � ��, ��� �� ������ ������, ������ ����������� vao, � �� �������
	// ������ ��� ��������� ������

	// ������� ���� �������
	delete[] vertices;
	delete[] indices;
	delete[] linePointsNormals;

	return g_3dModel.vbo != 0 && g_3dModel.ibo != 0 && g_3dModel.vao != 0;
}


// ������������� ����������� ��������
bool init()
{
	// ������������� ����� ��������� ������ ������ �����
	// �.�. ����� ���� ������� (���� ������� ������ �����)
	// �������� ������� ���������, � ���� �� ����� ����������������,
	// ����������� ��������� = 0..1 (� ������ ��� ���� ��� � ������ ������ ����������)
	// �������� ������: RGBA
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// �������� ���� ������ �������
	glEnable(GL_DEPTH_TEST);

	// ����� ������ ������� � ������ (���)
	return createShaderProgram();
}


// ��������� ������� ��������������� ����
void reshape(GLFWwindow* window, int width, int height)
{
	// �������, ��� ����� � ��� ���� ���������: 
	// (0,0) - ���������� ��� ������ ������� ����
	glViewport(0, 0, width, height);
	// ��������� ����� ������� ������
	g_windowWidth = width;
	g_windowHeight = height;
}

// ��������� ������� ������� ����
void mouseButtons_callback(GLFWwindow* window, int button, int action, int mods) {
	// ���� ���� ������ ����� ������ - ��������
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// �������� ������� �������
		double xpos, ypos;
		glfwGetCursorPos(g_window, &xpos, &ypos);
		// ���������� ���� ���������� � ����� ������� ����, � ��� �� � ����� ������ (�������� � ����������� �����)
		ypos = g_windowHeight - ypos;
		// ������ ����� � ���� �������
		vec3 posPoint = vec3(xpos, ypos, 0);
		// �������� ���������� � �������
		cout << "MouseClick: x = " << posPoint.x << " y = " << posPoint.y << "\n";
		// ���������, ������� �� ������������ ����� �� ��������� �� ������ ����������
		for (int i = 0; i < g_markedPoints.size(); i++) {
			vec3 vec = posPoint - g_markedPoints[i]; // ������ ����� ����� �������
			// ���� ����� ������� �� ������������� ����������� - �������
			if (vec3::length(vec) < g_safePointsDistance)
				return;
		}
		// ���� �������� ������ - ��������� ����� � ������ �����
		g_markedPoints.push_back(posPoint);
		//// � ���� � ������ ����� �����
		//g_linePoints.push_back(posPoint);
		// � �������� ������������
		if (g_markedPoints.size() > 1) {
			//g_linePoints = GetLinePoints(g_markedPoints, g_intermediatePointsCount);
			g_linePoints = g_markedPoints;
		}
		// � ���������� ������ �����. ������ � ��� ��� �� ������� ���� ���� ����� ����
		bool f = createPointsModel();
		// � ���� ����� ������ �����, �� ������ � ������ �����
		f = createLineModel();
	}
	// ���� ���� ������ ������ ������ ���� - ��������� � 3D �����
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		// ���� ����� 3D ��� ������� - ���������� ��
		if (g_is3D) {
			g_markedPoints.clear();
			g_linePoints.clear();
			g_is3D = false;
		}
		
		// ���� ������ ���� ����� - �������
		if (g_markedPoints.size() < 2)
			return;
		g_is3D = true;
		create3dModel();

	}
}



// ��������� �����
void draw()
{
	// ������� ����� �����
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ���� 3D �����
	if (g_is3D) {
		// �������� ������ ��������� ���������
		glUseProgram(g_3dShaderProgram);
		// �������� ������ ����� (���������, ��� ���������������� �������)
		glBindVertexArray(g_3dModel.vao);

		mat4 m = mat4(); // ������� model
		// ������������� �� �����
		m = mat4::translate(m, vec3(0, 0, -250));
		m = mat4::scale(m, vec3(0.1, 0.1, 0.1));
		// ������������ �� ����������� � ����������� �� �������
		m = mat4::translate(m, vec3(50, 50, 50));
		m = mat4::rotate(m, glfwGetTime()*23, vec3(0.0f, -1.0f, 0.0f));
		m = mat4::translate(m, vec3(-50, -50, -50));
		mat4 v = mat4::getViewMatrix(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0)); // ������� view
		// ����������� ����� � NDC � ������� ������� ������������ �������� � ��������� �����������
		mat4 p = mat4::getPerspectiveProjectionMatrix(0.01, 1000, (float)g_windowWidth, (float)g_windowHeight, 45.0f); // ������� projection
		mat4 mvp = p * v * m; // ������� mvp
		mat4 mv = v * m; // ������� mvp
		// ���������� ������� mvp �� ������
		glUniformMatrix4fv(g_uMVP3D, 1, GL_FALSE, mvp.getMatrixPtr());
		// ���������� ������� mv �� ������
		glUniformMatrix4fv(g_uMV3D, 1, GL_FALSE, mv.getMatrixPtr());

		// ���������: �������������� ������� ��� ��� ��������� �������������
		// GL_UNSIGNED_INT = 2^32 - ������ ���������� �������� (������� ����� ���������� �� ���)
		// NULL - ������ ������� �� ������ ������. ����� �� ��������� �� ������ ����, �� ������������
		glDrawElements(GL_TRIANGLES, g_3dModel.indexCount, GL_UNSIGNED_INT, NULL);
		return;
	}

	// �������� ������ ��������� ���������
	glUseProgram(g_2dShaderProgram);
	// ���� ���������� ���� �� ���� ����� - ������������ ������ �����
	if (g_markedPoints.size() > 0) {
		// �������� ������ ����� (���������, ��� ���������������� �������)
		glBindVertexArray(g_pointsModel.vao);

		// ����������� ����� � NDC � ������� ������� ������������ �������� � ��������� �����������. 
		mat4 m = mat4(); // ������� model
		mat4 v = mat4::getViewMatrix(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0)); // ������� view
		mat4 p = mat4::getParallelProjectionMatrix(0, g_windowWidth, 0, g_windowHeight, -1, 1); // ������� projection
		mat4 mvp = p * v * m; // ������� mvp
		// ���������� ������� mvp �� ������
		glUniformMatrix4fv(g_uMVP2D, 1, GL_FALSE, mvp.getMatrixPtr());

		// ���������: �������������� ������� ��� ��� ��������� �������������
		// GL_UNSIGNED_INT = 2^32 - ������ ���������� �������� (������� ����� ���������� �� ���)
		// NULL - ������ ������� �� ������ ������. ����� �� ��������� �� ������ ����, �� ������������
		glDrawElements(GL_TRIANGLES, g_pointsModel.indexCount, GL_UNSIGNED_INT, NULL);
	}
	// ���� ���������� ��� � ������ ����� - ������������ ������ �����
	if (g_markedPoints.size() > 1) {
		// �������� ������ ����� (���������, ��� ���������������� �������)
		glBindVertexArray(g_lineModel.vao);

		// ����������� ����� � NDC � ������� ������� ������������ �������� � ��������� �����������. 
		mat4 m = mat4(); // ������� model
		mat4 v = mat4::getViewMatrix(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0)); // ������� view
		mat4 p = mat4::getParallelProjectionMatrix(0, g_windowWidth, 0, g_windowHeight, -1, 1); // ������� projection
		mat4 mvp = p * v * m; // ������� mvp
		// ���������� ������� mvp �� ������
		glUniformMatrix4fv(g_uMVP2D, 1, GL_FALSE, mvp.getMatrixPtr());

		// ���������: �������������� ������� ��� ��� ��������� ������������������ �����
		// GL_UNSIGNED_INT = 2^32 - ������ ���������� �������� (������� ����� ���������� �� ���)
		// NULL - ������ ������� �� ������ ������. ����� �� ��������� �� ������ ����, �� ������������
		glDrawElements(GL_LINE_STRIP, g_lineModel.indexCount, GL_UNSIGNED_INT, NULL);
	}
}


// ������ ������ ����� �������� (��������� ��������� � ������� vbo, ibo, vao)
void cleanup()
{
	// �������
	if (g_2dShaderProgram != 0)
		glDeleteProgram(g_2dShaderProgram);
	if (g_3dShaderProgram != 0)
		glDeleteProgram(g_3dShaderProgram);
	// ������ �����
	if (g_pointsModel.vbo != 0)
		glDeleteBuffers(1, &g_pointsModel.vbo);
	if (g_pointsModel.ibo != 0)
		glDeleteBuffers(1, &g_pointsModel.ibo);
	if (g_pointsModel.vao != 0)
		glDeleteVertexArrays(1, &g_pointsModel.vao);
	// ������ �����
	if (g_lineModel.vbo != 0)
		glDeleteBuffers(1, &g_lineModel.vbo);
	if (g_lineModel.ibo != 0)
		glDeleteBuffers(1, &g_lineModel.ibo);
	if (g_lineModel.vao != 0)
		glDeleteVertexArrays(1, &g_lineModel.vao);
	// ������ 3D �������
	if (g_3dModel.vbo != 0)
		glDeleteBuffers(1, &g_3dModel.vbo);
	if (g_3dModel.ibo != 0)
		glDeleteBuffers(1, &g_3dModel.ibo);
	if (g_3dModel.vao != 0)
		glDeleteVertexArrays(1, &g_3dModel.vao);
}


// ������������� OpenGL
bool initOpenGL()
{
	// �������������� ���������� GLFW ����� � ��������������
	// GLFW - ���������� ��� ������ � ������ � �.�. �� ����, �� ��� �������
	if (!glfwInit())
	{
		cout << "Failed to initialize GLFW" << endl;
		return false;
	}

	// ����������� OpenGL ������ 3.3 (��� ���������� �������?)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// ������ ���� (���� ���, ������, OpenGL ���, ��� ��� ���, � ��� �����)
	g_window = glfwCreateWindow(g_windowWidth, g_windowHeight, "OpenGL Test", NULL, NULL);
	if (g_window == NULL)
	{
		cout << "Failed to open GLFW window" << endl;
		glfwTerminate();
		return false;
	}

	// �������������� ����������� �������� OpenGL � ����������� ��� - ������ ������� (�� ��������������� ���)
	glfwMakeContextCurrent(g_window);

	// �������������� ���������� ���������� ���������� GLEW ��� ��������� OpenGL Core Profile
	// GLEW - "������� ����������", ����������, ����������� ������ �� OpenGL. �������
	glewExperimental = true;

	// �������������� ������� GLEW
	if (glewInit() != GLEW_OK)
	{
		cout << "Failed to initialize GLEW" << endl;
		return false;
	}

	// ���������� � ���, ��� ������ �������� ������� ������� Escape.
	// � ��� ���� ��� ������ ������ ����� ���� exception'��?
	glfwSetInputMode(g_window, GLFW_STICKY_KEYS, GL_TRUE);


	// �����: ��������� � ������� ������� (reshape) ���������-����������� �������
	// � reshape �������, ��� ����� � ��� ���� ���������
	glfwSetFramebufferSizeCallback(g_window, reshape);

	// ��������� ���������� ������� �� ������� ����
	glfwSetMouseButtonCallback(g_window, mouseButtons_callback);

	return true;
}


// ������� �������� OpenGL (GLFW)
void tearDownOpenGL()
{
	// ���������� GLWF
	glfwTerminate();
}

int main()
{
	// ������������� OpenGL
	if (!initOpenGL())
		return -1;

	// ������������� ����������� �������� - ���� �������� ����������
	bool isOk = init();

	// ���� ������������� �������� ������� - �������� ������
	if (isOk)
	{
		// ������� ���� ��������, ���� ���� �� ���� ������� � �� ���� ������ ������ Escape
		while (glfwGetKey(g_window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(g_window) == 0)
		{
			// ������������ �����
			draw();

			// ����� ������� (���� ������ �����)
			glfwSwapBuffers(g_window);
			// �������� �������?
			glfwPollEvents();
		}
	}

	// ������� ����������� �������
	cleanup();

	// ������������� OpenGL
	tearDownOpenGL();

	return isOk ? 0 : -1;
}
