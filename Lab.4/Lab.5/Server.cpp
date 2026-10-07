#include <windows.h>
#include <stdio.h>

#define IDC_CREATE  101
#define IDC_CLOSE   102
#define IDC_STATUS  103
#define IDC_LOG     104
#define IDT_TIMER   1

const wchar_t* MAILSLOT_NAME = L"\\\\.\\mailslot\\Lab5_Mailslot";
const DWORD MAX_MESSAGE = 2048;

HANDLE hMailslot = INVALID_HANDLE_VALUE;
HWND hStatus, hLog;

HWND AddControl(HWND parent, const wchar_t* type, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id)
{
    HWND hCtrl = CreateWindowW(type, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageW(hCtrl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    return hCtrl;
}

void AddLog(const wchar_t* text)
{
    int len = GetWindowTextLengthW(hLog);
    SendMessageW(hLog, EM_SETSEL, len, len);
    SendMessageW(hLog, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(hLog, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

void LogError(const wchar_t* function)
{
    wchar_t text[100];
    swprintf_s(text, L"Помилка %s, код %lu", function, GetLastError());
    AddLog(text);
}

void CreateMailslotClick(HWND hWnd)
{
    if (hMailslot != INVALID_HANDLE_VALUE) {
        AddLog(L"Поштова скринька вже створена.");
        return;
    }

    hMailslot = CreateMailslotW(MAILSLOT_NAME, MAX_MESSAGE, 0, NULL);
    if (hMailslot == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        wchar_t text[200];
        swprintf_s(text, L"Помилка CreateMailslot, код %lu%s", error,
            error == ERROR_ALREADY_EXISTS ? L": скринька вже існує (запущено інший сервер)" : L"");
        AddLog(text);
        SetWindowTextW(hStatus, L"Статус: помилка створення скриньки");
        return;
    }

    if (!SetTimer(hWnd, IDT_TIMER, 1000, NULL))
        LogError(L"SetTimer");

    wchar_t text[200];
    swprintf_s(text, L"Поштову скриньку %s створено, сервер запущено.", MAILSLOT_NAME);
    AddLog(text);
    SetWindowTextW(hStatus, L"Статус: працює");
}

void CloseMailslotClick(HWND hWnd)
{
    if (hMailslot == INVALID_HANDLE_VALUE) {
        AddLog(L"Поштова скринька не відкрита.");
        return;
    }

    KillTimer(hWnd, IDT_TIMER);
    CloseHandle(hMailslot);
    hMailslot = INVALID_HANDLE_VALUE;
    AddLog(L"Поштову скриньку закрито, сервер зупинено.");
    SetWindowTextW(hStatus, L"Статус: закрито");
}

void ReadMailslot()
{
    DWORD nextSize, count;
    if (!GetMailslotInfo(hMailslot, NULL, &nextSize, &count, NULL)) {
        LogError(L"GetMailslotInfo");
        return;
    }

    for (DWORD i = 0; i < count; i++) {
        wchar_t buffer[MAX_MESSAGE / sizeof(wchar_t) + 1] = {};
        DWORD bytesRead;
        if (!ReadFile(hMailslot, buffer, MAX_MESSAGE, &bytesRead, NULL)) {
            LogError(L"ReadFile");
            return;
        }
        AddLog(L"---------- отримано повідомлення ----------");
        AddLog(buffer);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        AddControl(hWnd, L"BUTTON", L"Створити поштову скриньку", 0, 10, 10, 200, 30, IDC_CREATE);
        AddControl(hWnd, L"BUTTON", L"Закрити поштову скриньку", 0, 220, 10, 200, 30, IDC_CLOSE);
        hStatus = AddControl(hWnd, L"STATIC", L"Статус: скринька не створена", 0, 10, 50, 460, 20, IDC_STATUS);
        hLog = AddControl(hWnd, L"EDIT", L"", WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            10, 75, 460, 340, IDC_LOG);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CREATE) CreateMailslotClick(hWnd);
        if (LOWORD(wParam) == IDC_CLOSE)  CloseMailslotClick(hWnd);
        break;

    case WM_TIMER:
        if (hMailslot != INVALID_HANDLE_VALUE)
            ReadMailslot();
        break;

    case WM_DESTROY:
        if (hMailslot != INVALID_HANDLE_VALUE)
            CloseHandle(hMailslot);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"Lab5Server";
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowW(L"Lab5Server", L"Сервер — поштова скринька (ЛР №5)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 470, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
