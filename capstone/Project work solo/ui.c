#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <windows.h>

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitUI(HWND hwnd);
void AddLog(HWND hwnd, const char *text);
void Connect(HWND hwnd);
void ListFiles(HWND hwnd);

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
// Global variables
HWND g_hIPTextBox;
HWND g_hPortTextBox;
HWND g_hConnectButton;
HWND g_hListBox;
SOCKET connection;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    HWND hwnd;
    MSG msg;

    // Register the window class
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FileTransferApp";

    RegisterClass(&wc);

    // Create the window
    hwnd = CreateWindowEx(
        0,
        "FileTransferApp",
        "File Transfer Application",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    // Check for window creation failure
    if (hwnd == NULL) {
        return 0;
    }

    InitUI(hwnd);

    // Show the window
    ShowWindow(hwnd, nCmdShow);

    // Main message loop
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // Connect button clicked
                Connect(hwnd);
            } else if (LOWORD(wParam) == 2) { // List files button clicked
                ListFiles(hwnd);
            }
            return 0;
        case WM_SIZE: {
            RECT rect;
            GetClientRect(hwnd, &rect);

            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // Resize the controls based on the new window size
            MoveWindow(g_hIPTextBox, 20, 20, 200, 25, TRUE);
            MoveWindow(g_hPortTextBox, 240, 20, 100, 25, TRUE);
            MoveWindow(g_hConnectButton, 360, 20, 100, 25, TRUE);
            MoveWindow(g_hListBox, 20, 70, width - 40, height - 90, TRUE);

            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitUI(HWND hwnd) {
    // Create textboxes for IP and Port
    g_hIPTextBox = CreateWindow("EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER,
                                20, 20, 200, 25, hwnd, NULL, NULL, NULL);
    g_hPortTextBox = CreateWindow("EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER,
                                  240, 20, 100, 25, hwnd, NULL, NULL, NULL);

    // Create Connect button
    g_hConnectButton = CreateWindow("BUTTON", "Connect", WS_VISIBLE | WS_CHILD,
                                    360, 20, 100, 25, hwnd, (HMENU)1, NULL, NULL);

    // Create listbox to display file system
    g_hListBox = CreateWindow("LISTBOX", NULL, WS_VISIBLE | WS_BORDER | WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL,
                              20, 70, 740, 400, hwnd, NULL, NULL, NULL);
}

void AddLog(HWND hwnd, const char *text) {
    SendMessage(g_hListBox, LB_ADDSTRING, 0, (LPARAM)text);
}

void Connect(HWND hwnd) {
    // Get host and port from edit boxes
    char host[256];
    char port[10];
    GetWindowText(g_hIPTextBox, host, sizeof(host));
    GetWindowText(g_hPortTextBox, port, sizeof(port));

    // Connect code here...

    AddLog(hwnd, "Connected to server!");
}

void ListFiles(HWND hwnd) {
    // List files code here...

    AddLog(hwnd, "List of files:");
    AddLog(hwnd, "file1.txt");
    AddLog(hwnd, "file2.txt");
    AddLog(hwnd, "file3.txt");
}
