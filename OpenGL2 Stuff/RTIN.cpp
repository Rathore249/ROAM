#include <iostream>
#include <vector>
#include <cmath>
#include "RTIN.h"
#include "HeightMap.h"

using namespace std;

RTIN::RTIN() {
	size = 0;
	e_T = 0;
	flags = 0;
	normalBuffer = 0;
	vertexBuffer = 0;
	indexBuffer = 0;
}

RTIN::~RTIN() {
	if (e_T != 0) {
		delete [] e_T;
		e_T = 0;
	}
	if (flags != 0) {
		delete [] flags;
		flags = 0;
	}
}

void RTIN::Draw() {
	DrawTriangle(0);
	//cout << "After root recursed" << endl;
}

int RTIN::Parent(int triangle) {
	if (triangle == 0) {
		return -1;
	}
	return (triangle - 1) >> 1;
}

int RTIN::Child(child c, int triangle) {
	int result = (triangle << 1) + c;
	if (result < size) 
		return result;
	else
		return -1;
}

//a result of -1 means there is no neighbor
int RTIN::Neighbor(neighbor n, int triangle) {
	if (triangle == 0) {
		return -1;
	} else if (triangle == 1) {
		if (n == B)
			return 2;
		else
			return -1;
	} else if (triangle == 2) {
		if (n == B)
			return 1;
		else
			return -1;
	} else {
		int parent = Parent(triangle);
		int nParent; //parents neighbor
		if (n == B) {
			if (triangle % 2 == 0) {
				nParent = Neighbor(R, parent);
				if (nParent == -1) {
					return -1;
				}
				return Child(LEFT, nParent);
			} else {
				nParent = Neighbor(L, parent);
				if (nParent == -1) {
					return -1;
				}
				return Child(RIGHT, nParent);
			}
		} else if (n == L) {
			if (triangle % 2 == 0) {
				nParent = Neighbor(B, parent);
				if (nParent == -1) {
					return -1;
				}
				return Child(LEFT, nParent);
			} else {
				return Child(RIGHT, parent);
			}
		} else if (n == R) {
			if (triangle % 2 == 0) {
				return Child(LEFT, parent);
			} else {
				nParent = Neighbor(B, parent);
				if (nParent == -1) {
					return -1;
				}
				return Child(RIGHT, nParent);
			}
		}
	}
	return 0;
}

void RTIN::ForceSplit(int triangle) {
	int baseNeighbor = Neighbor(B, triangle);
	if (baseNeighbor == -1) { 
		Split(triangle);
	} else if (flags[baseNeighbor]) {
		Split(baseNeighbor);
		Split(triangle);
	} else {
		int baseNeighborParent = Parent(baseNeighbor);
		ForceSplit(baseNeighborParent);
		Split(baseNeighbor);
		Split(triangle);
	}
}

void RTIN::Split(int triangle) {
	flags[triangle] = 0;
	int left = Child(LEFT, triangle);
	int right = Child(RIGHT, triangle);
	flags[left] = 1;
	flags[right] = 1;
}

void RTIN::Merge(int triangle) {
	int T_B = Neighbor(B, triangle);
	flags[triangle] = 1;
	flags[T_B] = 1;
	int left = Child(LEFT, triangle);
	int right = Child(RIGHT, triangle);
	flags[left] = 0;
	flags[right] = 0;
	left = Child(LEFT, T_B);
	right = Child(RIGHT, T_B);
	flags[left] = 0;
	flags[right] = 0;
}

void RTIN::Triangulate(const char * filename, int levels) {
	size = (2 << levels) - 1;
	flags = new int[size];
	e_T = new float[size];
	normalBuffer = new vec3[size - 1];
	vertexBuffer = new vec4[4+(size-3)];
	indexBuffer = new GLuint[3 * (size - 1)];
	HeightMap z(filename, 0.5);
	
	vertexBuffer[0] = vec4(-1.0, 1.0, 0.0, 1.0);
	vertexBuffer[1] = vec4(1.0, 1.0, 0.0, 1.0);
	vertexBuffer[2] = vec4(-1.0, -1.0, 0.0, 1.0);
	vertexBuffer[3] = vec4(1.0, -1.0, 0.0, 1.0);

	vertexBuffer[0].z = z(vertexBuffer[0].x, vertexBuffer[0].y);
	vertexBuffer[1].z = z(vertexBuffer[1].x, vertexBuffer[1].y);
	vertexBuffer[2].z = z(vertexBuffer[2].x, vertexBuffer[2].y);
	vertexBuffer[3].z = z(vertexBuffer[3].x, vertexBuffer[3].y);

	indexBuffer[0] = 3;
	indexBuffer[1] = 0;
	indexBuffer[2] = 2;

	//cout << indexBuffer[0] << " " << indexBuffer[1] << " " << indexBuffer[2] << endl;

	indexBuffer[3] = 0;
	indexBuffer[4] = 3;
	indexBuffer[5] = 1;

	//cout << indexBuffer[3] << " " << indexBuffer[4] << " " << indexBuffer[5] << endl;

	int v = 4;
	for (int t = 1; t < size; t++) {
		if (Child(LEFT, t) == -1) break;
		int i = 3 * (t - 1);
		vertexBuffer[v] = vertexBuffer[indexBuffer[i]] + vertexBuffer[indexBuffer[i + 1]];
		vertexBuffer[v] = vertexBuffer[v] / 2;
		vertexBuffer[v].z = z(vertexBuffer[v].x, vertexBuffer[v].y);
		
		int left = Child(LEFT, t);
		int right = Child(RIGHT, t);

		int j = 3 * (left - 1);

		indexBuffer[j] = indexBuffer[i + 2];
		indexBuffer[j + 1] = indexBuffer[i];
		indexBuffer[j + 2] = v;

		//cout << indexBuffer[j] << " " << indexBuffer[j+1] << " " << indexBuffer[j+2] << endl;

		j = 3 * (right - 1);

		indexBuffer[j] = indexBuffer[i + 1];
		indexBuffer[j + 1] = indexBuffer[i + 2];
		indexBuffer[j + 2] = v; 
		//cout << indexBuffer[j] << " " << indexBuffer[j+1] << " " << indexBuffer[j+2] << endl;
		v++;
	}

	for (int i = 0; i < size - 1; i++) {
		vec4 a = vertexBuffer[indexBuffer[3 * i]];
		vec4 b = vertexBuffer[indexBuffer[3 * i + 1]];
		vec4 c = vertexBuffer[indexBuffer[3 * i + 2]];
		normalBuffer[i] = normalize( cross( (b - a), (c - a) ) );
	}
}

void RTIN::DrawTriangle(int triangle) {
	//if (triangle >= 0) cout << "Draw triangle entered for " << triangle << endl;
	//if (triangle >= 0) cout << "flags[" << triangle << "]: " << flags[triangle] << endl;
	if (flags[triangle] == 1) {
		int i = 3 * (triangle - 1);
		int index;
		vec4 vect;
		vec3 norm;
		glBegin(GL_TRIANGLES);
			index = indexBuffer[i];
			vect = vertexBuffer[index]; i++;
			norm = normalBuffer[triangle - 1];
			glNormal3f(norm.x, norm.y, norm.z);
			glVertex4f(vect.x, vect.y, vect.z, 1.0);
			index = indexBuffer[i];
			vect = vertexBuffer[index]; i++;
			norm = normalBuffer[triangle - 1];
			glNormal3f(norm.x, norm.y, norm.z);
			glVertex4f(vect.x, vect.y, vect.z, 1.0);
			index = indexBuffer[i];
			vect = vertexBuffer[index];
			norm = normalBuffer[triangle - 1];
			glNormal3f(norm.x, norm.y, norm.z);
			glVertex4f(vect.x, vect.y, vect.z, 1.0);
		glEnd();
	} else {
		int left = Child(LEFT, triangle);
		int right = Child(RIGHT, triangle);
		if (left >= 0 && right >= 0) {
			//cout << "Consider children: " << left << " & " << right << endl;
			DrawTriangle(left);
			DrawTriangle(right);
		}
	}
}