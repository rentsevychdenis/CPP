#include <windows.h>
#include <stdio.h>

#define ID_THREADS_START 1001
#define ID_THREADS_STOP  1002

struct GlobalTime {
    WORD hour;
    WORD minute;
    WORD second;
};

GlobalTime g_time;
CRITICAL_SECTION g_cs;
HWND g_hWnd;
HANDLE hThreads[2] = { NULL, NULL };
bool g_stopThreads = false;
char szTimeStr[50] = "Press Threads -> Start";

DWORD WINAPI WriterThread(LPVOID lpParam) {
    while (!g_stopThreads) {
        SYSTEMTIME st;
        GetLocalTime(&st);

        EnterCriticalSection(&g_cs);
        g_time.hour = st.wHour;
        g_time.minute = st.wMinute;
        g_time.second = st.wSecond;
        LeaveCriticalSection(&g_cs);

        Sleep(500);
    }
    return 0;
}

DWORD WINAPI ReaderThread(LPVOID lpParam) {
    while (!g_stopThreads) {
        EnterCriticalSection(&g_cs);
        sprintf_s(szTimeStr, "%02d:%02d:%02d", g_time.hour, g_time.minute, g_time.second);
        LeaveCriticalSection(&g_cs);

        InvalidateRect(g_hWnd, NULL, TRUE);
        Sleep(1000);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HMENU hMenu = CreateMenu();
        HMENU hSubMenu = CreatePopupMenu();
        AppendMenuA(hSubMenu, MF_STRING, ID_THREADS_START, "Start");
        AppendMenuA(hSubMenu, MF_STRING, ID_THREADS_STOP, "Stop");
        AppendMenuA(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "Threads");
        SetMenu(hWnd, hMenu);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_THREADS_START:
            if (hThreads[0] == NULL && hThreads[1] == NULL) {
                g_stopThreads = false;
                hThreads[0] = CreateThread(NULL, 0, WriterThread, NULL, 0, NULL);
                hThreads[1] = CreateThread(NULL, 0, ReaderThread, NULL, 0, NULL);
            }
            break;
        case ID_THREADS_STOP:
            g_stopThreads = true;
            if (hThreads[0]) {
                WaitForSingleObject(hThreads[0], INFINITE);
                CloseHandle(hThreads[0]);
                hThreads[0] = NULL;
            }
            if (hThreads[1]) {
                WaitForSingleObject(hThreads[1], INFINITE);
                CloseHandle(hThreads[1]);
                hThreads[1] = NULL;
            }
            break;
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rt;
        GetClientRect(hWnd, &rt);
        DrawTextA(hdc, szTimeStr, -1, &rt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        g_stopThreads = true;
        if (hThreads[0]) {
            WaitForSingleObject(hThreads[0], INFINITE);
            CloseHandle(hThreads[0]);
        }
        if (hThreads[1]) {
            WaitForSingleObject(hThreads[1], INFINITE);
            CloseHandle(hThreads[1]);
        }
        DeleteCriticalSection(&g_cs);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitializeCriticalSection(&g_cs);

    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance,
                       NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1),
                       NULL, "Lab3WinClass", NULL };
    RegisterClassExA(&wc);

    g_hWnd = CreateWindowA("Lab3WinClass", "Lab 3 - Variant 2",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        500, 300, NULL, NULL, hInstance, NULL);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}