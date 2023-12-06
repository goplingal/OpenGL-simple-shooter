#include <windows.h>
#include <gl/gl.h>
#include <math.h>
#include <mmsystem.h>

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

#define W 40
#define H 40
#define EnemyCnt 40

HWND hwnd;

POINT ScrSize;
float ScrCoef;

float Cube[] = {0,0,0, 0,1,0, 1,1,0, 1,0,0, 0,0,1, 0,1,1, 1,1,1, 1,0,1};
GLuint CubeInd[] = {0,1,2, 2,3,0, 4,5,6, 6,7,4, 3,2,5, 6,7,3, 0,1,5, 5,4,0, 1,2,6, 6,5,1, 0,3,7, 7,4,0};

BOOL ShowMask = FALSE;

typedef struct
{
    float r, g, b;
} Color;

typedef struct
{
    Color clr;
} Cell;

Cell map[W][H];

void Map_Init()
{
    for(int i = 0; i < W; i++)
    {
        for (int j = 0; j < H; j++)
        {
            float dc = (rand() % 20) * 0.01;
            map[i][j].clr.r = 0.31 + dc;
            map[i][j].clr.g = 0.5 + dc;
            map[i][j].clr.b = 0.13 + dc;
        }
    }
}

struct
{
    float x,y,z;
    BOOL active;
} Enemy[EnemyCnt];

void Enemy_Init()
{
    for (int i = 0; i < EnemyCnt; i++)
    {
        Enemy[i].active = TRUE;
        Enemy[i].x = rand() % W;
        Enemy[i].y = rand() % H;
        Enemy[i].z = rand() % 5;
    }
}

void Enemy_Show()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, Cube);
    for (int i = 0; i < EnemyCnt; i++)
    {
        if (!Enemy[i].active)
        {
            continue;
        }
        glPushMatrix();
        glTranslatef(Enemy[i].x, Enemy[i].y, Enemy[i].z);
        if (ShowMask)
        {
            glColor3ub(255 - i, 0, 0);
        }
        else
        {
            glColor3ub(244, 60, 43);
        }
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, CubeInd);
        glPopMatrix();
    }
    glDisableClientState(GL_VERTEX_ARRAY);
}

struct
{
    float x, y, z;
    float Xrot, Zrot;
} Camera = {0, 0, 1.7, 70, -40};

void Camera_Apply()
{
    glRotatef(-Camera.Xrot, 1,0,0);
    glRotatef(-Camera.Zrot, 0,0,1);
    glTranslatef(-Camera.x, -Camera.y, -Camera.z);
}

void Camera_Rotation(float xAngle, float zAngle)
{
    Camera.Zrot += zAngle;
    if (Camera.Zrot < 0)
    {
        Camera.Zrot += 360;
    }
    if (Camera.Zrot > 360)
    {
        Camera.Zrot -= 360;
    }
    Camera.Xrot += xAngle;
    if (Camera.Xrot < 0)
    {
        Camera.Xrot = 0;
    }
    if (Camera.Xrot > 180)
    {
        Camera.Xrot = 180;
    }
}

void Player_Move()
{
    if (GetForegroundWindow() != hwnd)
    {
        return;
    }
    float angle = - Camera.Zrot / 180 * M_PI;
    float speed = 0;
    if (GetKeyState('W') < 0)
    {
        speed = 0.1;
    }
    if (GetKeyState('S') < 0)
    {
        speed = -0.1;
    }
    if (GetKeyState('A') < 0)
    {
        speed = 0.1;
        angle -= M_PI * 0.5;
    }
    if (GetKeyState('D') < 0)
    {
        speed = 0.1;
        angle += M_PI * 0.5;
    }
    if (speed != 0)
    {
        Camera.x += sin(angle) * speed;
        Camera.y += cos(angle) * speed;
    }
    POINT cur;
    static POINT base = {400, 300};
    GetCursorPos(&cur);
    Camera_Rotation((base.y - cur.y) / 5.0, (base.x - cur.x) / 5.0);
    SetCursorPos(base.x, base.y);
}

void Wind_Resize(int x, int y);

void Game_Move()
{
    Player_Move();
}

void Game_Init()
{
    glEnable(GL_DEPTH_TEST);
    Map_Init();
    Enemy_Init();
    RECT rct;
    GetClientRect(hwnd, &rct);
    Wind_Resize(rct.right, rct.bottom);
}

void Game_Show()
{
    float sz = 0.1;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ScrCoef*sz, ScrCoef*sz, -sz,sz, sz*2, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (ShowMask)
    {
        glClearColor(0,0,0,0);
    }
    else
    {
        glClearColor(0.6, 0.8, 1, 0);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    Camera_Apply();
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, Cube);
    for (int i = 0; i < W; i++)
    {
        for (int j = 0; j < H; j++)
        {
            glPushMatrix();
            glTranslatef(i, j, 0);
            if (ShowMask)
            {
                glColor3f(0,0,0);
            }
            else
            {
                glColor3f(map[i][j].clr.r, map[i][j].clr.g, map[i][j].clr.b);
            }
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, CubeInd);
            glPopMatrix();
        }
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    Enemy_Show();
    glPopMatrix();
}

void Player_Shoot()
{
    ShowMask = TRUE;
    Game_Show();
    ShowMask = FALSE;
    RECT rct;
    GLubyte clr[3];
    GetClientRect(hwnd, &rct);
    glReadPixels(rct.right / 2.0, rct.bottom / 2.0, 1,1, GL_RGB, GL_UNSIGNED_BYTE, clr);
    if (clr[0] > 0)
    {
        Enemy[255 - clr[0]].active = FALSE;
    }
    PlaySound("woo.wav", NULL, SND_ASYNC);
}

void Wind_Resize(int x, int y)
{
    glViewport(0,0,x,y);
    ScrSize.x = x;
    ScrSize.y = y;
    ScrCoef = x / (float)y;
}

void Cross_Show()
{
    static float cross[] = {0,-1, 0,1, -1,0, 1,0};
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, cross);
    glPushMatrix();
    glColor3f(1,1,1);
    glTranslatef(ScrSize.x * 0.5, ScrSize.y * 0.5, 0);
    glScalef(15, 15, 1);
    glLineWidth(3);
    glDrawArrays(GL_LINES, 0, 4);
    glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Menu_Show()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, ScrSize.x, ScrSize.y, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    Cross_Show();
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
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;
    if (!RegisterClassEx(&wcex))
        return 0;
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "OpenGL Sample",
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
    EnableOpenGL(hwnd, &hDC, &hRC);
    Game_Init();
    while (!bQuit)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
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
            Game_Move();
            Game_Show();
            Menu_Show();
            SwapBuffers(hDC);
            Sleep (1);
        }
    }
    DisableOpenGL(hwnd, hDC, hRC);
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
        Wind_Resize(LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_SETCURSOR:
        ShowCursor(FALSE);
        break;

    case WM_LBUTTONDOWN:
        Player_Shoot();
        break;
    case WM_DESTROY:
        return 0;

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
    *hDC = GetDC(hwnd);
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
    *hRC = wglCreateContext(*hDC);
    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

