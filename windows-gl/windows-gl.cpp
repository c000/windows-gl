#include "stdafx.h"
#include "windows-gl.h"
#include <gl/GL.h>
#include "glext.h"

#define MAX_LOADSTRING 100

// Init OpenGL procs
typedef struct {
    PFNGLCREATEPROGRAMPROC CreateProgram;
    PFNGLATTACHSHADERPROC AttachShader;
    PFNGLLINKPROGRAMPROC LinkProgram;
    PFNGLUSEPROGRAMPROC UseProgram;

    PFNGLCREATESHADERPROC CreateShader;
    PFNGLSHADERSOURCEPROC ShaderSource;
    PFNGLCOMPILESHADERPROC CompileShader;
    PFNGLGETSHADERIVPROC GetShaderiv;
    PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;

    PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
    PFNGLUNIFORM1FPROC Uniform1f;
} GLProcs;

GLProcs gGLProcs;

BOOL initOpenGLProcs() {
    gGLProcs.CreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    gGLProcs.AttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    gGLProcs.LinkProgram =  (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    gGLProcs.UseProgram =  (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");

    gGLProcs.CreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    gGLProcs.ShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    gGLProcs.CompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    gGLProcs.GetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    gGLProcs.GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");

    gGLProcs.GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    gGLProcs.Uniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    return TRUE;
}

// Shaders
GLchar vShaderSrc[] =
"attribute vec3 position;\n"
"varying vec2 vCalcCoord;\n"
"void main(void) {\n"
"    vCalcCoord = position.xy * 1.;\n"
"    gl_Position = vec4(position, 1.);\n"
"}\n";


GLchar fShaderSrc[] =
"precision mediump float;\n"
"varying vec2 vCalcCoord;\n"
"uniform float time;\n"
"const int loops = 0x10;\n"
"vec2 mv2(vec2 v1, vec2 v2) {\n"
"    return vec2(v1.x * v2.x - v1.y * v2.y, v1.x * v2.y + v1.y * v2.x);\n"
"}\n"
"float mandelblot_orbit(vec2 pos, vec2 target) {\n"
"    vec2 acc = pos;\n"
"    int calcs = loops;\n"
"    float dmin = 1.0e10;\n"
"    for (int i = 0; i < loops; ++i) {\n"
"        float re = acc.x;\n"
"        float im = acc.y;\n"
"        float t = radians(mod(time, 47.) / 47. * 360.);\n"
"        float t2 = radians(mod(time, 31.) / 31. * 360.);\n"
"        vec2 constant = vec2(-.4, .6) + vec2(.2 * sin(t2), .1 * sin(t));\n"
"        acc = mv2(acc, mv2(acc, acc)) + constant;\n"

"        float d = distance(target, acc);\n"
"        dmin = min(d, dmin);\n"
"    }\n"
"    return dmin;\n"
"}\n"
"void main(void) {\n"
"    gl_FragColor = vec4(1., .5, .1, 1.) * .1 / mandelblot_orbit(vCalcCoord, vec2(-.24, .27));\n"
"}\n";

// Main
WCHAR gTitle[MAX_LOADSTRING] = _T("MyWindow");
int timeCount = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT mes, WPARAM wParam, LPARAM lParam) {
    if (mes == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, mes, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPWSTR    lpCmdLine,
        _In_ int       nCmdShow)
{
    MSG msg; HWND hWnd;
    WNDCLASSEX wcex = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0,
        0,
        hInstance,
        NULL,
        NULL,
        (HBRUSH)(COLOR_WINDOW + 1),
        NULL,
        gTitle,
        NULL};

    if (!RegisterClassEx(&wcex)) {
        return 0;
    }

    if (!(hWnd = CreateWindow(
                    gTitle,
                    gTitle,
                    WS_SYSMENU,
                    CW_USEDEFAULT,
                    0,
                    512,
                    512,
                    NULL,
                    NULL,
                    hInstance,
                    NULL))) {
        return 0;
    }

    HDC hDC = GetDC(hWnd);
    if (!hDC) {
        return 1;
    }

    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormatIndex = ChoosePixelFormat(hDC, &pfd);
    if (pixelFormatIndex == 0) {
        return 2;
    }

    if (!SetPixelFormat(hDC, pixelFormatIndex, &pfd)) {
        DWORD err = GetLastError();
        return 3;
    }

    // Get context
    HGLRC hGLRC = wglCreateContext(hDC);
    if (!hGLRC) {
        DWORD err = GetLastError();
        return 4;
    }

    if (!wglMakeCurrent(hDC, hGLRC)) {
        DWORD err = GetLastError();
        return 5;
    }
    if (!initOpenGLProcs()) {
        return 6;
    }

    // Create Shaders
    GLuint vShader = gGLProcs.CreateShader(GL_VERTEX_SHADER);
    GLchar *src;
    src = vShaderSrc;
    gGLProcs.ShaderSource(vShader, 1, &src, 0);
    gGLProcs.CompileShader(vShader);
    GLint status;
    gGLProcs.GetShaderiv(vShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        gGLProcs.GetShaderiv(vShader, GL_INFO_LOG_LENGTH, &length);

        GLchar log[1024];
        GLsizei logSize;
        gGLProcs.GetShaderInfoLog(vShader, 1024, &logSize, log);
        return 7;
    }

    GLuint fShader = gGLProcs.CreateShader(GL_FRAGMENT_SHADER);
    src = fShaderSrc;
    gGLProcs.ShaderSource(fShader, 1, &src, 0);
    gGLProcs.CompileShader(fShader);
    gGLProcs.GetShaderiv(fShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        gGLProcs.GetShaderiv(vShader, GL_INFO_LOG_LENGTH, &length);

        GLchar log[1024];
        GLsizei logSize;
        gGLProcs.GetShaderInfoLog(vShader, 1024, &logSize, log);
        return 8;
    }

    GLuint program = gGLProcs.CreateProgram();
    gGLProcs.AttachShader(program, vShader);
    gGLProcs.AttachShader(program, fShader);
    gGLProcs.LinkProgram(program);
    gGLProcs.UseProgram(program);

    ShowWindow(hWnd, nCmdShow);

    do {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            DispatchMessage(&msg);
        }
        else {
            wglMakeCurrent(hDC, hGLRC);
            ++timeCount;
            GLint timeVar = gGLProcs.GetUniformLocation(program, "time");
            gGLProcs.Uniform1f(timeVar, (float)timeCount * 1e-3f);

            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glRectf(-1.f, -1.f, 1.f, 1.f);
            glFlush();

            SwapBuffers(hDC);
            wglMakeCurrent(NULL, NULL);

        }
    } while (msg.message != WM_QUIT);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hGLRC);
    ReleaseDC(hWnd, hDC);

    return 0;
}
