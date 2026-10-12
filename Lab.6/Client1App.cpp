#include <windows.h>

HWND hEdit;

void Send(HWND hWnd)
{
    wchar_t szBuf[256] = {};
    GetWindowTextW(hEdit, szBuf, 256);

    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "myFileMapping");
    if (hMapFile == NULL) {
        MessageBoxW(hWnd, L"Не можливо відкрити відображений у пам'яті об'єкт", L"Map", MB_OK);
        return;
    }
    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 512);
    if (pBuf == NULL) {
        MessageBoxW(hWnd, L"Подання проектованого файлу неможливе", L"Map", MB_OK);
        CloseHandle(hMapFile);
        return;
    }
    CopyMemory(pBuf, szBuf, sizeof(szBuf));
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);

    HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "myFileMappingEvent");
    SetEvent(hEvent);
    CloseHandle(hEvent);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        HDC hdc = GetDC(hWnd);
        wchar_t text[256];
        wsprintfW(text,
            L"Дані від Клієнта №1:\r\n"
            L"Обсяг віртуального екрану (SM_CXVIRTUALSCREEN) = %d\r\n"
            L"Наявність зовнішнього диску (SM_REMOTECONTROL) = %d\r\n"
            L"Ширина екрану (HORZRES) = %d",
            GetSystemMetrics(SM_CXVIRTUALSCREEN),
            GetSystemMetrics(SM_REMOTECONTROL),
            GetDeviceCaps(hdc, HORZRES));
        ReleaseDC(hWnd, hdc);

        hEdit = CreateWindowW(L"EDIT", text, WS_CHILD | WS_VISIBLE | ES_MULTILINE,
                              0, 0, 0, 0, hWnd, NULL, NULL, NULL);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        break;
    }
    case WM_SIZE:
        MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
            Send(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"Client1App";
    RegisterClassW(&wc);

    HMENU hMenu = CreateMenu();
    HMENU hSub = CreatePopupMenu();
    AppendMenuA(hSub, MF_STRING, 1, "Send");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSub, "FileMapping");

    CreateWindowW(L"Client1App", L"Клієнт №1", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                  580, 50, 450, 200, NULL, hMenu, hInst, NULL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
