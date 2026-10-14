#include <windows.h>
#include <string.h>

const wchar_t* lpszPipeName = L"\\\\.\\pipe\\myPipe";

HWND hEdit;

void Send(HWND hWnd)
{
    wchar_t szBuf[512] = {};
    GetWindowTextW(hEdit, szBuf, 512);

    HANDLE hNamedPipe;
    while (true) {
        hNamedPipe = CreateFileW(lpszPipeName, GENERIC_READ | GENERIC_WRITE,
                                 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hNamedPipe != INVALID_HANDLE_VALUE)
            break;
        if (GetLastError() != ERROR_PIPE_BUSY || !WaitNamedPipeW(lpszPipeName, 5000)) {
            MessageBoxW(hWnd, L"Не можливо відкрити канал. Запустіть сервер (Pipe -> Start)", L"Pipe", MB_OK | MB_ICONERROR);
            return;
        }
    }

    DWORD dwMode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hNamedPipe, &dwMode, NULL, NULL);

    DWORD cbWritten, cbRead;
    if (!WriteFile(hNamedPipe, szBuf, (DWORD)((wcslen(szBuf) + 1) * sizeof(wchar_t)), &cbWritten, NULL)) {
        MessageBoxW(hWnd, L"Помилка запису в канал", L"Pipe", MB_OK | MB_ICONERROR);
        CloseHandle(hNamedPipe);
        return;
    }

    wchar_t szReply[128] = {};
    if (ReadFile(hNamedPipe, szReply, sizeof(szReply) - sizeof(wchar_t), &cbRead, NULL))
        MessageBoxW(hWnd, szReply, L"Відповідь сервера", MB_OK);

    CloseHandle(hNamedPipe);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        HDC hdc = GetDC(hWnd);
        wchar_t text[256];
        wsprintfW(text,
            L"Дані від Клієнта №1:\r\n"
            L"Кількість процесорних ядер (SM_DIGITIZER) = %d\r\n"
            L"Стан мережі (SM_NETWORK) = %d\r\n"
            L"Ширина екрану (HORZRES) = %d",
            GetSystemMetrics(SM_DIGITIZER),
            GetSystemMetrics(SM_NETWORK),
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
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSub, "Pipe");

    CreateWindowW(L"Client1App", L"Клієнт №1", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                  580, 50, 450, 200, NULL, hMenu, hInst, NULL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
