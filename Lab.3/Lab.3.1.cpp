#include <iostream>
#include <vector>
#include <windows.h>
#include <chrono>

using namespace std;

const int ARRAY_SIZE = 100000000;
vector<int> dataArray;
long long globalSum = 0;

HANDLE hMutex;
HANDLE hStartEvent;
HANDLE hConsoleMutex;

struct ThreadData {
    int startIdx;
    int endIdx;
};

void SafePrint(const string& msg, DWORD threadId) {
    WaitForSingleObject(hConsoleMutex, INFINITE);
    cout << "[Потік " << threadId << "] " << msg << "\n";
    ReleaseMutex(hConsoleMutex);
}

DWORD WINAPI WorkerProc(LPVOID lpParam) {
    ThreadData* td = (ThreadData*)lpParam;
    DWORD tid = GetCurrentThreadId();

    SafePrint("Створено. Очікування команди на старт...", tid);

    WaitForSingleObject(hStartEvent, INFINITE);
    SafePrint("Почав обчислення своєї частини масиву.", tid);

    long long localSum = 0;
    for (int i = td->startIdx; i < td->endIdx; ++i) {
        localSum += dataArray[i];
    }

    SafePrint("Очікування доступу до глобальної суми (Mutex)...", tid);
    WaitForSingleObject(hMutex, INFINITE);

    SafePrint("Доступ отримано. Оновлення глобальної суми.", tid);
    globalSum += localSum;

    ReleaseMutex(hMutex);

    SafePrint("Завершує роботу.", tid);

    ExitThread(0);
    return 0;
}

int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    cout << "Ініціалізація масиву (" << ARRAY_SIZE << " елементів)...\n";
    dataArray.assign(ARRAY_SIZE, 1);

    hMutex = CreateMutex(NULL, FALSE, NULL);
    hConsoleMutex = CreateMutex(NULL, FALSE, NULL);
    hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    int numThreads;
    cout << "\nВведіть кількість потоків (наприклад: 2, 4, 8 або 16): ";
    cin >> numThreads;

    if (numThreads <= 0 || numThreads > 64) {
        cout << "Некоректна кількість потоків.\n";
        return 1;
    }

    globalSum = 0;
    ResetEvent(hStartEvent);

    vector<HANDLE> hThreads(numThreads);
    vector<ThreadData> tData(numThreads);

    int chunkSize = ARRAY_SIZE / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        tData[i].startIdx = i * chunkSize;
        tData[i].endIdx = (i == numThreads - 1) ? ARRAY_SIZE : (i + 1) * chunkSize;

        hThreads[i] = CreateThread(NULL, 0, WorkerProc, &tData[i], 0, NULL);
    }

    cout << "\nУсі потоки створені. Натисніть Enter для початку обчислень...";
    cin.ignore();
    cin.get();

    auto startTime = chrono::high_resolution_clock::now();

    SetEvent(hStartEvent);

    WaitForMultipleObjects(numThreads, hThreads.data(), TRUE, INFINITE);

    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

    cout << "\n=== РЕЗУЛЬТАТИ ===\n";
    cout << "Обчислена сума: " << globalSum << " (Очікувалося: " << ARRAY_SIZE << ")\n";
    cout << "Час виконання: " << duration.count() << " мс\n";

    for (int i = 0; i < numThreads; ++i) {
        CloseHandle(hThreads[i]);
    }
    CloseHandle(hMutex);
    CloseHandle(hConsoleMutex);
    CloseHandle(hStartEvent);

    return 0;
}