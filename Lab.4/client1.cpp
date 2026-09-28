#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string>

#ifndef SM_KEYBOARDPRESENT
#define SM_KEYBOARDPRESENT 61
#endif

#ifndef ID_CLIENT_SETCONNECTION
#define ID_CLIENT_SETCONNECTION 1001
#define ID_CLIENT_SENDMSG       1002
#endif

#include "client1.h"

#pragma comment(lib, "WS2_32.lib")

#define MAX_LOADSTRING 100
#define SERV_PORT 5000
#define WSA_NETEVENT (WM_USER+1)

char szBuf[1024];
DWORD cbWritten;
static HWND hwndEdit;
char mess[2048];
char* m_mess = mess;

WSADATA wsaData;
WORD wVersionRequested = MAKEWORD(1, 1);
int err = 0;
SOCKET cln_socket = INVALID_SOCKET;
static PHOSTENT phe;
SOCKADDR_IN dest_sin;
char szHostName[128] = "localhost";

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

// Динамічне створення меню
HMENU CreateClientMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING, ID_CLIENT_SETCONNECTION, L"Set Connection");
    AppendMenuW(hSubMenu, MF_STRING, ID_CLIENT_SENDMSG, L"Send Msg");
    AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSubMenu, MF_STRING, IDM_EXIT, L"Exit");
    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"File");
    return hMenu;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT1, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT1));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    // Відключаємо меню з ресурсів
    wcex.lpszMenuName = NULL;

    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, L"Client 1", WS_OVERLAPPEDWINDOW, 100, 200, 420, 260, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

BOOL SetConnection(HWND hWnd)
{
    cln_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cln_socket == INVALID_SOCKET) return FALSE;
    phe = gethostbyname(szHostName);
    if (phe == NULL) return FALSE;
    dest_sin.sin_family = AF_INET;
    dest_sin.sin_port = htons(SERV_PORT);
    memcpy((char FAR*) & (dest_sin.sin_addr), phe->h_addr, phe->h_length);

    if (connect(cln_socket, (PSOCKADDR)&dest_sin, sizeof(dest_sin)) == SOCKET_ERROR) return FALSE;
    if (WSAAsyncSelect(cln_socket, hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE)) return FALSE;

    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)"Connection established!");
    return TRUE;
}

void SendMsg(HWND hWnd)
{
    int captionHeight = GetSystemMetrics(SM_CYCAPTION);
    int keyboardPresent = GetSystemMetrics(SM_KEYBOARDPRESENT);
    HDC hdc = GetDC(hWnd);
    int vertRes = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(hWnd, hdc);

    sprintf_s(szBuf, sizeof(szBuf), "Caption height: %d px\r\nKeyboard present: %s\r\nScreen height: %d px\r\n", captionHeight, (keyboardPresent ? "Yes" : "No"), vertRes);

    if (send(cln_socket, szBuf, (int)strlen(szBuf), 0) != SOCKET_ERROR)
    {
        sprintf_s(m_mess, 2000, "\r\nData sent to server:\r\n%s", szBuf);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
    }
    else
    {
        sprintf_s(m_mess, 2000, "%s\r\nSend error\r\n", m_mess);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_CREATE:
        // Встановлюємо своє меню
        SetMenu(hWnd, CreateClientMenu());

        hwndEdit = CreateWindow(TEXT("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 0, 0, 400, 200, hWnd, NULL, hInst, NULL);
        err = WSAStartup(wVersionRequested, &wsaData);
        if (err) return FALSE;

        sprintf_s(mess, "Using %s\r\nStatus: %s\r\n", wsaData.szDescription, wsaData.szSystemStatus);
        SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case ID_CLIENT_SETCONNECTION: SetConnection(hWnd); break;
        case ID_CLIENT_SENDMSG: SendMsg(hWnd); break;
        case IDM_ABOUT: DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About); break;
        case IDM_EXIT: WSACleanup(); DestroyWindow(hWnd); break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WSA_NETEVENT:
        if (WSAGETSELECTEVENT(lParam) == FD_READ)
        {
            int rc = recv(cln_socket, szBuf, sizeof(szBuf), 0);
            if (rc > 0) {
                szBuf[rc] = '\0';
                sprintf_s(mess, "%s\r\nData from server: %s\r\n", mess, szBuf);
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
            }
        }
        if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) MessageBoxA(hWnd, "Server closed", "Server", MB_OK);
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_INITDIALOG: return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}