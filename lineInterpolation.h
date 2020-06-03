#pragma once

#include <vector>
#include "vec3.h"
#include <iostream>
#include <string>
using namespace std;

// ������ ������� - �������
void printArray(double* array, int n, string header) {
	cout << header << endl;
	for (int i = 0; i < n; i++)
		cout << array[i] << endl;
	cout << endl;
}

// ������ ������� - �������
void printMatrix(double** matrix, int n, string header) {
	cout << header << endl;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++)
			cout << matrix[i][j] << '\t';
		cout << endl;
	}
	cout << endl;
}


// ����� �������� ������� ���� (Traditional Matrix Algorithm)
// A - ������� �������
// B - ������ ��������� ������
// n - ����������� �������
double* TDMA(double** A, double* B, int n) {
	double* a = new double[n]; // ������ ������������� a
	double* b = new double[n]; // ������ ������������� b
	double* x = new double[n]; // ������ ������� x
	// b1*x1 + c1*x2 = d1 => x1 = -(c1/b1)*x2 + (d1/b1)
	// y1 = b1, a1 = -(c1/y1), b1 = d1/y1
	double y = A[0][0];
	a[0] = -A[0][1] / y;
	b[0] = B[0] / y;
	// ������ ��� ������ �������� ��� i = 1, ... , n-2 (������� ������������� ���������)
	for (int i = 1; i <= n - 2; i++) {
		y = A[i][i] + A[i][i - 1] * a[i - 1];
		a[i] = -A[i][i + 1] / y;
		b[i] = (B[i] - A[i][i - 1] * b[i - 1]) / y;
	}
	// �� ��������� ��������� ������� ���� x[n] = b[n]
	y = A[n - 1][n - 1] + A[n - 1][n - 2] * a[n - 2];
	b[n - 1] = (B[n - 1] - A[n - 1][n - 2] * b[n - 2]) / y;
	x[n - 1] = b[n - 1];
	// �������� ��� ��������
	for (int i = n - 2; i >= 0; i--)
		x[i] = a[i] * x[i + 1] + b[i];
	// ������ �������� ������ � ���������� x
	delete[] a;
	delete[] b;
	return x;
}

// ���������� �������� ������� � �������� �����
// x - �����, ��� ������� ����� ��������� ��������
// X - x[i]
// a, b, c, d - a[i], b[i], c[i], d[i]
// ��� i - ����� �������
double CalcSplineValue(double x, double X, double a, double b, double c, double d) {
	return a + b * (x - X) + c / 2 * (x - X) * (x - X) + d / 6 * (x - X) * (x - X) * (x - X);
}

// ���������� ����� ����� � ������� ������������ ���������� �������� ������� 1
// points - ������� ����� ����� (x � y)
// segmentsCount - ����������� ���������� ������������� �����
vector<vec3> GetLinePoints(vector<vec3> points, int intermediatePointsCount) {
	int n = points.size(); // ���������� ������� �����
	// ������� ��� ���������� - x � y �����
	double* x = new double[n];
	double* f = new double[n];
	for (int i = 0; i < n; i++) {
		x[i] = points[i].x;
		f[i] = points[i].y;
	}

	// h - ����� �������� ������������ (�������� ������ ������, ���� ������� ���������). ���������
	double* h = new double[n + 1];
	for (int i = 1; i < n + 1; i++)
		h[i - 1] = x[i] - x[i - 1];
	// ������������ a. ����� f(x[i])
	double* a = new double[n - 1];
	for (int i = 1; i < n; i++)
		a[i - 1] = f[i];
	// ������������ c. ����� �� �����, ����� ������ ��������������� �������
	double* c;
	// ��������� �������, ������� ���������� ������
	double** A = new double* [n - 1];
	double* B = new double[n - 1];
	// ��� �� ����������
	for (int i = 1; i < n; i++) {
		int curEquationIndex = i - 1; // ������ �������� ��������� � �������
		// ������ �������� � ��������� ������
		A[curEquationIndex] = new double[n - 1];
		for (int j = 0; j < n - 1; j++)
			A[curEquationIndex][j] = 0;
		// ������ ��������� ������ B
		B[curEquationIndex] = 6 * ((f[i + 1] - f[i]) / h[i + 1] - (f[i] - f[i - 1]) / h[i]);
		// �������� ������� A, ������� �� ������� (��� � �������� ��� ��� ������� ����������)
		if (i != 1)
			A[curEquationIndex][curEquationIndex - 1] = h[i];
		A[curEquationIndex][curEquationIndex] = 2 * (h[i] + h[i + 1]);
		if (i != n - 1)
			A[curEquationIndex][curEquationIndex + 1] = h[i + 1];
	}
	// ������ ����������� �������, ������� ������������ c
	c = TDMA(A, B, n - 1);
	// ������������ d
	double* d = new double[n - 1];
	for (int i = 1; i < n ; i++) {
		int curIndex = i - 1; // ������ �������� ������������ � �������
		double cCur = c[curIndex]; // c[i]
		double cPrev = i == 1 ? 0 : c[curIndex - 1]; // c[i-1]
		d[curIndex] = (cCur - cPrev) / h[i];
	}
	// ������������ b
	double* b = new double[n - 1];
	for (int i = 1; i < n; i++) {
		int curIndex = i - 1; // ������ �������� ������������ � �������
		b[curIndex] = (c[curIndex] * h[i]) / 2 - (1 / 6) * d[curIndex] * h[i] * h[i] + (f[i] - f[i - 1]) / h[i];
	}

	// ������������ ��������� - �������������
	vector<vec3> res;
	// ��� �� ���������
	for (int i = 1; i < n; i++) {
		double step = (x[i] - x[i - 1]) / intermediatePointsCount;
		// ��������� ����� ������ �������
		res.push_back(points[i - 1]);
		// ��������� ������������� �����
		for (int j = 1; j <= intermediatePointsCount; j++) {
			double xCur = x[i - 1] + step * j; // ������� ����� x
			double yCur = CalcSplineValue(xCur, f[i], a[i - 1], b[i - 1], c[i - 1], d[i - 1]);
			res.push_back(vec3(xCur, yCur, 0));
		}
	}
	// ��������� ��������� �����
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


	// ������� ��������� �������
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