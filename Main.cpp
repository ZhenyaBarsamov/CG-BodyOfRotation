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


// Модель. В OpenGL такого понятия нет, мы сами его определили
class Model
{
public:
	GLuint vbo; // vertex buffer object (объект вертексного буфера), хэндлер на массив вершин на видяхе (никаких прямых адресов)
	GLuint ibo; // index buffer object (объект индексного буфера), хэндлер на массив индексов на видяхе
	GLuint vao; // vertex array object (неудачное название), хэндлер на массив настроек того, как вершины в памяти разложены (на точку 5 float, 2 + 3). 
				// Для того, чтоб донести это системе, нужно передать ей Memory Layout
	GLsizei indexCount; // GLsizei - тож int какой-то. У нас вершин 4, но по конвееру вершины 0 и 2 пройдут дважды как разные, поэтому индексы нужны

	// Конструктор, зануляющий все значения
	Model() {
		vbo = 0;
		ibo = 0;
		vao = 0;
		indexCount = 0;
	}
};

// -------------КРИВЫЕ------------------
bool g_is3D = false; // режим отображения (2D - кривая, 3D - тело вращения)
int g_windowWidth = 800; // ширина окна
int g_windowHeight = 600; // высота окна

const double g_safePointsDistance = 15; // минимальное расстояние между точками
const int g_intermediatePointsCount = 32; // количество промежуточных точек для интерполяции (32)
const int g_3dModelFragmentsCount = 64; // количество разбиений тела вращения (64)

vector<vec3> g_markedPoints; // точки, отмеченные пользователем
vector<vec3> g_linePoints; // точки, по которым построится кривая (в т.ч. промежуточные)

GLint g_uMVP2D; // матрица MVP для 2D шейдера
GLint g_uMVP3D; // матрица MVP для 3D шейдера
GLint g_uMV3D; // матрица MV для 3D шейдера

Model g_pointsModel = Model();
Model g_lineModel = Model();
Model g_3dModel = Model();

// Структура для хранения цвета в формате RBG (значения от 0 до 1)
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
Color g_pointsColor = Color(0.7, 0, 0); // цвет точек
Color g_lineColor = Color(0.188, 0.835, 0.784); // цвет линии
Color g_3dModelColor = Color(0.188, 0.835, 0.784); // цвет 3D модели

GLuint g_2dShaderProgram;
GLuint g_3dShaderProgram;

// -------------------------------------



// Компиляция шейдера. Возвращается результат компиляции
GLuint createShader(const GLchar* code, GLenum type)
{
	GLuint result = glCreateShader(type);

	glShaderSource(result, 1, &code, NULL);
	glCompileShader(result);

	GLint compiled;
	glGetShaderiv(result, GL_COMPILE_STATUS, &compiled);

	// Если ошибка - нам об этом скажут, и укажут, в какой строке.
	// Только проблема - у нас строка одна : )
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

// Создание (линковка) шейдерной программы
GLuint createProgram(GLuint vsh, GLuint fsh)
{
	GLuint result = glCreateProgram();

	glAttachShader(result, vsh);
	glAttachShader(result, fsh);

	glLinkProgram(result);

	GLint linked;
	glGetProgramiv(result, GL_LINK_STATUS, &linked);

	// Не слинкуются они, если не сошлись характерами: к примеру, обязательно должен быть вершинный
	// и фрагментный шейдеры, а если оба пришли вершинные - фейл. Или не сошлись входами / выходами
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

// Создание шейдерной программы
bool createShaderProgram()
{
	g_2dShaderProgram = 0;
	g_3dShaderProgram = 0;

	// -------------------ШЕЙДЕРЫ 2D-----------------

	// Текст вершиннного шейдера
	// in - входящие переменные, кладутся в векторные регистры
	// *векторные регистры хранят несколько чисел, и за один такт - операция со всеми
	// layout (location = 0) - ставим в соответствие имени номер. Можно не делать, но так лучше
	// В имени переменной: "a_" - attribute, "v_" - vector
	// gl_position - тип уже определённой переменной. Формируем позицию (x и y, z = 0)
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

	// Текст фрагментного шейдера
	// o_color - соответствует нулевой цели рендеринга (мб несколько буферов цвета)
	// в o_color ноль - это альфа-канал
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

	// Для их хранения
	GLuint vertexShader, fragmentShader;

	// Компилируем наши шейдеры
	vertexShader = createShader(vsh2D, GL_VERTEX_SHADER);
	fragmentShader = createShader(fsh2D, GL_FRAGMENT_SHADER);

	// Создаём (слинкуем) шейдерную программу, которая и будет записана на видяху
	g_2dShaderProgram = createProgram(vertexShader, fragmentShader);

	// После создания шейдерной программы шейдеры удаляем
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Получаем хэндлер на юниформ-переменную матрицы mvp
	g_uMVP2D = glGetUniformLocation(g_2dShaderProgram, "u_mvp");

	// -------------------ШЕЙДЕРЫ 3D-----------------

	// Текст вершиннного шейдера
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

	// Текст фрагментного шейдера
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

	// Компилируем наши шейдеры
	vertexShader = createShader(vsh3D, GL_VERTEX_SHADER);
	fragmentShader = createShader(fsh3D, GL_FRAGMENT_SHADER);

	// Создаём (слинкуем) шейдерную программу, которая и будет записана на видяху
	g_3dShaderProgram = createProgram(vertexShader, fragmentShader);

	// После создания шейдерной программы шейдеры удаляем
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Получаем хэндлер на юниформ-переменную матрицы mvp
	g_uMVP3D = glGetUniformLocation(g_3dShaderProgram, "u_mvp");
	g_uMV3D = glGetUniformLocation(g_3dShaderProgram, "u_mv");


	return g_2dShaderProgram != 0 && g_3dShaderProgram != 0;
}


// Создание модели для рисования точек
bool createPointsModel()
{
	// В случае, если модель пересоздаётся - чистим буферы
	if (g_pointsModel.vbo != 0)
		glDeleteBuffers(1, &g_pointsModel.vbo);
	if (g_pointsModel.ibo != 0)
		glDeleteBuffers(1, &g_pointsModel.ibo);
	if (g_pointsModel.vao != 0)
		glDeleteVertexArrays(1, &g_pointsModel.vao);

	// Создаём модель

	// Массив вершин. На каждую точку 3 вершины, на каждую вершину 5 атрибутов - мы точку обозначаем красным треугольником с ней в середине
	int verticesLength = g_markedPoints.size() * 3 * 5; // длина массива vertices
	GLfloat* vertices = new GLfloat[verticesLength];
	float triangleH = 10; // длина высоты треугольника, который мы поставим в точке
	for (int pointIndex = 0; pointIndex < g_markedPoints.size(); pointIndex++) {
		int curPointBeginIndex = 15 * pointIndex; // индекс начала вершин текущей точки
		// Задаём три вершины на точку
		vertices[curPointBeginIndex + 0] = g_markedPoints[pointIndex].x; // задаём x вершины 1
		vertices[curPointBeginIndex + 1] = g_markedPoints[pointIndex].y + triangleH /2; // задаём y вершины 1 - поднимаем её над точкой на h/2
		vertices[curPointBeginIndex + 2] = g_pointsColor.R; // задаём r вершины 1
		vertices[curPointBeginIndex + 3] = g_pointsColor.G; // задаём g вершины 1
		vertices[curPointBeginIndex + 4] = g_pointsColor.B; // задаём b вершины 1
		// ---
		vertices[curPointBeginIndex + 5] = g_markedPoints[pointIndex].x - triangleH / 2; // задаём x вершины 2 - уводим её влево от точки на h/2
		vertices[curPointBeginIndex + 6] = g_markedPoints[pointIndex].y - triangleH / 2; // задаём y вершины 2 - и вниз от точки на h/2
		vertices[curPointBeginIndex + 7] = g_pointsColor.R; // задаём r вершины 2
		vertices[curPointBeginIndex + 8] = g_pointsColor.G; // задаём g вершины 2
		vertices[curPointBeginIndex + 9] = g_pointsColor.B; // задаём b вершины 2
		// ---
		vertices[curPointBeginIndex + 10] = g_markedPoints[pointIndex].x + triangleH / 2; // задаём x вершины 3 - уводим её вправо от точки на h/2
		vertices[curPointBeginIndex + 11] = g_markedPoints[pointIndex].y - triangleH / 2; // задаём y вершины 3 - и вниз от точки на h/2
		vertices[curPointBeginIndex + 12] = g_pointsColor.R; // задаём r вершины 3
		vertices[curPointBeginIndex + 13] = g_pointsColor.G; // задаём g вершины 3
		vertices[curPointBeginIndex + 14] = g_pointsColor.B; // задаём b вершины 3
	}
	
	// Массив индексов - порядок обхода вершин для построения треугольников. На каждую точку треугольник => по три индекса на точку
	int indicesLength = g_markedPoints.size() * 3;
	GLuint* indices = new GLuint[indicesLength];
	for (int pointIndex = 0; pointIndex < g_markedPoints.size(); pointIndex++) {
		int curPointBeginIndex = 3 * pointIndex; // индекс начала индексов текущей точки
		indices[curPointBeginIndex] = curPointBeginIndex;
		indices[curPointBeginIndex + 1] = curPointBeginIndex + 1;
		indices[curPointBeginIndex + 2] = curPointBeginIndex + 2;
	}

	// Толкаем массивы на видяху

	// Говорим, что нам нужен один массив настроек (vao)
	glGenVertexArrays(1, &g_pointsModel.vao);
	// Биндим его. После этого система автоматом неявно будет писать в этот vao нужное - Memory Layout
	glBindVertexArray(g_pointsModel.vao);

	// Создаём буффер под вертексный буфер (vbo), одну штуку
	glGenBuffers(1, &g_pointsModel.vbo);
	// Биндим его на роль "массив вершин"
	glBindBuffer(GL_ARRAY_BUFFER, g_pointsModel.vbo);
	// Блокирующим образом копируем память (vertices) из ОЗУ на видяху. 
	// Говорим, 20 float'ов, откуда берём данные. 
	// GL_STATIC_DRAW - говорим о том, что будем просто рисовать и ничего менять не будем. Просто хинт видяхе.
	glBufferData(GL_ARRAY_BUFFER, verticesLength * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// Создаём массив индексов, одну штуку
	glGenBuffers(1, &g_pointsModel.ibo);
	// Биндим его на соответствующую роль
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_pointsModel.ibo);
	// Копируем на видяху
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLength * sizeof(GLuint), indices, GL_STATIC_DRAW);
	// Сохраним количество индексов для себя
	g_pointsModel.indexCount = indicesLength;

	// Данные на видяхе, но она не знает, как их интерпретировать. Нужно сообщить ей об этом

	// Включаем атрибут номер ноль (координаты)
	glEnableVertexAttribArray(0);
	// Настраиваем атрибут номер ноль: 2 - двухкомпонентный, тип - float, 
	// GL_FALSE - используем, как есть (отключаем нормирование для векторов)
	// 5 * sizeof(float) - шаг, с которым надо выхватывать этот атрибут из массива
	// Затем - смещение к первому вхождению этого атрибута, кастованное к указателю
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)0);
	// Включаем атрибут номер один (цвет)
	glEnableVertexAttribArray(1);
	// Настраиваем атрибут номер один: аналогично
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	// * Есть ограничения на кол-во атрибутов, но нам хватит: два точно есть (координаты + цвет)

	// И всё, что мы сейчас делали, слушал забинденный vao, и всё записал
	// Данные для конвейера готовы

	// Удаляем наши массивы
	delete[] vertices;
	delete[] indices;

	return g_pointsModel.vbo != 0 && g_pointsModel.ibo != 0 && g_pointsModel.vao != 0;
}


// Создание модели для рисования точек
bool createLineModel()
{
	// В случае, если модель пересоздаётся - чистим буферы
	if (g_lineModel.vbo != 0)
		glDeleteBuffers(1, &g_lineModel.vbo);
	if (g_lineModel.ibo != 0)
		glDeleteBuffers(1, &g_lineModel.ibo);
	if (g_lineModel.vao != 0)
		glDeleteVertexArrays(1, &g_lineModel.vao);

	// Создаём модель

	// Массив вершин. На каждую точку по вершине на вершину по пять атрибута
	int verticesLength = g_linePoints.size() * 5; // длина массива vertices
	GLfloat* vertices = new GLfloat[verticesLength];
	for (int pointIndex = 0; pointIndex < g_linePoints.size(); pointIndex++) {
		int curPointBeginIndex = 5 * pointIndex; // индекс начала вершин текущей точки
		// Задаём атрибуты вершины
		vertices[curPointBeginIndex + 0] = g_linePoints[pointIndex].x; // задаём x вершины
		vertices[curPointBeginIndex + 1] = g_linePoints[pointIndex].y; // задаём y вершины
		vertices[curPointBeginIndex + 2] = g_lineColor.R; // задаём r вершины
		vertices[curPointBeginIndex + 3] = g_lineColor.G; // задаём g вершины
		vertices[curPointBeginIndex + 4] = g_lineColor.B; // задаём b вершины
	}

	// Массив индексов - порядок обхода вершин для построения треугольников. На каждую точку вершина => индексов столько же, скок точек
	int indicesLength = g_linePoints.size();
	GLuint* indices = new GLuint[indicesLength];
	for (int pointIndex = 0; pointIndex < g_linePoints.size(); pointIndex++) {
		indices[pointIndex] = pointIndex;
	}

	// Толкаем массивы на видяху

	// Говорим, что нам нужен один массив настроек (vao)
	glGenVertexArrays(1, &g_lineModel.vao);
	// Биндим его. После этого система автоматом неявно будет писать в этот vao нужное - Memory Layout
	glBindVertexArray(g_lineModel.vao);

	// Создаём буффер под вертексный буфер (vbo), одну штуку
	glGenBuffers(1, &g_lineModel.vbo);
	// Биндим его на роль "массив вершин"
	glBindBuffer(GL_ARRAY_BUFFER, g_lineModel.vbo);
	// Блокирующим образом копируем память (vertices) из ОЗУ на видяху. 
	// Говорим, 20 float'ов, откуда берём данные. 
	// GL_STATIC_DRAW - говорим о том, что будем просто рисовать и ничего менять не будем. Просто хинт видяхе.
	glBufferData(GL_ARRAY_BUFFER, verticesLength * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// Создаём массив индексов, одну штуку
	glGenBuffers(1, &g_lineModel.ibo);
	// Биндим его на соответствующую роль
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_lineModel.ibo);
	// Копируем на видяху
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLength * sizeof(GLuint), indices, GL_STATIC_DRAW);
	// Сохраним количество индексов для себя
	g_lineModel.indexCount = indicesLength;

	// Данные на видяхе, но она не знает, как их интерпретировать. Нужно сообщить ей об этом

	// Включаем атрибут номер ноль (координаты)
	glEnableVertexAttribArray(0);
	// Настраиваем атрибут номер ноль: 2 - двухкомпонентный, тип - float, 
	// GL_FALSE - используем, как есть (отключаем нормирование для векторов)
	// 5 * sizeof(float) - шаг, с которым надо выхватывать этот атрибут из массива
	// Затем - смещение к первому вхождению этого атрибута, кастованное к указателю
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)0);
	// Включаем атрибут номер один (цвет)
	glEnableVertexAttribArray(1);
	// Настраиваем атрибут номер один: аналогично
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	// * Есть ограничения на кол-во атрибутов, но нам хватит: два точно есть (координаты + цвет)

	// И всё, что мы сейчас делали, слушал забинденный vao, и всё записал
	// Данные для конвейера готовы

	// Удаляем наши массивы
	delete[] vertices;
	delete[] indices;

	return g_lineModel.vbo != 0 && g_lineModel.ibo != 0 && g_lineModel.vao != 0;
}


// Создание модели для рисования 3D модели
bool create3dModel()
{
	// В случае, если модель пересоздаётся - чистим буферы
	if (g_3dModel.vbo != 0)
		glDeleteBuffers(1, &g_3dModel.vbo);
	if (g_3dModel.ibo != 0)
		glDeleteBuffers(1, &g_3dModel.ibo);
	if (g_3dModel.vao != 0)
		glDeleteVertexArrays(1, &g_3dModel.vao);

	// Создаём модель

	// Находим нормали для точек кривой: между соседями точками находим вектор, и строим к ним перпендикуляр
	vec3* linePointsNormals = new vec3[g_linePoints.size()]; // вектор нормалей точек кривой
	for (int i = 0; i < g_linePoints.size(); i++) {
		// Координаты предыдущей и следующей точки. Обрабатываем граничные точки
		int prevPointIndex = i == 0 ? 0 : i - 1;
		int nextPointIndex = i == g_linePoints.size() - 1 ? g_linePoints.size() - 1 : i + 1;
		vec3 vec = g_linePoints[nextPointIndex] - g_linePoints[prevPointIndex]; // координаты вектора
		linePointsNormals[i] = vec3(-vec.y, vec.x, 0); // нормаль к нему
	}


	// Получаем все точки фрагментов (друг за другом, в порядке поворотов)
	vector<vec3> fragmentsPoints(g_linePoints); // в точках фрагментов сначала только точки линии
	vector<vec3> fragPointsNormals; // вектор нормалей точек фрагмента
	float rotationDeg = 360.0 / g_3dModelFragmentsCount; // угол между одним фрагментом и соседним с ним
	int fragmentPointsCount = g_linePoints.size(); // количество точек во фрагменте (кол-во точек в линии)
	// Добавляем нормали первого фрагмента (исходной линии)
	for (int i = 0; i < fragmentPointsCount; i++)
		fragPointsNormals.push_back(linePointsNormals[i]);
	// Первый фрагмент - сама линия. Начинаем со второго - т.е. с одного поворота
	for (int rotationIndex = 1; rotationIndex < g_3dModelFragmentsCount; rotationIndex++) {
		mat4 rotationMat = mat4::rotate(mat4(), rotationDeg * rotationIndex, vec3(-1, 0, 0)); // матрица поворота по оси X (т.е. вверх)
		// Каждую точку предыдущего фрагмента умножаем слева на матрицу поворота, и закидываем в вектор точек фрагментов
		for (int i = 0; i < fragmentPointsCount; i++) {
			vec3 p = rotationMat * g_linePoints[i];
			vec3 n = rotationMat * linePointsNormals[i];
			fragmentsPoints.push_back(p);
			fragPointsNormals.push_back(n);
		}
	}
	// И последним фрагментом закидываем первый - чтоб было удобней соединять последний с первым
	for (int i = 0; i < fragmentPointsCount; i++) {
		fragmentsPoints.push_back(g_linePoints[i]);
		fragPointsNormals.push_back(linePointsNormals[i]);
	}

	// Массив вершин. Вершин = количество точек. На каждую вершину 9 атрибутов
	int fragmentsPointsCount = fragmentsPoints.size();
	int verticesLength = fragmentsPoints.size() * 9; // длина массива vertices
	GLfloat* vertices = new GLfloat[verticesLength];
	for (int pointIndex = 0; pointIndex < fragmentsPointsCount; pointIndex++) {
		int curVertexBeginIndex = 9 * pointIndex; // начало атрибутов текущей вершины
		vertices[curVertexBeginIndex + 0] = fragmentsPoints[pointIndex].x; // задаём x вершины
		vertices[curVertexBeginIndex + 1] = fragmentsPoints[pointIndex].y; // задаём y вершины
		vertices[curVertexBeginIndex + 2] = fragmentsPoints[pointIndex].z; // задаём z вершины
		vertices[curVertexBeginIndex + 3] = g_3dModelColor.R; // задаём r вершины
		vertices[curVertexBeginIndex + 4] = g_3dModelColor.G; // задаём g вершины
		vertices[curVertexBeginIndex + 5] = g_3dModelColor.B; // задаём b вершины
		vertices[curVertexBeginIndex + 6] = fragPointsNormals[pointIndex].x; // задаём x нормали вершины
		vertices[curVertexBeginIndex + 7] = fragPointsNormals[pointIndex].y; // задаём y нормали вершины
		vertices[curVertexBeginIndex + 8] = fragPointsNormals[pointIndex].z; // задаём z нормали вершины
	}

	// Массив индексов - порядок обхода вершин для построения треугольников. Кол-во треугольников м/у соседними фрагментами * 3 вершины * кол-во таких соединений
	int indicesLength = (g_linePoints.size() * 2 - 2) * 3 * g_3dModelFragmentsCount;
	GLuint* indices = new GLuint[indicesLength];
	// Для каждого фрагмента: текущий фрагмент соединяем со следующим
	int indexIndex = 0;
	for (int fragmentIndex = 0; fragmentIndex < g_3dModelFragmentsCount; fragmentIndex++) {
		int curFragmentBeginIndex = fragmentPointsCount * fragmentIndex; // индекс начала индексов для текущего фрагмента
		// Рисуем прямоугольник из двух треугольников
		for (int fragPointIndex = 0; fragPointIndex < fragmentPointsCount - 1; fragPointIndex++) {
			// Первый треугольник
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + 1;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + fragmentPointsCount;
			indexIndex++;
			// Второй треугольник
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + fragmentPointsCount;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + 1;
			indexIndex++;
			indices[indexIndex] = curFragmentBeginIndex + fragPointIndex + fragmentPointsCount + 1;
			indexIndex++;
		}
	}

	// Толкаем массивы на видяху

	// Говорим, что нам нужен один массив настроек (vao)
	glGenVertexArrays(1, &g_3dModel.vao);
	// Биндим его. После этого система автоматом неявно будет писать в этот vao нужное - Memory Layout
	glBindVertexArray(g_3dModel.vao);

	// Создаём буффер под вертексный буфер (vbo), одну штуку
	glGenBuffers(1, &g_3dModel.vbo);
	// Биндим его на роль "массив вершин"
	glBindBuffer(GL_ARRAY_BUFFER, g_3dModel.vbo);
	// Блокирующим образом копируем память (vertices) из ОЗУ на видяху. 
	// Говорим, 20 float'ов, откуда берём данные. 
	// GL_STATIC_DRAW - говорим о том, что будем просто рисовать и ничего менять не будем. Просто хинт видяхе.
	glBufferData(GL_ARRAY_BUFFER, verticesLength * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// Создаём массив индексов, одну штуку
	glGenBuffers(1, &g_3dModel.ibo);
	// Биндим его на соответствующую роль
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_3dModel.ibo);
	// Копируем на видяху
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesLength * sizeof(GLuint), indices, GL_STATIC_DRAW);
	// Сохраним количество индексов для себя
	g_3dModel.indexCount = indicesLength;

	// Данные на видяхе, но она не знает, как их интерпретировать. Нужно сообщить ей об этом

	// Включаем атрибут номер ноль (координаты)
	glEnableVertexAttribArray(0);
	// Настраиваем атрибут номер ноль: 3 - трёхкомпонентный, тип - float, 
	// GL_FALSE - используем, как есть (отключаем нормирование для векторов)
	// 5 * sizeof(float) - шаг, с которым надо выхватывать этот атрибут из массива
	// Затем - смещение к первому вхождению этого атрибута, кастованное к указателю
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (const GLvoid*)0);
	// Включаем атрибут номер один (цвет)
	glEnableVertexAttribArray(1);
	// Настраиваем атрибут номер один: аналогично
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
	// Включаем атрибут номер два (нормаль)
	glEnableVertexAttribArray(2);
	// Настраиваем атрибут номер один: аналогично
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (const GLvoid*)(6 * sizeof(GLfloat)));
	// * Есть ограничения на кол-во атрибутов, но нам хватит: два точно есть (координаты + цвет)

	// И всё, что мы сейчас делали, слушал забинденный vao, и всё записал
	// Данные для конвейера готовы

	// Удаляем наши массивы
	delete[] vertices;
	delete[] indices;
	delete[] linePointsNormals;

	return g_3dModel.vbo != 0 && g_3dModel.ibo != 0 && g_3dModel.vao != 0;
}


// Инициализация графических ресурсов
bool init()
{
	// Устанавливаем белый начальным цветом буфера цвета
	// Т.е. задаём цвет заливки (цвет очистки буфера цвета)
	// Цветовых моделей несколько, и чтоб от этого абстрагироваться,
	// принимаемые параметры = 0..1 (и дальше они сами там в нужную модель приводятся)
	// Указание цветов: RGBA
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// Включаем тест буфера глубины
	glEnable(GL_DEPTH_TEST);

	// Затем создаём шейдеры и модель (нет)
	return createShaderProgram();
}


// Обработка события масштабирования окна
void reshape(GLFWwindow* window, int width, int height)
{
	// Говорим, что такое у нас порт просмотра: 
	// (0,0) - координаты его левого нижнего угла
	glViewport(0, 0, width, height);
	// Сохраняем новые размеры экрана
	g_windowWidth = width;
	g_windowHeight = height;
}

// Обработка события нажатия мыши
void mouseButtons_callback(GLFWwindow* window, int button, int action, int mods) {
	// Если была нажата левая кнопка - отмечаем
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// Получаем позицию курсора
		double xpos, ypos;
		glfwGetCursorPos(g_window, &xpos, &ypos);
		// Координаты окна начинаются в левом верхнем углу, а нам бы в левом нижнем (приводим к координатам сцены)
		ypos = g_windowHeight - ypos;
		// Создаём точку в виде вектора
		vec3 posPoint = vec3(xpos, ypos, 0);
		// Печатаем координаты в консоль
		cout << "MouseClick: x = " << posPoint.x << " y = " << posPoint.y << "\n";
		// Проверяем, удалена ли поставленная точка от остальных на нужное расстояние
		for (int i = 0; i < g_markedPoints.size(); i++) {
			vec3 vec = posPoint - g_markedPoints[i]; // вектор между двумя точками
			// Если длина вектора не удовлетворяет минимальной - выходим
			if (vec3::length(vec) < g_safePointsDistance)
				return;
		}
		// Если проверка прошла - добавляем точку в список точек
		g_markedPoints.push_back(posPoint);
		//// И пока в список точек линии
		//g_linePoints.push_back(posPoint);
		// И проводим интерполяцию
		if (g_markedPoints.size() > 1) {
			//g_linePoints = GetLinePoints(g_markedPoints, g_intermediatePointsCount);
			g_linePoints = g_markedPoints;
		}
		// И пересоздаём модель точек. Теперь у нас там по крайней мере одна точка есть
		bool f = createPointsModel();
		// А если точек больше одной, то создаём и модель линии
		f = createLineModel();
	}
	// Если была нажата правая кнопка мыши - переходим в 3D режим
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		// Если режим 3D уже включён - сбрасываем всё
		if (g_is3D) {
			g_markedPoints.clear();
			g_linePoints.clear();
			g_is3D = false;
		}
		
		// Если меньше двух точек - выходим
		if (g_markedPoints.size() < 2)
			return;
		g_is3D = true;
		create3dModel();

	}
}



// Отрисовка сцены
void draw()
{
	// Очищаем буфер цвета
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Если 3D Режим
	if (g_is3D) {
		// Включаем нужную шейдерную программу
		glUseProgram(g_3dShaderProgram);
		// Включаем модель точек (указываем, как интерпретировать вершины)
		glBindVertexArray(g_3dModel.vao);

		mat4 m = mat4(); // матрица model
		// Позиционируем на сцене
		m = mat4::translate(m, vec3(0, 0, -250));
		m = mat4::scale(m, vec3(0.1, 0.1, 0.1));
		// Поворачиваем по горизонтали в зависимости от времени
		m = mat4::translate(m, vec3(50, 50, 50));
		m = mat4::rotate(m, glfwGetTime()*23, vec3(0.0f, -1.0f, 0.0f));
		m = mat4::translate(m, vec3(-50, -50, -50));
		mat4 v = mat4::getViewMatrix(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0)); // матрица view
		// Преобразуем точки в NDC с помощью матрицы параллельной проекции с заданными параметрами
		mat4 p = mat4::getPerspectiveProjectionMatrix(0.01, 1000, (float)g_windowWidth, (float)g_windowHeight, 45.0f); // матрица projection
		mat4 mvp = p * v * m; // матрица mvp
		mat4 mv = v * m; // матрица mvp
		// Закидываем матрицу mvp на видяху
		glUniformMatrix4fv(g_uMVP3D, 1, GL_FALSE, mvp.getMatrixPtr());
		// Закидываем матрицу mv на видяху
		glUniformMatrix4fv(g_uMV3D, 1, GL_FALSE, mv.getMatrixPtr());

		// Отрисовка: интерпретируем индексы как для отрисовки треугольников
		// GL_UNSIGNED_INT = 2^32 - размер индексного регистра (сколько может адресовать за раз)
		// NULL - возьми индексы из памяти видяхи. Могли бы указатель на массив дать, но неэффективно
		glDrawElements(GL_TRIANGLES, g_3dModel.indexCount, GL_UNSIGNED_INT, NULL);
		return;
	}

	// Включаем нужную шейдерную программу
	glUseProgram(g_2dShaderProgram);
	// Если поставлена хотя бы одна точка - отрисовываем модель точек
	if (g_markedPoints.size() > 0) {
		// Включаем модель точек (указываем, как интерпретировать вершины)
		glBindVertexArray(g_pointsModel.vao);

		// Преобразуем точки в NDC с помощью матрицы параллельной проекции с заданными параметрами. 
		mat4 m = mat4(); // матрица model
		mat4 v = mat4::getViewMatrix(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0)); // матрица view
		mat4 p = mat4::getParallelProjectionMatrix(0, g_windowWidth, 0, g_windowHeight, -1, 1); // матрица projection
		mat4 mvp = p * v * m; // матрица mvp
		// Закидываем матрицу mvp на видяху
		glUniformMatrix4fv(g_uMVP2D, 1, GL_FALSE, mvp.getMatrixPtr());

		// Отрисовка: интерпретируем индексы как для отрисовки треугольников
		// GL_UNSIGNED_INT = 2^32 - размер индексного регистра (сколько может адресовать за раз)
		// NULL - возьми индексы из памяти видяхи. Могли бы указатель на массив дать, но неэффективно
		glDrawElements(GL_TRIANGLES, g_pointsModel.indexCount, GL_UNSIGNED_INT, NULL);
	}
	// Если поставлено две и больше точек - отрисовываем модель линии
	if (g_markedPoints.size() > 1) {
		// Включаем модель линии (указываем, как интерпретировать вершины)
		glBindVertexArray(g_lineModel.vao);

		// Преобразуем точки в NDC с помощью матрицы параллельной проекции с заданными параметрами. 
		mat4 m = mat4(); // матрица model
		mat4 v = mat4::getViewMatrix(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0)); // матрица view
		mat4 p = mat4::getParallelProjectionMatrix(0, g_windowWidth, 0, g_windowHeight, -1, 1); // матрица projection
		mat4 mvp = p * v * m; // матрица mvp
		// Закидываем матрицу mvp на видяху
		glUniformMatrix4fv(g_uMVP2D, 1, GL_FALSE, mvp.getMatrixPtr());

		// Отрисовка: интерпретируем индексы как для отрисовки последовательности линий
		// GL_UNSIGNED_INT = 2^32 - размер индексного регистра (сколько может адресовать за раз)
		// NULL - возьми индексы из памяти видяхи. Могли бы указатель на массив дать, но неэффективно
		glDrawElements(GL_LINE_STRIP, g_lineModel.indexCount, GL_UNSIGNED_INT, NULL);
	}
}


// Чистка четырёх наших ресурсов (шейдерной программы и буферов vbo, ibo, vao)
void cleanup()
{
	// Шейдеры
	if (g_2dShaderProgram != 0)
		glDeleteProgram(g_2dShaderProgram);
	if (g_3dShaderProgram != 0)
		glDeleteProgram(g_3dShaderProgram);
	// Модель точек
	if (g_pointsModel.vbo != 0)
		glDeleteBuffers(1, &g_pointsModel.vbo);
	if (g_pointsModel.ibo != 0)
		glDeleteBuffers(1, &g_pointsModel.ibo);
	if (g_pointsModel.vao != 0)
		glDeleteVertexArrays(1, &g_pointsModel.vao);
	// Модель линии
	if (g_lineModel.vbo != 0)
		glDeleteBuffers(1, &g_lineModel.vbo);
	if (g_lineModel.ibo != 0)
		glDeleteBuffers(1, &g_lineModel.ibo);
	if (g_lineModel.vao != 0)
		glDeleteVertexArrays(1, &g_lineModel.vao);
	// Модель 3D объекта
	if (g_3dModel.vbo != 0)
		glDeleteBuffers(1, &g_3dModel.vbo);
	if (g_3dModel.ibo != 0)
		glDeleteBuffers(1, &g_3dModel.ibo);
	if (g_3dModel.vao != 0)
		glDeleteVertexArrays(1, &g_3dModel.vao);
}


// Инициализация OpenGL
bool initOpenGL()
{
	// Инициализируем библиотеку GLFW перед её использованием
	// GLFW - библиотека для работы с окнами и т.д. Не торт, но нам подойдёт
	if (!glfwInit())
	{
		cout << "Failed to initialize GLFW" << endl;
		return false;
	}

	// Запрашиваем OpenGL версии 3.3 (без устаревших функций?)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Создаём окно (если нет, значит, OpenGL нет, или ещё что, а это плохо)
	g_window = glfwCreateWindow(g_windowWidth, g_windowHeight, "OpenGL Test", NULL, NULL);
	if (g_window == NULL)
	{
		cout << "Failed to open GLFW window" << endl;
		glfwTerminate();
		return false;
	}

	// Инициализируем графический контекст OpenGL и захватываем его - делаем текущим (из многопоточности уже)
	glfwMakeContextCurrent(g_window);

	// Инициализируем внутреннюю переменную библиотеки GLEW для активации OpenGL Core Profile
	// GLEW - "рулевой расширений", библиотека, разрешающая ссылки на OpenGL. Хорошая
	glewExperimental = true;

	// Инициализируем функции GLEW
	if (glewInit() != GLEW_OK)
	{
		cout << "Failed to initialize GLEW" << endl;
		return false;
	}

	// Убеждаемся в том, что сможем захватиь нажатие клавиши Escape.
	// И ещё чтоб мог ловить всякие штуки типа exception'ов?
	glfwSetInputMode(g_window, GLFW_STICKY_KEYS, GL_TRUE);


	// Важно: цепляемся к событию ресайза (reshape) платформо-независимым образом
	// В reshape говорим, что такое у нас порт просмотра
	glfwSetFramebufferSizeCallback(g_window, reshape);

	// Назначаем обработчик события на нажатие мыши
	glfwSetMouseButtonCallback(g_window, mouseButtons_callback);

	return true;
}


// Очистка ресурсов OpenGL (GLFW)
void tearDownOpenGL()
{
	// Завершение GLWF
	glfwTerminate();
}

int main()
{
	// Инициализация OpenGL
	if (!initOpenGL())
		return -1;

	// Инициализация графических ресурсов - этап загрузки приложения
	bool isOk = init();

	// Если инициализация ресурсов успешна - работаем дальше
	if (isOk)
	{
		// Главный цикл работает, пока окно не было закрыто и не была нажата кнопка Escape
		while (glfwGetKey(g_window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(g_window) == 0)
		{
			// Отрисовываем сцену
			draw();

			// Смена буферов (типа меняем кадры)
			glfwSwapBuffers(g_window);
			// Получаем события?
			glfwPollEvents();
		}
	}

	// Очищаем графические ресурсы
	cleanup();

	// Останавливаем OpenGL
	tearDownOpenGL();

	return isOk ? 0 : -1;
}
