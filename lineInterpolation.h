#pragma once

#include <vector>
#include "vec3.h"
#include <iostream>
#include <string>
using namespace std;

// Печать массива - отладка
void printArray(double* array, int n, string header) {
	cout << header << endl;
	for (int i = 0; i < n; i++)
		cout << array[i] << endl;
	cout << endl;
}

// Печать матрицы - отладка
void printMatrix(double** matrix, int n, string header) {
	cout << header << endl;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++)
			cout << matrix[i][j] << '\t';
		cout << endl;
	}
	cout << endl;
}


// Метод прогонки решения СЛАУ (Traditional Matrix Algorithm)
// A - матрица системы
// B - стобец свободных членов
// n - размерность матрицы
double* TDMA(double** A, double* B, int n) {
	double* a = new double[n]; // вектор коэффициентов a
	double* b = new double[n]; // вектор коэффициентов b
	double* x = new double[n]; // вектор решения x
	// b1*x1 + c1*x2 = d1 => x1 = -(c1/b1)*x2 + (d1/b1)
	// y1 = b1, a1 = -(c1/y1), b1 = d1/y1
	double y = A[0][0];
	a[0] = -A[0][1] / y;
	b[0] = B[0] / y;
	// Прямой ход метода прогонки для i = 1, ... , n-2 (включая предпоследнее уравнение)
	for (int i = 1; i <= n - 2; i++) {
		y = A[i][i] + A[i][i - 1] * a[i - 1];
		a[i] = -A[i][i + 1] / y;
		b[i] = (B[i] - A[i][i - 1] * b[i - 1]) / y;
	}
	// На последнем уравнении прямого хода x[n] = b[n]
	y = A[n - 1][n - 1] + A[n - 1][n - 2] * a[n - 2];
	b[n - 1] = (B[n - 1] - A[n - 1][n - 2] * b[n - 2]) / y;
	x[n - 1] = b[n - 1];
	// Обратный ход прогонки
	for (int i = n - 2; i >= 0; i--)
		x[i] = a[i] * x[i + 1] + b[i];
	// Чистим ненужную память и возвращаем x
	delete[] a;
	delete[] b;
	return x;
}

// Вычисление значения сплайна в заданной точке
// x - точка, для которой будет вычислено значение
// X - x[i]
// a, b, c, d - a[i], b[i], c[i], d[i]
// где i - номер отрезка
double CalcSplineValue(double x, double X, double a, double b, double c, double d) {
	return a + b * (x - X) + c / 2 * (x - X) * (x - X) + d / 6 * (x - X) * (x - X) * (x - X);
}

// Вычисление точек линии с помощью интерполяции кубическим сплайним дефекта 1
// points - опорные точки линии (x и y)
// segmentsCount - необходимое количество промежуточных точек
vector<vec3> GetLinePoints(vector<vec3> points, int intermediatePointsCount) {
	int n = points.size(); // количество опорных точек
	// Массивы для понятности - x и y точек
	double* x = new double[n];
	double* f = new double[n];
	for (int i = 0; i < n; i++) {
		x[i] = points[i].x;
		f[i] = points[i].y;
	}

	// h - длины отрезков интерполяции (выделяем больше памяти, чтоб индексы совпадали). Заполняем
	double* h = new double[n + 1];
	for (int i = 1; i < n + 1; i++)
		h[i - 1] = x[i] - x[i - 1];
	// Коэффициенты a. Равны f(x[i])
	double* a = new double[n - 1];
	for (int i = 1; i < n; i++)
		a[i - 1] = f[i];
	// Коэффициенты c. Чтобы их найти, нужно решить трёхдиагональную систему
	double* c;
	// Формируем систему, которую необходимо решить
	double** A = new double* [n - 1];
	double* B = new double[n - 1];
	// Идём по уравнениям
	for (int i = 1; i < n; i++) {
		int curEquationIndex = i - 1; // индекс текущего уравнения в матрице
		// Создаём уравение и заполняем нулями
		A[curEquationIndex] = new double[n - 1];
		for (int j = 0; j < n - 1; j++)
			A[curEquationIndex][j] = 0;
		// Вектор свободных членов B
		B[curEquationIndex] = 6 * ((f[i + 1] - f[i]) / h[i + 1] - (f[i] - f[i - 1]) / h[i]);
		// Элементы матрицы A, которые не нулевые (три в середине или два крайних уравнениях)
		if (i != 1)
			A[curEquationIndex][curEquationIndex - 1] = h[i];
		A[curEquationIndex][curEquationIndex] = 2 * (h[i] + h[i + 1]);
		if (i != n - 1)
			A[curEquationIndex][curEquationIndex + 1] = h[i + 1];
	}
	// Решаем построенную систему, находим коэффициенты c
	c = TDMA(A, B, n - 1);
	// Коэффициенты d
	double* d = new double[n - 1];
	for (int i = 1; i < n ; i++) {
		int curIndex = i - 1; // индекс текущего коэффициента в массиве
		double cCur = c[curIndex]; // c[i]
		double cPrev = i == 1 ? 0 : c[curIndex - 1]; // c[i-1]
		d[curIndex] = (cCur - cPrev) / h[i];
	}
	// Коэффициенты b
	double* b = new double[n - 1];
	for (int i = 1; i < n; i++) {
		int curIndex = i - 1; // индекс текущего коэффициента в массиве
		b[curIndex] = (c[curIndex] * h[i]) / 2 - (1 / 6) * d[curIndex] * h[i] * h[i] + (f[i] - f[i - 1]) / h[i];
	}

	// Коэффициенты вычислены - интерполируем
	vector<vec3> res;
	// Идём по сегментам
	for (int i = 1; i < n; i++) {
		double step = (x[i] - x[i - 1]) / intermediatePointsCount;
		// Добавляем точку начала отрезка
		res.push_back(points[i - 1]);
		// Добавляем промежуточные точки
		for (int j = 1; j <= intermediatePointsCount; j++) {
			double xCur = x[i - 1] + step * j; // текущая точка x
			double yCur = CalcSplineValue(xCur, f[i], a[i - 1], b[i - 1], c[i - 1], d[i - 1]);
			res.push_back(vec3(xCur, yCur, 0));
		}
	}
	// Добавляем последнюю точку
	res.push_back(points[points.size() - 1]);

	printArray(a, n - 1, "a");
	printArray(c, n - 1, "c");
	printMatrix(A, n - 1, "A");
	printArray(B, n - 1, "B");
	printArray(d, n - 1, "d");
	printArray(b, n - 1, "b");
	cout << "Points:" << endl;
	for (int i = 0; i < res.size(); i++)
		cout << res[i].x << '\t' << res[i].y << endl;
	cout << "----------------------------------------------" << endl;


	// Удаляем созданные массивы
	delete[] x;
	delete[] f;
	delete[] h;
	delete[] a;
	delete[] c;
	for (int i = 0; i < n - 1; i++)
		delete[] A[i];
	delete[] A;
	delete[] B;
	delete[] d;
	delete[] b;
	return res;
}