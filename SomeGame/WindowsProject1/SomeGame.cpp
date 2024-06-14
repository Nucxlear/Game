#define _USE_MATH_DEFINES
#pragma comment(lib, "opengl32.lib")
#include <windows.h>
#include <gl/GL.h>
#include <cmath>
#include <mmsystem.h>


LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);
HWND hwnd;


float kube[] = { 0,0,0, 0,1,0, 1,1,0, 1,0,0, 0,0,1, 1,1,1, 1,0,1 };
GLuint kubeInd[] = { 0,1,2, 2,3,0, 4,5,6, 6,7,4, 3,2,5, 6,7,3, 0,1,5, 5,4,0,
1,2,6, 6,5,1, 0,3,7, 7,4,0 };
BOOL showMask = FALSE;
bool DrawCrosshairFlag = true;

typedef struct {
	float r, g, b;
}TColor;

typedef struct {
	TColor clr;
	float height;
} TCell;



#define pW 250
#define pH 250
TCell map[pW][pH];
TColor mapCol[pW][pH];

GLuint mapInd[pW - 1][pH - 1][6];
int mapIndCnt = sizeof(mapInd) / sizeof(GLuint);

struct Player
{
	float x, y, z;
	float Xrot, Zrot;
	BOOL isJumping;
};
Player player;

void Player_Init()
{
	player.x = 0;
	player.y = 0;
	player.z = -2;
	player.Xrot = 70;
	player.Zrot = -40;
	player.isJumping = FALSE;
}

void Map_Init()
{
	for (int i = 0; i < pW; i++)
		for (int j = 0; j < pH; j++)
		{
			float dc = (rand() % 20) * 0.01;
			map[i][j].clr.r = 0.31 + dc;
			map[i][j].clr.g = 0.5 + dc;
			map[i][j].clr.b = 0.13 + dc;
			map[i][j].height = (rand() % 150) / 500.0f;
		}
}

void Map_Show()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, map);
	glColorPointer(3, GL_FLOAT, 0, mapCol);
	glDrawElements(GL_TRIANGLES, mapIndCnt, GL_UNSIGNED_INT, mapInd);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}



#define enemyCnt 250
struct {
	float x, y, z;
	BOOL active;
	unsigned char r, g, b;
} enemy[enemyCnt];

void Enemy_Init()
{
	for (int i = 0; i < enemyCnt; i++)
	{
		enemy[i].active = TRUE;
		enemy[i].x = rand() % pW;
		enemy[i].y = rand() % pH;
		enemy[i].z = rand() % 5;
		enemy[i].r = rand() % 256;
		enemy[i].g = rand() % 256;
		enemy[i].b = rand() % 256;
	}
}

void Enemy_Show()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, kube);
	for (int i = 0; i < enemyCnt; i++)
	{
		if (!enemy[i].active) continue;
		glPushMatrix();
		glTranslatef(enemy[i].x, enemy[i].y, enemy[i].z);
		if (showMask)
		{
			glColor3ub(255 - i, 0, 0);
		}
		else glColor3ub(244, 60, 43);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, kubeInd);
		glPopMatrix();
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}

struct Camera {
	float x, y, z;
	float Xrot, Zrot;
	float verticalSpeed;
	float groundLevel;
	bool isJumping;
	float jumpStartHeight;
	float horizontalSpeed;
	float horizontalAngle;
} camera = { 0, 0, 1.7, 0, 70, 0, -40, false, 1.7, 0, 0 };

void Camera_Apply()
{
	glRotatef(-camera.Xrot, 1, 0, 0);
	glRotatef(-camera.Zrot, 0, 0, 1);
	glTranslatef(-camera.x, -camera.y, -camera.z);
}

float GetHeightAt(float x, float y)
{
	int ix = (int)x;
	int iy = (int)y;

	if (ix < 0 || ix >= pW - 1 || iy < 0 || iy >= pH - 1)
		return 0.0f;

	float fx = x - ix;
	float fy = y - iy;

	float h00 = map[ix][iy].height;
	float h10 = map[ix + 1][iy].height;
	float h01 = map[ix][iy + 1].height;
	float h11 = map[ix + 1][iy + 1].height;

	float h0 = h00 + fx * (h10 - h00);
	float h1 = h01 + fx * (h11 - h01);

	return h0 + fy * (h1 - h0);
}

void Camera_Rotation(float xAngle, float zAngle)
{
	camera.Zrot += zAngle;
	if (camera.Zrot < 0) camera.Zrot += 360;
	if (camera.Zrot > 360) camera.Zrot -= 360;
	camera.Xrot += xAngle;
	if (camera.Xrot < 0) camera.Xrot = 0;
	if (camera.Xrot > 180) camera.Xrot = 180;
}



void Player_Move()
{
	if (GetForegroundWindow() != hwnd) return;

	float ugol = -camera.Zrot / 180 * M_PI;
	float speed = 0;
	float acceleration = 1.0; // Ускорение

	if (GetKeyState('Q') < 0) {
		acceleration = 2.0; // Увеличиваем скорость в два раза
	}
	else {
		acceleration = 1.0; // Возвращаем стандартную скорость
	}

	if (GetKeyState('W') < 0) speed = 0.2 * acceleration;

	if (GetKeyState('S') < 0) speed = -0.2 * acceleration;
	if (GetKeyState('A') < 0) { speed = 0.2 * acceleration; ugol -= M_PI * 0.5; }
	if (GetKeyState('D') < 0) { speed = 0.2 * acceleration; ugol += M_PI * 0.5; }

	// Ускорение при нажатии на "Q"
	

	float newX = camera.x + sin(ugol) * speed;
	float newY = camera.y + cos(ugol) * speed;
	float newHeight = GetHeightAt(newX, newY) + 1.7;

	if (newX >= 0 && newX < pW && newY >= 0 && newY < pH && speed != 0 && !camera.isJumping)
	{
		camera.x = newX;
		camera.y = newY;
		camera.z = newHeight;
	}

	if (camera.x < 0) camera.x = 0;
	if (camera.x >= pW) camera.x = pW - 1;
	if (camera.y < 0) camera.y = 0;
	if (camera.y >= pH) camera.y = pH - 1;

	if (GetKeyState(VK_SPACE) < 0 && !camera.isJumping)
	{
		camera.isJumping = true;
		camera.verticalSpeed = 0.3;
		camera.jumpStartHeight = camera.z;
	}

	if (camera.isJumping)
	{
		camera.z += camera.verticalSpeed;
		camera.verticalSpeed -= 0.01;

		float groundHeight = GetHeightAt(camera.x, camera.y) + 1.7;

		if (camera.z <= groundHeight)
		{
			camera.z = groundHeight;
			camera.isJumping = false;
			camera.verticalSpeed = 0;
		}
	}

	if (camera.isJumping)
	{
		float newX = camera.x + sin(ugol) * speed;
		float newY = camera.y + cos(ugol) * speed;
		camera.x = newX;
		camera.y = newY;
	}

	else
	{
		float groundHeight = GetHeightAt(camera.x, camera.y) + 1.7;
		if (camera.z < groundHeight)
		{
			camera.z = groundHeight;
		}
	}

	POINT cur;
	static POINT base = { 400, 300 };
	GetCursorPos(&cur);
	Camera_Rotation((base.y - cur.y) / 5.0, (base.x - cur.x) / 5.0);
	SetCursorPos(base.x, base.y);
}

void Game_Move()
{
	Player_Move();
}

void WndResize(int x, int y)
{
	glViewport(0, 0, x, y);
	float k = x / (float)y;
	float sz = 0.1;
	glLoadIdentity();
	glFrustum(-k * sz, k * sz, -sz, sz, sz * 2, 100);
}

void Game_Init()
{
	glEnable(GL_DEPTH_TEST);
	Map_Init();
	Enemy_Init();

	RECT rct;
	GetClientRect(hwnd, &rct);
	WndResize(rct.right, rct.bottom);
}

void Game_Show()
{
	if (showMask) glClearColor(0, 0, 0, 0);
	else glClearColor(0.6f, 0.8f, 1.0f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	Camera_Apply();
	

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, kube);

	for (int i = 0; i < pW - 1; i++) {
		for (int j = 0; j < pH - 1; j++) {
			glBegin(GL_TRIANGLES);

			glColor3f(map[i][j].clr.r, map[i][j].clr.g, map[i][j].clr.b);
			glVertex3f(i, j, map[i][j].height);
			glColor3f(map[i + 1][j].clr.r, map[i + 1][j].clr.g, map[i + 1][j].clr.b);
			glVertex3f(i + 1, j, map[i + 1][j].height);
			glColor3f(map[i][j + 1].clr.r, map[i][j + 1].clr.g, map[i][j + 1].clr.b);
			glVertex3f(i, j + 1, map[i][j + 1].height);

			glColor3f(map[i + 1][j].clr.r, map[i + 1][j].clr.g, map[i + 1][j].clr.b);
			glVertex3f(i + 1, j, map[i + 1][j].height);
			glColor3f(map[i + 1][j + 1].clr.r, map[i + 1][j + 1].clr.g, map[i + 1][j + 1].clr.b);
			glVertex3f(i + 1, j + 1, map[i + 1][j + 1].height);
			glColor3f(map[i][j + 1].clr.r, map[i][j + 1].clr.g, map[i][j + 1].clr.b);
			glVertex3f(i, j + 1, map[i][j + 1].height);

			glEnd();
		}
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	Enemy_Show();
	
	glPopMatrix();
	
}

void Player_Shoot()
{
	showMask = TRUE;
	DrawCrosshairFlag = false;
	Game_Show();
	showMask = FALSE;
	DrawCrosshairFlag = false;

	RECT rct;
	GLubyte clr[3];
	GetClientRect(hwnd, &rct);
	glReadPixels(rct.right / 2.0, rct.bottom / 2.0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, clr);
	if (clr[0] > 0) enemy[255 - clr[0]].active = FALSE;
}


int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	WNDCLASSEX wcex;
	HDC hDC;
	HGLRC hRC;
	MSG msg;
	BOOL bQuit = FALSE;
	float theta = 0.0f;

	/* register window class */
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_OWNDC;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"GLSample";
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


	if (!RegisterClassEx(&wcex))
		return 0;

	// create main window
	hwnd = CreateWindowEx(0,
		L"GLSample",
		L"OpenGL Sample",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		800,
		600,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hwnd, nCmdShow);

	/* enable OpenGL for the window */
	EnableOpenGL(hwnd, &hDC, &hRC);



	Player_Init();
	Game_Init();
	Map_Init();
	/* program main loop */
	while (!bQuit)
	{
		/* check for messages */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			/* handle or dispatch messages */
			if (msg.message == WM_QUIT)
			{
				bQuit = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			/* OpenGL animation code goes here */
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glPushMatrix();
			Map_Show();

			Game_Move();
			Game_Show();
			glPopMatrix();

			SwapBuffers(hDC);

			if (player.z <= 0) player.isJumping = FALSE;
			theta += 1.0f;
			Sleep(1);
		}
	}

	/* shutdown OpenGL */
	DisableOpenGL(hwnd, hDC, hRC);

	/* destroy the window explicitly */
	DestroyWindow(hwnd);

	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		WndResize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_DESTROY:
		return 0;

	case WM_SETCURSOR:
		ShowCursor(FALSE);
		break;

	case WM_LBUTTONDOWN:
		Player_Shoot();
		break;

	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
	}
	break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
	PIXELFORMATDESCRIPTOR pfd;

	int iFormat;

	/* get the
	device context (DC) */
	*hDC = GetDC(hwnd);

	/* set the pixel format for the DC */
	ZeroMemory(&pfd, sizeof(pfd));

	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;

	iFormat = ChoosePixelFormat(*hDC, &pfd);

	SetPixelFormat(*hDC, iFormat, &pfd);

	/* create and enable the render context (RC) */
	*hRC = wglCreateContext(*hDC);

	wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL(HWND hwnd, HDC hDC, HGLRC hRC)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hwnd, hDC);
}