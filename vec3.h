#pragma once
#include <math.h>
#include <iostream>

using namespace std;

// Класс трёхкомпонентного float-вектора для хранения координат
class vec3 {
public:
	// Первый компонент вектора
	float x;
	// Второй компонент вектора
	float y; 
	// Третий компонент вектора
	float z;

	// Конструктор
	vec3(float _x, float _y, float _z) {
		x = _x;
		y = _y;
		z = _z;
	}

	// Конструктор по умолчанию
	vec3() : vec3(0, 0, 0) {
		// для создания нулевого вектора был вызван другой конструктор
	}

	// Вычисление модуля вектора
	static float length(vec3 vec) {
		return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
	}

	// Векторное произведение двух векторов
	static vec3 crossProduct(vec3 vec1, vec3 vec2) {
		// Находится по формуле, через определители второго порядка (второе слагаемое вычитается => поменяли знак)
		return vec3(vec1.y * vec2.z - vec1.z * vec2.y, vec1.z * vec2.x - vec1.x * vec2.z, vec1.x * vec2.y + vec1.y * vec2.x);
	}

	// Сумма двух векторов
	vec3 operator + (vec3 vec) {
		return vec3(x + vec.x, y + vec.y, z + vec.z);
	}

	// Разность двух векторов
	vec3 operator - (vec3 vec) {
		return vec3(x - vec.x, y - vec.y, z - vec.z);
	}

	// Деление вектора на число
	vec3 operator / (float num) {
		return vec3(x / num, y / num, z / num);
	}

	// Нормирование вектора
	static vec3 normalize(vec3 vec) {
		return vec / vec3::length(vec);
	}

	// Печать вектора
	void print() {
		cout <<"Вектор: " << x << "\t" << y << "\t" << z << endl;
	}
};