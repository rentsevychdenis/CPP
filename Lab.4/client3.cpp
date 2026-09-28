#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define ID_EDIT_TEXT 101
#define ID_BTN_SEND 102

#define SERVER_PORT 5000 
#define SERVER_IP "127.0.0.1" 

SOCKET ClientSocket = INVALID_SOCKET;
HWND hEdit = NULL;

// Функція для встановлення/відновлення підключення до сервера
bool ConnectToServer() {
    if (ClientSocket != INVALID_SOCKET) {
        closesocket(ClientSocket);
        ClientSocket = INVALID_SOCKET;
    }

    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(ClientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(ClientSocket);
        ClientSocket = INVALID_SOCKET;
        return false;
    }

    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            20, 20, 200, 25, hWnd, (HMENU)ID_EDIT_TEXT, GetModuleHandle(NULL), NULL);

        CreateWindowA("BUTTON", "Відправити",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            230, 20, 100, 25, hWnd, (HMENU)ID_BTN_SEND, GetModuleHandle(NULL), NULL);

        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            MessageBoxA(hWnd, "Помилка ініціалізації Winsock!", "Помилка", MB_OK | MB_ICONERROR);
            return -1;
        }

        // Спроба підключитися під час запуску клієнта
        ConnectToServer();
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == ID_BTN_SEND) {
            char buffer[256] = { 0 };
            GetWindowTextA(hEdit, buffer, 256);

            if (strlen(buffer) > 0) {
                // Якщо з'єднання відсутнє, намагаємося підключитися
                if (ClientSocket == INVALID_SOCKET) {
                    ConnectToServer();
                }

                // Відправка даних
                int sendResult = send(ClientSocket, buffer, (int)strlen(buffer), 0);

                if (sendResult == SOCKET_ERROR) {
                    if (ConnectToServer() && send(ClientSocket, buffer, (int)strlen(buffer), 0) != SOCKET_ERROR) {
                        SetWindowTextA(hEdit, "");
                    }
                    else {
                        MessageBoxA(hWnd, "Помилка відправки даних. Сервер недоступний.", "Помилка", MB_OK | MB_ICONERROR);
                    }
                }
                else {
                    SetWindowTextA(hEdit, "");
                }
            }
        }
        break;
    }
    case WM_DESTROY: {
        if (ClientSocket != INVALID_SOCKET) {
            closesocket(ClientSocket);
        }
        WSACleanup();
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "Client3StarClass";

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Помилка реєстрації класу вікна!", "Помилка", MB_OK | MB_ICONERROR);
        return 0;
    }

    HWND hWnd = CreateWindowExA(0, "Client3StarClass", "Клієнт 3 (Завдання із зірочкою)",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 370, 110,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}