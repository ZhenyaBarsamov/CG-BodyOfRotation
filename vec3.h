#pragma once
#include <math.h>
#include <iostream>

using namespace std;

// ����� ���������������� float-������� ��� �������� ���������
class vec3 {
public:
	// ������ ��������� �������
	float x;
	// ������ ��������� �������
	float y; 
	// ������ ��������� �������
	float z;

	// �����������
	vec3(float _x, float _y, float _z) {
		x = _x;
		y = _y;
		z = _z;
	}

	// ����������� �� ���������
	vec3() : vec3(0, 0, 0) {
		// ��� �������� �������� ������� ��� ������ ������ �����������
	}

	// ���������� ������ �������
	static float length(vec3 vec) {
		return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
	}

	// ��������� ������������ ���� ��������
	static vec3 crossProduct(vec3 vec1, vec3 vec2) {
		// ��������� �� �������, ����� ������������ ������� ������� (������ ��������� ���������� => �������� ����)
		return vec3(vec1.y * vec2.z - vec1.z * vec2.y, vec1.z * vec2.x - vec1.x * vec2.z, vec1.x * vec2.y + vec1.y * vec2.x);
	}

	// ����� ���� ��������
	vec3 operator + (vec3 vec) {
		return vec3(x + vec.x, y + vec.y, z + vec.z);
	}

	// �������� ���� ��������
	vec3 operator - (vec3 vec) {
		return vec3(x - vec.x, y - vec.y, z - vec.z);
	}

	// ������� ������� �� �����
	vec3 operator / (float num) {
		return vec3(x / num, y / num, z / num);
	}

	// ������������ �������
	static vec3 normalize(vec3 vec) {
		return vec / vec3::length(vec);
	}

	// ������ �������
	void print() {
		cout <<"������: " << x << "\t" << y << "\t" << z << endl;
	}
};