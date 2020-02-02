#Lets-Slice-Polygons-2019

#include <iostream>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <vector>

#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GL/glm.hpp"
#include "GL/ext.hpp"
#include "GL/gtc/matrix_transform.hpp"
#include "GL/freeglut_ext.h"

using namespace std;
int gwidth = 650;
int gheight = 650;

bool pendingTimerStop = false;

bool fillMode = true;

GLuint programID;

GLuint VAO[5];
GLuint VBO[5];

float sliceVtx[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float remainVtx[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float collideVtx_tr[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float collideVtx_sq[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

int sliceType[4] = { 0, 0, 0, 0 };
int collideCnt[4] = { 0, 0, 0, 0 };

float den;
float ua;
float ub;

float x[5];
float y[5];

bool collide_1[4] = { false, false, false, false };
bool collide_2[4] = { false, false, false, false };
bool collide_3[4] = { false, false, false, false };
bool collide_4[4] = { false, false, false, false };

bool isSliced[4] = { false, false, false, false };

bool isDragged = false;

bool isRoad = false;

float smallFallValue[4] = { 0.f, 0.f, 0.f, 0.f };
float bigFallValue[4] = { 0.f, 0.f, 0.f, 0.f };

float transX[4] = { -1.2f, 1.2f, -1.2f, 1.2f };
float transY[4] = { 0.125f, 0.725f, -1.075f, -0.475f };

float speedValue = 1.0f;

float lineColor = 1.0f;

int objectColor = 0;

glm::mat4 defaultMatrix = glm::mat4(1.0f);
glm::mat4 transMatrix[4];
glm::mat4 rotMatrix[4];

glm::mat4 smallFallMatrix[4];
glm::mat4 bigFallMatrix[4];

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path) {

	// 쉐이더들 생성
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// 버텍스 쉐이더 코드를 파일에서 읽기
	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_file_path, ios::in);
	if (VertexShaderStream.is_open()) {
		stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		printf("파일 %s 를 읽을 수 없음\n", vertex_file_path);
		return 0;
	}

	// 프래그먼트 쉐이더 코드를 파일에서 읽기
	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_file_path, ios::in);
	if (FragmentShaderStream.is_open()) {
		stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}
	else {
		printf("파일 %s 를 읽을 수 없음\n", fragment_file_path);
		return 0;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// 버텍스 쉐이더를 컴파일
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// 버텍스 쉐이더를 검사
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// 프래그먼트 쉐이더를 컴파일
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// 프래그먼트 쉐이더를 검사
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// 프로그램에 링크
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// 프로그램 검사
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

void convertDeviceXY2OpenglXY(int x, int y, float* ox, float* oy)
{
	int w = gwidth;
	int h = gheight;
	*ox = (float)((x - (float)w / 2.0) * (float)(1.0 / (float)(w / 2.0)));
	*oy = -(float)((y - (float)h / 2.0) * (float)(1.0 / (float)(h / 2.0)));
}

void Timer(int n)
{
	for (int i = 0; i < 4; i++)
	{
		collideCnt[i] = 0;

		if (!isSliced[i])
		{
			// 객체 날아오는 속도와 방향 세팅
			if (i == 0)
			{
				transX[i] += 0.03f * speedValue;
				transY[i] -= 0.015f * speedValue;
			}
			if (i == 1)
			{
				transX[i] -= 0.03f * speedValue;
				transY[i] -= 0.015f * speedValue;
			}
			if (i == 2)
			{
				transX[i] += 0.03f * speedValue;
				transY[i] += 0.015f * speedValue;
			}
			if (i == 3)
			{
				transX[i] -= 0.03f * speedValue;
				transY[i] += 0.015f * speedValue;
			}

			transMatrix[i] = glm::mat4(1.0f);
			transMatrix[i] = glm::translate(transMatrix[i], glm::vec3(transX[i], transY[i], 0.f));

			//ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / den
			//ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / den
			//ua와 ub가 모두 0과 1 사이면 충돌.

			x[1] = sliceVtx[0];
			x[2] = sliceVtx[2];
			y[1] = sliceVtx[1];
			y[2] = sliceVtx[3];

			if (i < 2) // 삼각형의 충돌체크 (if i == 0 or 1)
			{
				// 왼쪽 위의 변 충돌체크 : (0 , 0.35), (-0.2, 0)와 슬라이스 선분
				x[3] = 0.f + transX[i];
				x[4] = -0.2f + transX[i];
				y[3] = 0.35f + transY[i];
				y[4] = 0.f + transY[i];

				den = (y[4] - y[3]) * (x[2] - x[1]) - (x[4] - x[3]) * (y[2] - y[1]);
				ua = ((x[4] - x[3]) * (y[1] - y[3]) - (y[4] - y[3]) * (x[1] - x[3])) / den;
				ub = ((x[2] - x[1]) * (y[1] - y[3]) - (y[2] - y[1]) * (x[1] - x[3])) / den;

				if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 && den != 0.f)
				{
					collideCnt[i]++;
					collide_1[i] = true;

					collideVtx_tr[0] = x[1] + (ua * (x[2] - x[1])) - transX[i];
					collideVtx_tr[1] = y[1] + (ua * (y[2] - y[1])) - transY[i];
				}

				// 오른쪽 위의 변 충돌체크 : (0, 0.35), (0.2, 0)와 슬라이스 선분
				x[3] = 0.f + transX[i];
				x[4] = 0.2f + transX[i];
				y[3] = 0.35f + transY[i];
				y[4] = 0.f + transY[i];

				den = (y[4] - y[3]) * (x[2] - x[1]) - (x[4] - x[3]) * (y[2] - y[1]);
				ua = ((x[4] - x[3]) * (y[1] - y[3]) - (y[4] - y[3]) * (x[1] - x[3])) / den;
				ub = ((x[2] - x[1]) * (y[1] - y[3]) - (y[2] - y[1]) * (x[1] - x[3])) / den;

				if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 && den != 0.f)
				{
					collideCnt[i]++;
					collide_2[i] = true;

					if (collide_1[i])
					{
						collideVtx_tr[2] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_tr[3] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
					else
					{
						collideVtx_tr[0] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_tr[1] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
				}

				// 아래쪽 변 충돌체크 : (-0.2, 0), (0.2, 0)와 슬라이스 선분
				x[3] = -0.2f + transX[i];
				x[4] = 0.2f + transX[i];
				y[3] = 0.f + transY[i];
				y[4] = 0.f + transY[i];

				den = (y[4] - y[3]) * (x[2] - x[1]) - (x[4] - x[3]) * (y[2] - y[1]);
				ua = ((x[4] - x[3]) * (y[1] - y[3]) - (y[4] - y[3]) * (x[1] - x[3])) / den;
				ub = ((x[2] - x[1]) * (y[1] - y[3]) - (y[2] - y[1]) * (x[1] - x[3])) / den;

				if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 && den != 0.f)
				{
					collideCnt[i]++;
					collide_3[i] = true;


					collideVtx_tr[2] = x[1] + (ua * (x[2] - x[1])) - transX[i];
					collideVtx_tr[3] = y[1] + (ua * (y[2] - y[1])) - transY[i];
				}
			}

			else // 사각형의 충돌 체크
			{
				// 왼 변 충돌체크 : (-0.175, 0), (-0.175, 0.35)와 슬라이스 선분
				x[3] = -0.175f + transX[i];
				x[4] = -0.175f + transX[i];
				y[3] = 0.f + transY[i];
				y[4] = 0.35f + transY[i];

				den = (y[4] - y[3]) * (x[2] - x[1]) - (x[4] - x[3]) * (y[2] - y[1]);
				ua = ((x[4] - x[3]) * (y[1] - y[3]) - (y[4] - y[3]) * (x[1] - x[3])) / den;
				ub = ((x[2] - x[1]) * (y[1] - y[3]) - (y[2] - y[1]) * (x[1] - x[3])) / den;

				if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 && den != 0.f)
				{
					collideCnt[i]++;
					collide_1[i] = true;

					collideVtx_sq[0] = x[1] + (ua * (x[2] - x[1])) - transX[i];
					collideVtx_sq[1] = y[1] + (ua * (y[2] - y[1])) - transY[i];
				}

				// 위 변 충돌체크 : (-0.175, 0.35), (0.175, 0.35)와 슬라이스 선분
				x[3] = -0.175f + transX[i];
				x[4] = 0.175f + transX[i];
				y[3] = 0.35f + transY[i];
				y[4] = 0.35f + transY[i];

				den = (y[4] - y[3]) * (x[2] - x[1]) - (x[4] - x[3]) * (y[2] - y[1]);
				ua = ((x[4] - x[3]) * (y[1] - y[3]) - (y[4] - y[3]) * (x[1] - x[3])) / den;
				ub = ((x[2] - x[1]) * (y[1] - y[3]) - (y[2] - y[1]) * (x[1] - x[3])) / den;

				if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 && den != 0.f)
				{
					collideCnt[i]++;
					collide_2[i] = true;

					if (collide_1[i])
					{
						collideVtx_sq[2] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_sq[3] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
					else
					{
						collideVtx_sq[0] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_sq[1] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
				}

				// 오른 변 충돌체크 : (0.175, 0), (0.175, 0.35)와 슬라이스 선분
				x[3] = 0.175f + transX[i];
				x[4] = 0.175f + transX[i];
				y[3] = 0.f + transY[i];
				y[4] = 0.35f + transY[i];

				den = (y[4] - y[3]) * (x[2] - x[1]) - (x[4] - x[3]) * (y[2] - y[1]);
				ua = ((x[4] - x[3]) * (y[1] - y[3]) - (y[4] - y[3]) * (x[1] - x[3])) / den;
				ub = ((x[2] - x[1]) * (y[1] - y[3]) - (y[2] - y[1]) * (x[1] - x[3])) / den;

				if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 && den != 0.f)
				{
					collideCnt[i]++;
					collide_3[i] = true;

					if (collide_1[i] || collide_2[i])
					{
						collideVtx_sq[2] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_sq[3] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
					else
					{
						collideVtx_sq[0] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_sq[1] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
				}

				// 밑 변 충돌체크 : (-0.175, 0), (0.175, 0)와 슬라이스 선분
				x[3] = -0.175f + transX[i];
				x[4] = 0.175f + transX[i];
				y[3] = 0.f + transY[i];
				y[4] = 0.f + transY[i];

				den = (y[4] - y[3]) * (x[2] - x[1]) - (x[4] - x[3]) * (y[2] - y[1]);
				ua = ((x[4] - x[3]) * (y[1] - y[3]) - (y[4] - y[3]) * (x[1] - x[3])) / den;
				ub = ((x[2] - x[1]) * (y[1] - y[3]) - (y[2] - y[1]) * (x[1] - x[3])) / den;

				if (ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1 && den != 0.f)
				{
					collideCnt[i]++;
					collide_4[i] = true;

					if (collide_1[i] || collide_2[i] || collide_3[i])
					{
						collideVtx_sq[2] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_sq[3] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
					else
					{
						collideVtx_sq[0] = x[1] + (ua * (x[2] - x[1])) - transX[i];
						collideVtx_sq[1] = y[1] + (ua * (y[2] - y[1])) - transY[i];
					}
				}
			}
		}

		// 객체의 두 개의 변에 선분이 충돌하였을 경우 슬라이스 된다.
		if (collideCnt[i] >= 2)
		{
			isSliced[i] = true;
		}

		if (isSliced[i])
		{
			smallFallValue[i] -= 0.015f;
			bigFallValue[i] -= 0.01f;
			smallFallMatrix[i] = glm::translate(smallFallMatrix[i], glm::vec3(0.f, smallFallValue[i], 0.f));
			bigFallMatrix[i] = glm::translate(bigFallMatrix[i], glm::vec3(0.f, bigFallValue[i], 0.f));

			if (i == 0 || i == 2)
			{
				rotMatrix[i] = glm::rotate(rotMatrix[i], glm::radians(-3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			else if (i == 1 || i == 3)
			{
				rotMatrix[i] = glm::rotate(rotMatrix[i], glm::radians(3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}

			if (i < 2) // 삼각형일 때 슬라이스 타입
			{
				if (collide_1[i] && collide_2[i]) // 1번 모양 슬라이스
				{
					sliceType[i] = 1;
				}
				if (collide_2[i] && collide_3[i]) // 2번 모양 슬라이스
				{
					sliceType[i] = 2;
				}
				if (collide_1[i] && collide_3[i]) // 3번 모양 슬라이스
				{
					sliceType[i] = 3;
				}
			}
			else // 사각형일 때 슬라이스 타입
			{
				if (collide_1[i] && collide_2[i]) // 1번 모양 슬라이스
				{
					sliceType[i] = 1;
				}
				if (collide_2[i] && collide_3[i]) // 2번 모양 슬라이스
				{
					sliceType[i] = 2;
				}
				if (collide_1[i] && collide_4[i]) // 3번 모양 슬라이스
				{
					sliceType[i] = 3;
				}
				if (collide_3[i] && collide_4[i]) // 4번 모양 슬라이스
				{
					sliceType[i] = 4;
				}
				if (collide_2[i] && collide_4[i]) // 5번 모양 슬라이스
				{
					sliceType[i] = 5;
				}
				if (collide_1[i] && collide_3[i]) // 6번 모양 슬라이스
				{
					sliceType[i] = 6;
				}
				
			}
		}

		// 잘린 조각이 중력으로 일정 거리만큼 떨어졌거나 잘리지 않고 화면 밖으로 나갔다면 
		// 다시 원위치 & 원상태로 이동한 후 화면에 등장한다.
		if (bigFallValue[i] < -0.3f || transX[0] > 1.2f || transX[2] > 1.2f || transX[1] < -1.2 || transX[3] < -1.2f)
		{
			isSliced[i] = false;
			collide_1[i] = false;
			collide_2[i] = false;
			collide_3[i] = false;
			collide_4[i] = false;

			smallFallMatrix[i] = glm::mat4(1.0f);
			bigFallMatrix[i] = glm::mat4(1.0f);
			rotMatrix[i] = glm::mat4(1.0f);
			sliceType[i] = 0;

			smallFallValue[i] = 0.f;
			bigFallValue[i] = 0.f;
			
			if (i == 0)
			{
				transX[i] = -1.2f;
				transY[i] = 0.125f;
			}
			else if (i == 1)
			{
				transX[i] = 1.2f;
				transY[i] = 0.725f;
			}
			else if (i == 2)
			{
				transX[i] = -1.2f;
				transY[i] = -1.075f;
			}
			else if (i == 3)
			{
				transX[i] = 1.2f;
				transY[i] = -0.475f;
			}

			transMatrix[i] = glm::mat4(1.0f);
			transMatrix[i] = glm::translate(transMatrix[i], glm::vec3(transX[i], transY[i], 0.f));
		}
	}

	// 마우스로 그린 선분이 화면에 남아서 계속 슬라이스하지 않도록
	// isDragged가 true라면 false로 바꿔주고 선분을 초기화 시켜준다.
	if (isDragged)
	{
		sliceVtx[0] = 1.0f;
		sliceVtx[1] = 1.0f;
		sliceVtx[2] = 1.0f;
		sliceVtx[3] = 1.0f;

		isDragged = false;
	}

	// 슬라이스 선분의 잔상 remainVtx의 색상을 점점 까맣게 만들어 화면에서 사라지게 한다.
	if (lineColor > 0)
	{
		lineColor -= 0.1;
	}

	glutPostRedisplay(); // 화면 재출력
	if (!pendingTimerStop)
	{
		pendingTimerStop = true;
		glutTimerFunc(50, Timer, 1); // 타이머함수 재 설정
	}
	pendingTimerStop = false;
}

void Mouse(int button, int state, int x, int y)
{
	float ox;
	float oy;

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		convertDeviceXY2OpenglXY(x, y, &ox, &oy);
		cout << "x : " << ox << " y : " << oy << endl;

		sliceVtx[0] = ox;
		sliceVtx[1] = oy;
		sliceVtx[2] = ox;
		sliceVtx[3] = oy;

		remainVtx[0] = ox;
		remainVtx[1] = oy;
		remainVtx[2] = ox;
		remainVtx[3] = oy;
	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		convertDeviceXY2OpenglXY(x, y, &ox, &oy);
		cout << "x : " << ox << " y : " << oy << endl;

		sliceVtx[2] = ox;
		sliceVtx[3] = oy;
		remainVtx[2] = ox;
		remainVtx[3] = oy;

		lineColor = 1.0f;
		isDragged = true;
	}
}

void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '=': // 스피드 업
		if (speedValue < 2.8f)
		{
			speedValue += 0.4f;
		}
		break;

	case '-': // 스피드 다운
		if (speedValue > 0.4f)
		{
			speedValue -= 0.4f;
		}
		break;

	case 'r': // 경로 출력 on/off
		if (isRoad)
		{
			isRoad = false;
		}
		else
		{
			isRoad = true;
		}
		break;

	case 'w': // 와이어모드
		if (fillMode)
		{
			glPolygonMode(GLenum(GL_FRONT_AND_BACK), GLenum(GL_LINE));
			fillMode = false;
		}
		else
		{
			glPolygonMode(GLenum(GL_FRONT_AND_BACK), GLenum(GL_FILL));
			fillMode = true;
		}
		break;

	case 'q': // 프로그램 종료
		exit(0);
		break;
	}

	glutPostRedisplay();
}

void drawTriangle()
{
	float triangleVertexData[] = {
		// 삼각형에서 선분과 충돌한 두 지점을 (collideVtx[0], collideVtx[1]), (collideVtx[2], collideVtx[3])이라고 하자.
		// 삼각형이 잘려지는 모습은 슬라이스 선분이 삼각형의 어떤 두 변과 충돌했냐에 따라 세가지 형태이다.
		// 각 케이스의 좌표값을 따로 지정해주고 어떤 두 변과 충돌했냐를 체크하여
		// 알맞은 잘려진 두 도형을 출력하게 하였다.

		//초기 삼각형
		0.f, 0.35f, 0.f,							0.0f, 0.0f, 0.0f,
		-0.2f, 0.f, 0.f,							0.0f, 0.0f, 0.0f,
		0.2f, 0.f, 0.f,								0.0f, 0.0f, 0.0f,

		//첫 번째 모양으로 잘렸을 때 ( 위 쪽이 작게 잘림 )
		0.f, 0.35f, 0.f,							0.0f, 0.0f, 0.0f,
		collideVtx_tr[0], collideVtx_tr[1], 0.0f,	0.0f, 0.0f, 0.0f,
		collideVtx_tr[2], collideVtx_tr[3], 0.0f,	0.0f, 0.0f, 0.0f,
		-0.2f, 0.f, 0.f,							0.0f, 0.0f, 0.0f,
		0.2f, 0.f, 0.f,								0.0f, 0.0f, 0.0f,

		//두 번째 모양으로 잘렸을 때 ( 오른 쪽 밑이 작게 잘림 )
		0.2f, 0.f, 0.f,								0.0f, 0.0f, 0.0f,
		collideVtx_tr[2], collideVtx_tr[3], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_tr[0], collideVtx_tr[1], 0.f,	0.0f, 0.0f, 0.0f,
		-0.2f, 0.f, 0.f,							0.0f, 0.0f, 0.0f,
		0.f, 0.35f, 0.f,							0.0f, 0.0f, 0.0f,

		//세 번째 모양으로 잘렸을 때 ( 왼 쪽 밑이 작게 잘림 )
		-0.2f, 0.f, 0.f,							0.0f, 0.0f, 0.0f,
		collideVtx_tr[2], collideVtx_tr[3], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_tr[0], collideVtx_tr[1], 0.f,	0.0f, 0.0f, 0.0f,
		0.2f, 0.f, 0.f,								0.0f, 0.0f, 0.0f,
		0.f, 0.35f, 0.f,							0.0f, 0.0f, 0.0f,
	};

	glUseProgram(programID);

	unsigned int modelLocation = glGetUniformLocation(programID, "transform");

	for (int i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			objectColor = 0;
		}
		else
		{
			objectColor = 1;
		}

		if (objectColor == 0) // 빨간 색 삼각형 출력
		{
			for (int j = 0; j < 108; j++)
			{
				if (j % 6 == 3)
				{
					triangleVertexData[j] = 1.0f;
				}
				if (j % 6 == 4)
				{
					triangleVertexData[j] = 0.0f;
				}
				if (j % 6 == 5)
				{
					triangleVertexData[j] = 0.0f;
				}
			}
		}
		if (objectColor == 1) // 파란 색 삼각형 출력
		{
			for (int j = 0; j < 108; j++)
			{
				if (j % 6 == 3)
				{
					triangleVertexData[j] = 0.0f;
				}
				if (j % 6 == 4)
				{
					triangleVertexData[j] = 0.0f;
				}
				if (j % 6 == 5)
				{
					triangleVertexData[j] = 1.0f;
				}
			}
		}

		//--- VAO 객체생성및바인딩
		glGenVertexArrays(1, &VAO[i]); glBindVertexArray(VAO[i]);
		//--- vertex data 저장을위한VBO 생성및바인딩.
		glGenBuffers(1, &VBO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		//--- vertex data 데이터입력.
		glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertexData), triangleVertexData, GL_STATIC_DRAW);
		//---위치속성: 속성인덱스0
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		//---색상속성: 속성인덱스1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		if (sliceType[i] == 0)
		{
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(transMatrix[i]));
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		else
		{
			// 작은 조각
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(smallFallMatrix[i] * transMatrix[i] * rotMatrix[i]));

			glDrawArrays(GL_TRIANGLES, 3 + ((sliceType[i] - 1) * 5), 3);


			// 큰 조각
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(bigFallMatrix[i] * transMatrix[i] * rotMatrix[i]));

			glDrawArrays(GL_TRIANGLES, 4 + ((sliceType[i] - 1) * 5), 3);
			glDrawArrays(GL_TRIANGLES, 5 + ((sliceType[i] - 1) * 5), 3);
		}
	}
}

void drawSquare()
{
	float squareVertexData[] = {
		// 사각형에서 선분과 충돌한 두 지점을 (collideVtx_sq[0], collideVtx_sq[1]), (collideVtx_sq[2], collideVtx_sq[3])이라고 하자.
		// 사각형이 잘려지는 모습은 슬라이스 선분이 삼각형의 어떤 두 변과 충돌했냐에 따라 여섯가지 형태이다.
		// 각 케이스의 좌표값을 따로 지정해주고 어떤 두 변과 충돌했냐를 체크하여
		// 알맞은 잘려진 두 도형을 출력하게 하였다.

		//초기 사각형
		-0.175f, 0.35f, 0.f,				0.0f, 0.0f, 0.0f,
		-0.175f, 0.f, 0.f,					0.0f, 0.0f, 0.0f,
		0.175f, 0.35f, 0.f,					0.0f, 0.0f, 0.0f,
		0.175f, 0.0f, 0.f,					0.0f, 0.0f, 0.0f,

		//첫 번째 모양으로 잘렸을 때 ( 왼 쪽 위가 작게 잘림 )
		-0.175f, 0.35f, 0.f,				0.0f, 0.0f, 0.0f,
		collideVtx_sq[0], collideVtx_sq[1], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_sq[2], collideVtx_sq[3], 0.f,	0.0f, 0.0f, 0.0f,
		-0.175f, 0.f, 0.f,					0.0f, 0.0f, 0.0f,
		0.175f, 0.35f, 0.f,					0.0f, 0.0f, 0.0f,
		0.175f, 0.0f, 0.f,					0.0f, 0.0f, 0.0f,

		//두 번째 모양으로 잘렸을 때 ( 오른 쪽 위가 작게 잘림 )
		0.175f, 0.35f, 0.f,					0.0f, 0.0f, 0.0f,
		collideVtx_sq[0], collideVtx_sq[1], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_sq[2], collideVtx_sq[3], 0.f,	0.0f, 0.0f, 0.0f,
		-0.175f, 0.35f, 0.f,				0.0f, 0.0f, 0.0f,
		0.175f, 0.0f, 0.f,					0.0f, 0.0f, 0.0f,
		-0.175f, 0.f, 0.f,					0.0f, 0.0f, 0.0f,

		//세 번째 모양으로 잘렸을 때 ( 왼 쪽 밑이 작게 잘림 )
		-0.175f, 0.f, 0.f,					0.0f, 0.0f, 0.0f,
		collideVtx_sq[0], collideVtx_sq[1], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_sq[2], collideVtx_sq[3], 0.f,	0.0f, 0.0f, 0.0f,
		-0.175f, 0.35f, 0.f,				0.0f, 0.0f, 0.0f,
		0.175f, 0.0f, 0.f,					0.0f, 0.0f, 0.0f,
		0.175f, 0.35f, 0.f,					0.0f, 0.0f, 0.0f,

		//네 번째 모양으로 잘렸을 때 ( 오른 쪽 밑이 작게 잘림 )
		0.175f, 0.0f, 0.f,					0.0f, 0.0f, 0.0f,
		collideVtx_sq[0], collideVtx_sq[1], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_sq[2], collideVtx_sq[3], 0.f,	0.0f, 0.0f, 0.0f,
		0.175f, 0.35f, 0.f,					0.0f, 0.0f, 0.0f,
		-0.175f, 0.f, 0.f,					0.0f, 0.0f, 0.0f,
		-0.175f, 0.35f, 0.f,				0.0f, 0.0f, 0.0f,

		//다섯 번째 모양으로 잘렸을 때 ( 수직으로 잘림 )
		-0.175f, 0.35f, 0.f,				0.0f, 0.0f, 0.0f,
		-0.175f, 0.f, 0.f,					0.0f, 0.0f, 0.0f,
		collideVtx_sq[0], collideVtx_sq[1], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_sq[2], collideVtx_sq[3], 0.f,	0.0f, 0.0f, 0.0f,
		0.175f, 0.35f, 0.f,					0.0f, 0.0f, 0.0f,
		0.175f, 0.0f, 0.f,					0.0f, 0.0f, 0.0f,

		//다섯 번째 모양으로 잘렸을 때 ( 수직으로 잘림 )
		-0.175f, 0.35f, 0.f,				0.0f, 0.0f, 0.0f,
		0.175f, 0.35f, 0.f,					0.0f, 0.0f, 0.0f,
		collideVtx_sq[0], collideVtx_sq[1], 0.f,	0.0f, 0.0f, 0.0f,
		collideVtx_sq[2], collideVtx_sq[3], 0.f,	0.0f, 0.0f, 0.0f,
		-0.175f, 0.f, 0.f,					0.0f, 0.0f, 0.0f,
		0.175f, 0.0f, 0.f,					0.0f, 0.0f, 0.0f,
	};

	glUseProgram(programID);

	unsigned int modelLocation = glGetUniformLocation(programID, "transform");

	for (int i = 2; i < 4; i++)
	{
		if (i == 2)
		{
			objectColor = 0;
		}
		else
		{
			objectColor = 1;
		}

		if (objectColor == 0) // 노란색 사각형 출력
		{
			for (int j = 0; j < 240; j++)
			{
				if (j % 6 == 3)
				{
					squareVertexData[j] = 1.0f;
				}
				if (j % 6 == 4)
				{
					squareVertexData[j] = 1.0f;
				}
				if (j % 6 == 5)
				{
					squareVertexData[j] = 0.0f;
				}
			}
		}
		if (objectColor == 1) // 하늘색 사각형 출력
		{
			for (int j = 0; j < 240; j++)
			{
				if (j % 6 == 3)
				{
					squareVertexData[j] = 0.0f;
				}
				if (j % 6 == 4)
				{
					squareVertexData[j] = 1.0f;
				}
				if (j % 6 == 5)
				{
					squareVertexData[j] = 1.0f;
				}
			}
		}

		//--- VAO 객체생성및바인딩
		glGenVertexArrays(1, &VAO[i]); glBindVertexArray(VAO[i]);
		//--- vertex data 저장을위한VBO 생성및바인딩.
		glGenBuffers(1, &VBO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		//--- vertex data 데이터입력.
		glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexData), squareVertexData, GL_STATIC_DRAW);
		//---위치속성: 속성인덱스0
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		//---색상속성: 속성인덱스1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		if (sliceType[i] == 0)
		{
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(transMatrix[i]));
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glDrawArrays(GL_TRIANGLES, 1, 3);
		}
		else
		{
			// 작은 조각
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(smallFallMatrix[i] * transMatrix[i] * rotMatrix[i]));

			if (sliceType[i] < 5)
			{
				glDrawArrays(GL_TRIANGLES, 4 + ((sliceType[i] - 1) * 6), 3);
			}
			else
			{ 
				glDrawArrays(GL_TRIANGLES, 4 + ((sliceType[i] - 1) * 6), 3);
				glDrawArrays(GL_TRIANGLES, 5 + ((sliceType[i] - 1) * 6), 3);
			}

			// 큰 조각 
			
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(bigFallMatrix[i] * transMatrix[i] * rotMatrix[i]));

			if (sliceType[i] < 5)
			{
				glDrawArrays(GL_TRIANGLES, 5 + ((sliceType[i] - 1) * 6), 3);
				glDrawArrays(GL_TRIANGLES, 6 + ((sliceType[i] - 1) * 6), 3);
				glDrawArrays(GL_TRIANGLES, 7 + ((sliceType[i] - 1) * 6), 3);
			}
			else
			{
				glDrawArrays(GL_TRIANGLES, 6 + ((sliceType[i] - 1) * 6), 3);
				glDrawArrays(GL_TRIANGLES, 7 + ((sliceType[i] - 1) * 6), 3);
			}
		}
	}
}

void drawSlice()
{
	float vertexData[] = {

		sliceVtx[0], sliceVtx[1], 0.f,		1.0f, 1.0f, 1.0f,
		sliceVtx[2], sliceVtx[3], 0.f,		1.0f, 1.0f, 1.0f,

		remainVtx[0], remainVtx[1], 0.f,		lineColor, lineColor, lineColor,
		remainVtx[2], remainVtx[3], 0.f,		lineColor, lineColor, lineColor,

	};

	glUseProgram(programID);

	unsigned int modelLocation = glGetUniformLocation(programID, "transform");

	//--- VAO 객체생성및바인딩
	glGenVertexArrays(1, &VAO[0]); glBindVertexArray(VAO[0]);
	//--- vertex data 저장을위한VBO 생성및바인딩.
	glGenBuffers(1, &VBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	//--- vertex data 데이터입력.
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	//---위치속성: 속성인덱스0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//---색상속성: 속성인덱스1
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(defaultMatrix));
	glDrawArrays(GL_LINES, 0, 2);
	glDrawArrays(GL_LINES, 2, 2);
}

void drawRoad()
{
	float vertexData[] = {

		-1.2f, -0.9f, 0.f,		0.5f, 0.5f, 0.0f,
		1.2, 0.3f, 0.f,			0.5f, 0.5f, 0.0f,

		-1.2f, -0.3f, 0.f,		0.0f, 0.0f, 0.5f,
		1.2, 0.9f, 0.f,			0.0f, 0.0f, 0.5f,

		-1.2f, 0.3f, 0.f,		0.5f, 0.0f, 0.0f,
		1.2, -0.9f, 0.f,		0.5f, 0.0f, 0.0f,

		-1.2f, 0.9f, 0.f,		0.0f, 0.5f, 0.5f,
		1.2, -0.3f, 0.f,		0.0f, 0.5f, 0.5f,

	};

	glUseProgram(programID);

	unsigned int modelLocation = glGetUniformLocation(programID, "transform");

	//--- VAO 객체생성및바인딩
	glGenVertexArrays(1, &VAO[0]); glBindVertexArray(VAO[0]);
	//--- vertex data 저장을위한VBO 생성및바인딩.
	glGenBuffers(1, &VBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	//--- vertex data 데이터입력.
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	//---위치속성: 속성인덱스0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//---색상속성: 속성인덱스1
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(defaultMatrix));

	glDrawArrays(GL_LINES, 0, 2);
	glDrawArrays(GL_LINES, 2, 2);
	glDrawArrays(GL_LINES, 4, 2);
	glDrawArrays(GL_LINES, 6, 2);
}

void Init()
{
	glEnable(GL_DEPTH_TEST);

	programID = LoadShaders("vs.glsl", "fs.glsl"); // shader 파일과 fragment 파일을 프로그램 링크

	glutTimerFunc(10, Timer, 1);
}

GLvoid onDisplay() // 콜백 함수: 출력
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // 채울 배경색을 지정
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //설정된 색으로 배경 칠하기


	drawTriangle();
	drawSquare();

	if (isRoad)
	{
		drawRoad();
	}

	drawSlice();

	glutSwapBuffers();
}

GLvoid onReshape(int w, int h) // 콜백 함수: 다시 그리기
{
	gwidth = w;
	gheight = h;
	glViewport(0, 0, w, h);
}

int main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); // 디스플레이 모드 설정
	glutInitWindowPosition(300, 0); // 윈도우의 위치 지정
	glutInitWindowSize(gwidth, gheight); // 윈도우의 크기 지정
	glutCreateWindow("practice_1"); // 윈도우 생성(윈도우 이름)

	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) // glew 초기화
	{

		cerr << "Unable to initialize GLEW" << endl;
		exit(-1);
	}
	else {
		cout << "GLEW Initialized\n";
	}

	for (int i = 0; i < 4; i++)
	{
		transMatrix[i] = glm::mat4(1.0f);
		rotMatrix[i] = glm::mat4(1.0f);
		smallFallMatrix[i] = glm::mat4(1.0f);
		bigFallMatrix[i] = glm::mat4(1.0f);
	}

	Init();

	glutDisplayFunc(onDisplay); // 출력 함수의 지정
	glutReshapeFunc(onReshape); // 다시 그리기 함수 지정
	glutMouseFunc(Mouse);
	glutKeyboardFunc(Keyboard);

	glutMainLoop(); // 이벤트 처리 시작
}
