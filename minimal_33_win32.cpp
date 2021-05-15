#include <Windows.h>

//opengl attributes and extensions
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
typedef HGLRC(WINAPI *WGL_CREATE_CONTEXT_ATTRIBS_ARB_PROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL(WINAPI *WGL_CHOOSE_PIXEL_FORMAT_ARB_PROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
static WGL_CREATE_CONTEXT_ATTRIBS_ARB_PROC wglCreateContextAttribsARB;
static WGL_CHOOSE_PIXEL_FORMAT_ARB_PROC wglChoosePixelFormatARB;

LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
bool getExtensionFunctions();
bool InitGlContext(HWND wnd);
HWND CreateWin32Window(HINSTANCE instance, const char *window_name);

static bool window_open = true;

bool getExtensionFunctions()
{
    WNDCLASSA wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = "extension loader";
    if (!RegisterClassA(&wc))
        return false;

    HWND wnd = CreateWindowA(
        wc.lpszClassName,
        "",
        0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0,
        0,
        wc.hInstance,
        0);

    if (!wnd)
        return false;

    HDC dc = GetDC(wnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    int pfd_idx = ChoosePixelFormat(dc, &pfd);
    if (!pfd_idx)
        return false;

    if (!SetPixelFormat(dc, pfd_idx, &pfd))
        return false;

    HGLRC glrc = wglCreateContext(dc);
    if (!glrc)
        return false;

    if (!wglMakeCurrent(dc, glrc))
        return false;

    wglCreateContextAttribsARB = (WGL_CREATE_CONTEXT_ATTRIBS_ARB_PROC)wglGetProcAddress("wglCreateContextAttribsARB");
    wglChoosePixelFormatARB = (WGL_CHOOSE_PIXEL_FORMAT_ARB_PROC)wglGetProcAddress("wglChoosePixelFormatARB");
    if (!wglCreateContextAttribsARB || !wglChoosePixelFormatARB)
        return false;

    //clean u
    wglMakeCurrent(0, 0);
    wglDeleteContext(glrc);
    ReleaseDC(wnd, dc);
    DestroyWindow(wnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);

    return true;
}

bool InitGlContext(HWND wnd)
{
    //TODO(lucas): provide some information on the error
    //we must do the window nonsense
    //check if extension functions exist
    if (!wglCreateContextAttribsARB || !wglChoosePixelFormatARB)
    {
        if (!getExtensionFunctions())
            return false;
    }

    //any failure return false
    //set wnd to a 3.3 open gl context
    HDC dc = GetDC(wnd);
    int pixel_attributes[] = {
        WGL_DOUBLE_BUFFER_ARB, 1,
        WGL_SUPPORT_OPENGL_ARB, 1,
        WGL_DRAW_TO_WINDOW_ARB, 1,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        0};

    int pfd_idx;
    unsigned int n_pfd;
    if (!wglChoosePixelFormatARB(dc, pixel_attributes, 0, 1, &pfd_idx, &n_pfd) || !n_pfd)
        return false;

    PIXELFORMATDESCRIPTOR pfd;
    if(!DescribePixelFormat(dc, pfd_idx, sizeof(pfd), &pfd))
        return false;
    if (!SetPixelFormat(dc, pfd_idx, &pfd))
        return false;

    int context_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0};

    HGLRC glrc = wglCreateContextAttribsARB(dc, 0, context_attributes);
    if (!glrc)
        return false;

    if (!wglMakeCurrent(dc, glrc))
        return false;

    //glad binding
    // bindGlContextFunctions();

    ReleaseDC(wnd, dc);

    return true;
}

LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    LRESULT result;
    switch (msg)
    {
    case WM_CREATE:
    {
        //spawn open gl
        if (InitGlContext(wnd))
        {
            return 0;
        }
        else
        {
            MessageBoxA(wnd, "Could not intialise open gl 3.3", 0, MB_OK);
            return -1;
        }
    }
    break;
    case WM_DESTROY:
    {
        window_open = false;
        result = 0;
    }
    break;
    default:
    {
        result = DefWindowProcA(wnd, msg, w_param, l_param);
    }
    break;
    }

    return result;
}

HWND CreateWin32Window(HINSTANCE instance, const char *window_name)
{
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "class";
    if (!RegisterClassA(&wc))
        return 0;

    HWND wnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        window_name,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0,
        0,
        instance,
        0);

    return wnd;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    HWND window = CreateWin32Window(hInstance, "Ping Pong");
    if (!window)
    {
        goto error;
    }

    //at this point we have a windows window that is in an open gl 3.3 context
    ShowWindow(window, nCmdShow);

    while (window_open)
    {
        MSG msg;
        while (PeekMessage(&msg, window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
error:
    return 1;
}