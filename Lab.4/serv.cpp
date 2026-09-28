#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// Ідентифікатори меню
#define ID_SERVER_START    2001
#define ID_SERVER_STOP     2002
#define IDM_EXIT           105

#define SERV_PORT 5000
#define WSA_ACCEPT   (WM_USER + 0)
#define WSA_NETEVENT (WM_USER + 1)

// Глобальні змінні
SOCKET srv_socket = INVALID_SOCKET;
SOCKET sock[2] = { INVALID_SOCKET, INVALID_SOCKET };
SOCKADDR_IN sockaddr[2];
HWND hwndList = NULL;

// Динамічне створення меню
HMENU CreateServerMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenuW(hSubMenu, MF_STRING, ID_SERVER_START, L"Start Server");
    AppendMenuW(hSubMenu, MF_STRING, ID_SERVER_STOP, L"Stop Server");
    AppendMenuW(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSubMenu, MF_STRING, IDM_EXIT, L"Exit");
    AppendMenuW(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, L"File");
    return hMenu;
}

// Вивід текстових повідомлень у вікно
void LogMessage(const wchar_t* msg) {
    if (hwndList) {
        SendMessageW(hwndList, LB_ADDSTRING, 0, (LPARAM)msg);
    }
}

// Запуск сервера
void StartServer(HWND hWnd) {
    if (srv_socket != INVALID_SOCKET) {
        LogMessage(L"Server is already running!");
        return;
    }

    srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_socket == INVALID_SOCKET) {
        LogMessage(L"Error creating socket!");
        return;
    }

    SOCKADDR_IN sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(SERV_PORT);
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(srv_socket, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR) {
        LogMessage(L"Bind error!");
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        return;
    }

    if (listen(srv_socket, 2) == SOCKET_ERROR) {
        LogMessage(L"Listen error!");
        closesocket(srv_socket);
        srv_socket = INVALID_SOCKET;
        return;
    }

    WSAAsyncSelect(srv_socket, hWnd, WSA_ACCEPT, FD_ACCEPT);
    LogMessage(L"WinSock 2.0 initialized");
    LogMessage(L"Status: Server listening on port 5000");
}

// Обробка подій вікна
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        SetMenu(hWnd, CreateServerMenu());
        hwndList = CreateWindowW(L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
            10, 10, 440, 220, hWnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        StartServer(hWnd);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_SERVER_START:
            StartServer(hWnd);
            break;
        case ID_SERVER_STOP:
            if (srv_socket != INVALID_SOCKET) {
                closesocket(srv_socket);
                srv_socket = INVALID_SOCKET;
                LogMessage(L"Server stopped.");
            }
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        break;
    }
    case WSA_ACCEPT: {
        if (WSAGETSELECTERROR(lParam)) break;

        int nextClient = -1;
        if (sock[0] == INVALID_SOCKET) nextClient = 0;
        else if (sock[1] == INVALID_SOCKET) nextClient = 1;

        if (nextClient != -1) {
            int addrlen = sizeof(SOCKADDR_IN);
            sock[nextClient] = accept(srv_socket, (LPSOCKADDR)&sockaddr[nextClient], &addrlen);
            WSAAsyncSelect(sock[nextClient], hWnd, WSA_NETEVENT, FD_READ | FD_CLOSE);

            wchar_t msg[128];
            swprintf(msg, 128, L"Client %d connected!", nextClient + 1);
            LogMessage(msg);
        }
        break;
    }
    case WSA_NETEVENT: {
        SOCKET s = (SOCKET)wParam;
        WORD event = WSAGETSELECTEVENT(lParam);

        if (event == FD_READ) {
            char buf[512] = { 0 };
            int bytes = recv(s, buf, sizeof(buf) - 1, 0);
            if (bytes > 0) {
                wchar_t wbuf[512] = { 0 };
                MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, 512);
                LogMessage(wbuf);
            }
        }
        else if (event == FD_CLOSE) {
            for (int i = 0; i < 2; i++) {
                if (sock[i] == s) {
                    closesocket(sock[i]);
                    sock[i] = INVALID_SOCKET;
                    wchar_t msg[128];
                    swprintf(msg, 128, L"Client %d disconnected.", i + 1);
                    LogMessage(msg);
                    break;
                }
            }
        }
        break;
    }
    case WM_DESTROY:
        WSACleanup();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"ServerWindowClass";

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"ServerWindowClass", L"Server (Lab 4)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 480, 300,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}