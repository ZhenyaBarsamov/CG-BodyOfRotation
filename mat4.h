#pragma once
#include <iostream>
#define _USE_MATH_DEFINES // для доступа к математическим константам
#include <math.h>
#include "vec3.h"

using namespace std;

// Класс матрицы размерности 4 x 4
class mat4 {
private:
	// Поле - матрица в виде одномерного массива.
	// Матрица хранится в массиве по столбцам
	float m[16];

public:
	// Получение указателя на инкапсулируемую матрицу (массив)
	float* getMatrixPtr() {
		return m;
	}

	// Присвоение новой матрицы
	void setMatrix(float mat[16]) {
		// Копируем память из matrix в m
		memcpy(m, mat, 16 * sizeof(float));
	}

	// Конструктор для создания объекта матрицы из массива
	mat4(float a[16]) {
		this->setMatrix(a);
	}

	// Конструктор для создания диагональной матрицы с dNum на диагонали
	mat4(float dNum) : 
		m{	dNum, 0, 0, 0,
			0, dNum, 0, 0,
			0, 0, dNum, 0,
			0, 0, 0, dNum } {
		// Для инициализации поля использовались списки инициализации
	}

	// Конструктор для создания единичной матрицы
	mat4() : mat4(1) {
		// Для создания единичной матрицы был вызван другой конструктор
	}

	// Умножение двух матриц
	mat4 operator * (mat4 mat) {
		float res[16];
		// Помним, что матрица у нас хранится массивом, по столбцам
		for (int row = 0; row < 4; row++)
			for (int col = 0; col < 4; col++) {
				float sum = 0;
				// Получаем произведение строки на столбец
				for (int i = 0; i < 4; i++)
					sum += m[col + 4*i] * mat.getMatrixPtr()[4*row + i];
				res[4 * row + col] = sum;
			}
		return mat4(res);
	}

	// Умножение матрицы на вектор (четвёртый компонент вектора считается равным нулю)
	vec3 operator * (vec3 vec) {
		vec3 res = vec3(0, 0, 0);
		// Получаем икс (четвёртый компонент вектора считаем равным нулю)
		res.x += m[0] * vec.x + m[4] * vec.y + m[8] * vec.z;
		res.y += m[1] * vec.x + m[5] * vec.y + m[9] * vec.z;
		res.z += m[2] * vec.x + m[6] * vec.y + m[10] * vec.z;
		return res;
	}

	// Печать матрицы
	void print() {
		cout << "Matrix:" << endl;
		for (int i = 0; i < 4; i++)
			cout << m[i] << "\t" << m[i + 4] << "\t" << m[i + 8] << "\t" << m[i + 12] << endl;
		cout << endl;
	}

	
	//-------СТАТИЧЕСКИЕ СОЗДАВАТОРЫ ДЛЯ АФИННЫХ ПРЕОБРАЗОВАНИЙ--------
	
	// Масштабирование
	// m - матрица модели
	// v - вектор, задающий масштабирование по соответствующим осям
	static mat4 scale(mat4 m, vec3 v) {
		// Заполняем также по столбцам
		float sMat[16] = {
			v.x, 0, 0, 0,
			0, v.y, 0, 0,
			0, 0, v.z, 0,
			0, 0, 0,   1
		};
		return m * mat4(sMat);
	}
	
	// Перенос
	// m - матрица модели
	// v - вектор, задающий перенос по соответствующим осям
	static mat4 translate(mat4 m, vec3 v) {
		// Заполняем также по столбцам
		float tMat[16] = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			v.x, v.y, v.z, 1
		};
		return m * mat4(tMat);
	}

	// Поворот
	// m - матрица модели
	// degAngle - угол поворота (градусы)
	// v - вектор, задающий ось, вокруг которой будет поворачиваться объект
	static mat4 rotate(mat4 m, float degAngle, vec3 v) {
		float radAngle= degAngle * M_PI / 180; // переводим в радианы
		float c = cos(radAngle);
		float s = sin(radAngle);
		// Заполняем также по столбцам
		float rMat[16] = {
			v.x*v.x*(1-c)+c,		v.y*v.x*(1-c)+v.z*s,	v.x*v.z*(1-c)-v.y*s,	0,
			v.x*v.y*(1-c)-v.z*s,	v.y*v.y*(1-c)+c,		v.y*v.z*(1-c)+v.x*s,	0,
			v.x*v.z*(1-c)+v.y*s,	v.y*v.z*(1-c)-v.x*s,	v.z*v.z*(1-c)+c,		0,
				0,							0,						0,				1
		};
		return m * mat4(rMat);
	}

	// Получить дефолтную матрицу модели m (единичная матрица) - хранит преобразование размещения объекта
	static mat4 getModelMatrix() {
		return mat4();
	}

	// Получить матрицу вида - преобразование камеры
	// e - точка, определяющая позицию наблюдателя на сцене
	// c - точка, определяющая центр видимой области
	// u - вектор, определяющий направление вверх для наблюдателя (вертикальная ориентация камеры)
	static mat4 getViewMatrix(vec3 e, vec3 c, vec3 u) {
		vec3 f = vec3::normalize(c - e);
		vec3 s = vec3::normalize(vec3::crossProduct(f, u));
		vec3 v = vec3::crossProduct(s, f);
		// Заполняем также по столбцам

		float m1[16] = {
			s.x,	v.x,	-f.x,	0,
			s.y,	v.y,	-f.y,	0,
			s.z,	v.z,	-f.z,	0,
			0,		0,		0,		1
		};

		float m2[16] = {
			1,		0,		0,		0,
			0,		1,		0,		0,
			0,		0,		1,		0,
			-e.x,	-e.y,	-e.z,	1
		};
		return mat4(m1) * mat4(m2);
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	// Получить матрицу параллельной проекции по заданным параметрам
	// l - left
	// r - right
	// b - bottom
	// t - top
	// b - near > 0 = 0.01
	// f - far = 1000.0
	static mat4 getParallelProjectionMatrix(float l, float r, float b, float t, float n, float f) {
		// Заполняем также по столбцам
		float ppMatr[16] = {
			2 / (r - l),					0,							0,				0,
			0,							2 / (t - b),					0,				0,
			0,								0,						-2 / (f - n),		0,
			-(r + l) / (r - l),		-(t + b) / (t - b),			-(f + n) / (f - n),		1
		};
		return ppMatr;
	}

	// Получить матрицу перспективной проекции по заданным параметрам
	// l - left
	// r - right
	// b - bottom
	// t - top
	// b - near > 0 = 0.01
	// f - far = 1000.0
	static mat4 getPerspectiveProjectionMatrix(float l, float r, float b, float t, float n, float f) {
		// Заполняем также по столбцам
		float ppMatr[16] =
		{
			2 * n / (t - b),				0,						0,					0,
			0,						2 * n / (r - l),				0,					0,
			(r + l) / (r - l),		(t + b) / (t - b),		-(f + n) / (f - n),			-1,
			0,								0,				-2 * f * n / (f - n),		0
		};
		return ppMatr;
	}

	// Получить матрицу перспективной проекции с углом обзора в градусах
	// b - near > 0 = 0.01
	// f - far = 1000.0
	// w - width = ширина экрана
	// h - heigth = высота экрана
	// tetaDeg - угол обзора (градусы)
	static mat4 getPerspectiveProjectionMatrix(float n, float f, float w, float h, float tetaDeg) {
		float tetaRad = tetaDeg * M_PI / 180;
		float tg = tanf(tetaRad / 2);
		return getPerspectiveProjectionMatrix(-n * tg, n * tg, -n * w / h * tg, n * w / h * tg, n, f);
	}

	// Получение массива, содержащего по столбцам матрицу mat с отброшенными четвёртыми измерениями
	// mat - матрица
	static float* getMat3ArrayFromMat4(mat4 mat) {
		float* matPtr = mat.getMatrixPtr();
		float* res = new float[9] {
			matPtr[0], matPtr[1], matPtr[2],
			matPtr[4], matPtr[5], matPtr[6],
			matPtr[8], matPtr[9], matPtr[10]
		};
		return res;
	}

	// Получение массива, содержащего по столбцам нормальную матрицу (матрицу нормалей)
	// modelViewMatrix - матрица ModelView (MV)
	static float* getNormalMatrixArray(mat4 modelViewMatrix) {
		// Обрезаем у матрицы четвёртое измерение
		float* mv3Array = getMat3ArrayFromMat4(modelViewMatrix);
		// Считаем определитель матрицы
		float det = (mv3Array[0] * mv3Array[4] * mv3Array[8]) + (mv3Array[3] * mv3Array[7] * mv3Array[2]) + (mv3Array[1] * mv3Array[5] * mv3Array[6])
			- (mv3Array[6] * mv3Array[4] * mv3Array[2]) - (mv3Array[1] * mv3Array[3] * mv3Array[8]) - (mv3Array[5] * mv3Array[7] * mv3Array[0]);
		// Считаем матрицу алгебраических дополнений (миноры плюс знак), и делим каждый элемент на определитель
		float* normalMatrixArray = new float[9] {
			(mv3Array[4] * mv3Array[8] - mv3Array[7] * mv3Array[5]) / det, (mv3Array[6] * mv3Array[5] - mv3Array[3] * mv3Array[8]) / det, (mv3Array[3] * mv3Array[7] - mv3Array[6] * mv3Array[4]) / det,
			(mv3Array[7] * mv3Array[2] - mv3Array[1] * mv3Array[8]) / det, (mv3Array[0] * mv3Array[8] - mv3Array[6] * mv3Array[2]) / det, (mv3Array[6] * mv3Array[1] - mv3Array[0] * mv3Array[7]) / det,
			(mv3Array[1] * mv3Array[5] - mv3Array[4] * mv3Array[2]) / det, (mv3Array[3] * mv3Array[2] - mv3Array[0] * mv3Array[5]) / det, (mv3Array[0] * mv3Array[4] - mv3Array[3] * mv3Array[1]) / det
		};
		delete[] mv3Array;
		// Получили нормальную матрицу - транспонировать не надо, т.к. нам нужна обратная транспонированная матрица
		return normalMatrixArray;
	}
};