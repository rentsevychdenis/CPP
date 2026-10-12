#include <windows.h>

HWND hEdit;
HANDLE hMapFile, hEvent;

void Append(const wchar_t* text)
{
    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, len, len);
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)text);
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
}

void ReadMapping(HWND hWnd)
{
    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (pBuf == NULL) {
        MessageBoxW(hWnd, L"Подання проектованого файлу неможливе", L"Map", MB_OK);
        return;
    }
    Append((const wchar_t*)pBuf);
    UnmapViewOfFile(pBuf);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        hEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                              0, 0, 0, 0, hWnd, NULL, NULL, NULL);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
        SetTimer(hWnd, 1, 300, NULL);
        break;
    case WM_SIZE:
        MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_TIMER:
        if (hEvent && WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
            ReadMapping(hWnd);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1:
            if (hMapFile) break;
            hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 512, "myFileMapping");
            if (hMapFile == NULL) {
                MessageBoxW(hWnd, L"Не можливо відкрити відображений у пам'яті об'єкт", L"Map", MB_OK);
                break;
            }
            hEvent = CreateEventA(NULL, FALSE, FALSE, "myFileMappingEvent");
            Append(L"Сервер FileMapping запущено");
            break;
        case 2:
            ReadMapping(hWnd);
            break;
        case 3:
            CloseHandle(hMapFile);
            CloseHandle(hEvent);
            hMapFile = hEvent = NULL;
            Append(L"Сервер FileMapping зупинено");
            break;
        }
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
    wc.lpszClassName = L"ServerApp";
    RegisterClassW(&wc);

    HMENU hMenu = CreateMenu();
    HMENU hSub = CreatePopupMenu();
    AppendMenuA(hSub, MF_STRING, 1, "Start");
    AppendMenuA(hSub, MF_STRING, 2, "Read");
    AppendMenuA(hSub, MF_STRING, 3, "Stop");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSub, "FileMapping");

    CreateWindowW(L"ServerApp", L"Сервер", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                  50, 50, 500, 450, NULL, hMenu, hInst, NULL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
