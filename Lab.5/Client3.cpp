#include <windows.h>
#include <stdio.h>

#define IDC_WIDTH   101
#define IDC_HEIGHT  102
#define IDC_CALC    103
#define IDC_DATA    104
#define IDC_SEND    105
#define IDC_STATUS  106

const wchar_t* MAILSLOT_NAME = L"\\\\.\\mailslot\\Lab5_Mailslot";

HWND hWidth, hHeight, hData, hStatus;

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
    else
        swprintf_s(text, L"Помилка %s, код %lu.", function, error);
    SetWindowTextW(hStatus, text);
}

bool ReadNumber(HWND hEdit, double& value)
{
    wchar_t text[50];
    GetWindowTextW(hEdit, text, 50);
    for (int i = 0; text[i]; i++)
        if (text[i] == L',') text[i] = L'.';

    wchar_t* end;
    value = wcstod(text, &end);
    return end != text && *end == L'\0' && value > 0;
}

bool Calculate()
{
    double width, height;
    if (!ReadNumber(hWidth, width) || !ReadNumber(hHeight, height)) {
        SetWindowTextW(hStatus, L"Введіть додатні числа для ширини й висоти (наприклад, 10 і 5).");
        return false;
    }
    double area = width * height;

    SYSTEMTIME t;
    GetLocalTime(&t);

    wchar_t text[300];
    swprintf_s(text,
        L"[CLIENT 3] %02d.%02d.%04d %02d:%02d:%02d\r\n"
        L"Математичне обчислення: площа прямокутника\r\n"
        L"Ширина (Width) = %g\r\n"
        L"Висота (Height) = %g\r\n"
        L"Площа (Area) = %g",
        t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond, width, height, area);
    SetWindowTextW(hData, text);
    SetWindowTextW(hStatus, L"Площу обчислено.");
    return true;
}

void SendToServer()
{
    wchar_t text[300];
    GetWindowTextW(hData, text, 300);

    HANDLE hMailslot = CreateFileW(MAILSLOT_NAME, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hMailslot == INVALID_HANDLE_VALUE) {
        ShowError(L"CreateFile");
        return;
    }

    DWORD size = (DWORD)(wcslen(text) + 1) * sizeof(wchar_t);
    DWORD written;
    if (WriteFile(hMailslot, text, size, &written, NULL))
        SetWindowTextW(hStatus, L"Результат надіслано на сервер.");
    else
        ShowError(L"WriteFile");

    CloseHandle(hMailslot);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        AddControl(hWnd, L"STATIC", L"Ширина:", 0, 10, 14, 50, 20, 0);
        hWidth = AddControl(hWnd, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 60, 10, 80, 22, IDC_WIDTH);
        AddControl(hWnd, L"STATIC", L"Висота:", 0, 155, 14, 50, 20, 0);
        hHeight = AddControl(hWnd, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 205, 10, 80, 22, IDC_HEIGHT);
        AddControl(hWnd, L"BUTTON", L"Обчислити", 0, 300, 8, 110, 26, IDC_CALC);
        hData = AddControl(hWnd, L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY, 10, 45, 400, 85, IDC_DATA);
        AddControl(hWnd, L"BUTTON", L"Надіслати на сервер", 0, 10, 140, 195, 30, IDC_SEND);
        hStatus = AddControl(hWnd, L"STATIC", L"Введіть ширину й висоту прямокутника.", 0, 10, 180, 400, 45, IDC_STATUS);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CALC)
            Calculate();
        if (LOWORD(wParam) == IDC_SEND && Calculate())
            SendToServer();
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
    wc.lpszClassName = L"Lab5Client3";
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowW(L"Lab5Client3", L"Клієнт №3 — площа прямокутника (ЛР №5)",
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
