#include <windows.h>
#include <stdlib.h>
#include <string.h>

#define WM_PIPE_DATA (WM_APP + 1)

const wchar_t* lpszPipeName = L"\\\\.\\pipe\\myPipe";

HWND hMainWnd, hEdit;
HANDLE hNamedPipe = INVALID_HANDLE_VALUE;
HANDLE hThread;
volatile bool bRunning;

void Append(const wchar_t* text)
{
    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, len, len);
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)text);
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
}

DWORD WINAPI PipeThread(LPVOID)
{
    wchar_t szBuf[512];
    DWORD cbRead, cbWritten;
    const wchar_t* szReply = L"Сервер отримав дані";

    while (bRunning) {
        BOOL fConnected = ConnectNamedPipe(hNamedPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!fConnected) {
            if (!bRunning)
                break;
            DWORD dwError = GetLastError();
            DisconnectNamedPipe(hNamedPipe);
            if (dwError == ERROR_NO_DATA)
                continue;
            wchar_t text[64];
            wsprintfW(text, L"ConnectNamedPipe: Error %lu", dwError);
            PostMessageW(hMainWnd, WM_PIPE_DATA, 0, (LPARAM)_wcsdup(text));
            break;
        }

        if (ReadFile(hNamedPipe, szBuf, sizeof(szBuf) - sizeof(wchar_t), &cbRead, NULL)) {
            szBuf[cbRead / sizeof(wchar_t)] = L'\0';
            PostMessageW(hMainWnd, WM_PIPE_DATA, 0, (LPARAM)_wcsdup(szBuf));
            WriteFile(hNamedPipe, szReply, (DWORD)((wcslen(szReply) + 1) * sizeof(wchar_t)), &cbWritten, NULL);
            FlushFileBuffers(hNamedPipe);
        }
        else if (bRunning) {
            wchar_t text[64];
            wsprintfW(text, L"ReadFile: Error %lu", GetLastError());
            PostMessageW(hMainWnd, WM_PIPE_DATA, 0, (LPARAM)_wcsdup(text));
        }
        DisconnectNamedPipe(hNamedPipe);
    }
    return 0;
}

void StartPipe(HWND hWnd)
{
    if (hNamedPipe != INVALID_HANDLE_VALUE) return;

    hNamedPipe = CreateNamedPipeW(lpszPipeName,
                                  PIPE_ACCESS_DUPLEX,
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                  PIPE_UNLIMITED_INSTANCES,
                                  1024, 1024, 5000, NULL);
    if (hNamedPipe == INVALID_HANDLE_VALUE) {
        wchar_t text[64];
        wsprintfW(text, L"CreateNamedPipe: Error %lu", GetLastError());
        MessageBoxW(hWnd, text, L"Pipe", MB_OK | MB_ICONERROR);
        return;
    }
    bRunning = true;
    hThread = CreateThread(NULL, 0, PipeThread, NULL, 0, NULL);
    Append(L"Канал \\\\.\\pipe\\myPipe створено. Очікування клієнтів...");
}

void StopPipe()
{
    if (hNamedPipe == INVALID_HANDLE_VALUE) return;

    bRunning = false;
    while (WaitForSingleObject(hThread, 50) == WAIT_TIMEOUT)
        CancelSynchronousIo(hThread);
    CloseHandle(hThread);
    CloseHandle(hNamedPipe);
    hThread = NULL;
    hNamedPipe = INVALID_HANDLE_VALUE;
    Append(L"Канал закрито");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        hEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                              0, 0, 0, 0, hWnd, NULL, NULL, NULL);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        break;
    case WM_SIZE:
        MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_PIPE_DATA: {
        wchar_t* text = (wchar_t*)lParam;
        if (text) {
            Append(text);
            Append(L"");
            free(text);
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1:
            StartPipe(hWnd);
            break;
        case 2:
            StopPipe();
            break;
        }
        break;
    case WM_DESTROY:
        StopPipe();
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
    wc.lpszClassName = L"ServerApp";
    RegisterClassW(&wc);

    HMENU hMenu = CreateMenu();
    HMENU hSub = CreatePopupMenu();
    AppendMenuA(hSub, MF_STRING, 1, "Start");
    AppendMenuA(hSub, MF_STRING, 2, "Stop");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSub, "Pipe");

    hMainWnd = CreateWindowW(L"ServerApp", L"Сервер", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             50, 50, 500, 450, NULL, hMenu, hInst, NULL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
