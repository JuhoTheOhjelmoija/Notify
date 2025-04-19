#include <windows.h>
#include <string>
#include <shellapi.h>

// Constants
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

// Global variables
HWND g_hwnd = nullptr;
NOTIFYICONDATAW g_nid = {};
HMENU g_hMenu = nullptr;
HFONT g_hFont = nullptr;

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NotificationWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowNotification(const std::wstring& text);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow) {
    // Register window class
    const wchar_t CLASS_NAME[] = L"ClipboardMonitorClass";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    if (!RegisterClassW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create hidden window
    g_hwnd = CreateWindowExW(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        L"Clipboard Monitor",           // Window text
        WS_OVERLAPPED,                  // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,  // Position
        0, 0,                          // Size (0,0 = hidden)
        HWND_MESSAGE,                  // Parent window (message-only)
        nullptr,                       // Menu
        hInstance,                     // Instance handle
        nullptr                        // Additional application data
    );

    if (!g_hwnd) {
        MessageBoxW(nullptr, L"Failed to create window!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create font for notifications
    g_hFont = CreateFontW(
        16,                    // Height
        0,                     // Width
        0,                     // Escapement
        0,                     // Orientation
        FW_NORMAL,            // Weight
        FALSE,                // Italic
        FALSE,                // Underline
        0,                    // StrikeOut
        ANSI_CHARSET,         // CharSet
        OUT_DEFAULT_PRECIS,   // OutPrecision
        CLIP_DEFAULT_PRECIS,  // ClipPrecision
        CLEARTYPE_QUALITY,    // Quality
        DEFAULT_PITCH | FF_DONTCARE,  // PitchAndFamily
        L"Segoe UI"          // Face name
    );

    // Create tray menu
    g_hMenu = CreatePopupMenu();
    AppendMenuW(g_hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    // Setup tray icon
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"Clipboard Monitor");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Add clipboard format listener
    if (!AddClipboardFormatListener(g_hwnd)) {
        MessageBoxW(nullptr, L"Failed to add clipboard listener!", L"Error", MB_OK | MB_ICONERROR);
        DestroyWindow(g_hwnd);
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup
    DeleteObject(g_hFont);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLIPBOARDUPDATE:
            if (OpenClipboard(hwnd)) {
                HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                if (hData) {
                    const wchar_t* clipboardText = static_cast<const wchar_t*>(GlobalLock(hData));
                    if (clipboardText) {
                        // Show balloon tip
                        g_nid.uFlags = NIF_INFO;
                        wcscpy_s(g_nid.szInfo, L"Content copied to clipboard");
                        wcscpy_s(g_nid.szInfoTitle, L"Clipboard Monitor");
                        g_nid.dwInfoFlags = NIIF_INFO;
                        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
                        
                        GlobalUnlock(hData);
                    }
                }
                CloseClipboard();
            }
            break;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                DestroyWindow(hwnd);
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK NotificationWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static UINT_PTR timerId = 0;

    switch (uMsg) {
        case WM_CREATE:
            // Start timer to close the window
            timerId = SetTimer(hwnd, 1, 2000, nullptr);
            break;

        case WM_TIMER:
            KillTimer(hwnd, timerId);
            DestroyWindow(hwnd);
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Set up DC
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            SelectObject(hdc, g_hFont);

            // Get window text
            wchar_t text[256];
            GetWindowTextW(hwnd, text, 256);

            // Calculate text rectangle
            RECT rc;
            GetClientRect(hwnd, &rc);
            rc.left += 10;  // Add padding
            rc.top += 10;

            // Draw text
            DrawTextW(hdc, text, -1, &rc, DT_LEFT | DT_WORDBREAK);
            
            EndPaint(hwnd, &ps);
            break;
        }

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void ShowNotification(const std::wstring& text) {
    // Get the work area (screen size without taskbar)
    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    // Calculate window position (bottom right of work area)
    int width = 300;
    int height = 80;
    int x = workArea.right - width - 20;
    int y = workArea.bottom - height - 20;

    // Create notification window
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        L"ClipboardNotificationClass",
        text.c_str(),
        WS_POPUP,
        x, y, width, height,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (hwnd) {
        // Set window transparency
        SetLayeredWindowAttributes(hwnd, 0, 240, LWA_ALPHA);
        
        // Show window
        ShowWindow(hwnd, SW_SHOWNA);
        UpdateWindow(hwnd);

        // Play notification sound
        PlaySoundW(L"SystemNotification", nullptr, SND_ALIAS | SND_ASYNC);
    }
}