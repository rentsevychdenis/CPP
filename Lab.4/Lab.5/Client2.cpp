#include <windows.h>
#include <stdio.h>

#define IDC_DATA     101
#define IDC_REFRESH  102
#define IDC_SEND     103
#define IDC_STATUS   104

const wchar_t* MAILSLOT_NAME = L"\\\\.\\mailslot\\Lab5_Mailslot";

HWND hData, hStatus;

HWND AddControl(HWND parent, const wchar_t* type, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id)
{
    HWND hCtrl = CreateWindowW(type, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageW(hCtrl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    return hCtrl;
}

void ShowError(const wchar_t* function)
{
    DWORD error = GetLastError();
    wchar_t text[300];
    if (error == ERROR_FILE_NOT_FOUND)
        swprintf_s(text, L"Помилка %s, код %lu: поштову скриньку не знайдено (сервер не запущено або скриньку закрито).", function, error);
    else if (error == ERROR_INVALID_PARAMETER)
        swprintf_s(text, L"Помилка %s, код %lu: повідомлення завелике (максимум 2048 байт).", function, error);
    else
        swprintf_s(text, L"Помилка %s, код %lu.", function, error);
    SetWindowTextW(hStatus, text);
}

void RefreshData()
{
    int cyMenuSize = GetSystemMetrics(SM_CYMENUSIZE);

    int cxMenuCheck = GetSystemMetrics(SM_CXMENUCHECK);

    HDC hdc = GetDC(NULL);
    int vertRes = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(NULL, hdc);

    SYSTEMTIME t;
    GetLocalTime(&t);

    wchar_t text[500];
    swprintf_s(text,
        L"[CLIENT 2] %02d.%02d.%04d %02d:%02d:%02d\r\n"
        L"SM_CYMENUSIZE = %d (висота кнопки меню, px)*\r\n"
        L"SM_CXMENUCHECK = %d (ширина позначки пункту меню, px)\r\n"
        L"VERTRES = %d (вертикальний розмір екрану, px)\r\n"
        L"* SM_CYBUTTON з таблиці варіанта у WinAPI не існує",
        t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, cyMenuSize, cxMenuCheck, vertRes);
    SetWindowTextW(hData, text);
    SetWindowTextW(hStatus, L"Дані сформовано.");
}

void SendToServer()
{
    wchar_t text[2000];
    GetWindowTextW(hData, text, 2000);

    HANDLE hMailslot = CreateFileW(MAILSLOT_NAME, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hMailslot == INVALID_HANDLE_VALUE) {
        ShowError(L"CreateFile");
        return;
    }

    DWORD size = (DWORD)(wcslen(text) + 1) * sizeof(wchar_t);
    DWORD written;
    if (WriteFile(hMailslot, text, size, &written, NULL))
        SetWindowTextW(hStatus, L"Повідомлення надіслано на сервер.");
    else
        ShowError(L"WriteFile");

    CloseHandle(hMailslot);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        hData = AddControl(hWnd, L"EDIT", L"", WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, 10, 10, 400, 120, IDC_DATA);
        AddControl(hWnd, L"BUTTON", L"Оновити дані", 0, 10, 140, 195, 30, IDC_REFRESH);
        AddControl(hWnd, L"BUTTON", L"Надіслати на сервер", 0, 215, 140, 195, 30, IDC_SEND);
        hStatus = AddControl(hWnd, L"STATIC", L"", 0, 10, 180, 400, 45, IDC_STATUS);
        RefreshData();
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_REFRESH) RefreshData();
        if (LOWORD(wParam) == IDC_SEND)    SendToServer();
        break;

    case WM_DESTROY:
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
    wc.lpszClassName = L"Lab5Client2";
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowW(L"Lab5Client2", L"Клієнт №2 (ЛР №5)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 440, 280, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
