#include <windows.h>
#include <shellapi.h>
#include <string>

#pragma comment(lib, "shell32.lib")

#define WM_TRAYICON (WM_USER + 1)

#define ID_TRAY_SETTINGS 1001
#define ID_TRAY_STARTUP  1002
#define ID_TRAY_EXIT     1003

#define ID_SETTINGS_STARTUP 2001
#define ID_SETTINGS_CLOSE   2002

const wchar_t APP_NAME[] = L"KeyStatus";
const wchar_t MUTEX_NAME[] = L"KeyStatus_SingleInstance";

NOTIFYICONDATAW nid{};

bool g_startup = false;

HICON g_icon = nullptr;

// ------------------------------------------------------------
// Tạo icon C / N
// ------------------------------------------------------------

HICON CreateKeyIcon(bool caps, bool num)
{
    const int W = 64;
    const int H = 64;

    HDC hdc = GetDC(nullptr);

    HDC memDC = CreateCompatibleDC(hdc);

    HBITMAP bitmap =
        CreateCompatibleBitmap(hdc, W, H);

    HGDIOBJ oldBitmap =
        SelectObject(memDC, bitmap);

    // Nền trắng
    HBRUSH white =
        CreateSolidBrush(RGB(255, 255, 255));

    RECT rc{ 0, 0, W, H };

    FillRect(memDC, &rc, white);

    DeleteObject(white);

    // Màu xanh dương
    COLORREF blue =
        RGB(30, 120, 220);

    COLORREF gray =
        RGB(170, 170, 170);

    // C
    HBRUSH brushC =
        CreateSolidBrush(caps ? blue : gray);

    RECT rcC{ 2, 2, 30, 62 };

    FillRect(memDC, &rcC, brushC);

    DeleteObject(brushC);

    // N
    HBRUSH brushN =
        CreateSolidBrush(num ? blue : gray);

    RECT rcN{ 34, 2, 62, 62 };

    FillRect(memDC, &rcN, brushN);

    DeleteObject(brushN);

    // Font
    HFONT font =
        CreateFontW(
            42,
            0,
            0,
            0,
            FW_BOLD,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_SWISS,
            L"Arial"
        );

    HFONT oldFont =
        (HFONT)SelectObject(memDC, font);

    SetBkMode(memDC, TRANSPARENT);

    SetTextColor(
        memDC,
        RGB(255, 255, 255)
    );

    RECT textC{ 2, 7, 30, 62 };

    DrawTextW(
        memDC,
        L"C",
        -1,
        &textC,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RECT textN{ 34, 7, 62, 62 };

    DrawTextW(
        memDC,
        L"N",
        -1,
        &textN,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(memDC, oldFont);

    DeleteObject(font);

    // Icon mask
    HBITMAP mask =
        CreateBitmap(
            W,
            H,
            1,
            1,
            nullptr
        );

    ICONINFO ii{};

    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;

    ii.hbmMask = mask;
    ii.hbmColor = bitmap;

    HICON icon =
        CreateIconIndirect(&ii);

    DeleteObject(mask);

    SelectObject(memDC, oldBitmap);

    DeleteObject(bitmap);

    DeleteDC(memDC);

    ReleaseDC(nullptr, hdc);

    return icon;
}

// ------------------------------------------------------------
// Startup
// ------------------------------------------------------------

bool IsStartupEnabled()
{
    HKEY key;

    if (RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_READ,
        &key) != ERROR_SUCCESS)
    {
        return false;
    }

    wchar_t buffer[MAX_PATH];

    DWORD size = sizeof(buffer);

    LONG result =
        RegQueryValueExW(
            key,
            APP_NAME,
            nullptr,
            nullptr,
            (LPBYTE)buffer,
            &size
        );

    RegCloseKey(key);

    return result == ERROR_SUCCESS;
}

void SetStartup(bool enable)
{
    HKEY key;

    if (RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_WRITE,
        &key) != ERROR_SUCCESS)
    {
        return;
    }

    if (enable)
    {
        wchar_t path[MAX_PATH];

        GetModuleFileNameW(
            nullptr,
            path,
            MAX_PATH
        );

        RegSetValueExW(
            key,
            APP_NAME,
            0,
            REG_SZ,
            (BYTE*)path,
            (DWORD)((wcslen(path) + 1) * sizeof(wchar_t))
        );
    }
    else
    {
        RegDeleteValueW(
            key,
            APP_NAME
        );
    }

    RegCloseKey(key);

    g_startup = enable;
}

// ------------------------------------------------------------
// Cập nhật icon
// ------------------------------------------------------------

void UpdateTrayIcon()
{
    bool caps =
        (GetKeyState(VK_CAPITAL) & 1) != 0;

    bool num =
        (GetKeyState(VK_NUMLOCK) & 1) != 0;

    if (g_icon)
    {
        DestroyIcon(g_icon);
        g_icon = nullptr;
    }

    g_icon =
        CreateKeyIcon(
            caps,
            num
        );

    nid.hIcon = g_icon;

    wcscpy_s(
        nid.szTip,
        L"KeyStatus"
    );

    Shell_NotifyIconW(
        NIM_MODIFY,
        &nid
    );
}

// ------------------------------------------------------------
// Settings window
// ------------------------------------------------------------

LRESULT CALLBACK SettingsProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    static HWND checkBox;

    switch (msg)
    {
    case WM_CREATE:
    {
        checkBox =
            CreateWindowW(
                L"BUTTON",
                L"Tự chạy cùng Windows",
                WS_VISIBLE |
                WS_CHILD |
                BS_AUTOCHECKBOX,
                20,
                20,
                220,
                30,
                hwnd,
                (HMENU)ID_SETTINGS_STARTUP,
                GetModuleHandle(nullptr),
                nullptr
            );

        SendMessageW(
            checkBox,
            BM_SETCHECK,
            g_startup ? BST_CHECKED : BST_UNCHECKED,
            0
        );

        CreateWindowW(
            L"BUTTON",
            L"Đóng",
            WS_VISIBLE |
            WS_CHILD |
            BS_PUSHBUTTON,
            170,
            70,
            80,
            30,
            hwnd,
            (HMENU)ID_SETTINGS_CLOSE,
            GetModuleHandle(nullptr),
            nullptr
        );

        break;
    }

    case WM_COMMAND:

        if (LOWORD(wParam) ==
            ID_SETTINGS_STARTUP)
        {
            bool checked =
                SendMessageW(
                    checkBox,
                    BM_GETCHECK,
                    0,
                    0
                ) == BST_CHECKED;

            SetStartup(checked);
        }

        if (LOWORD(wParam) ==
            ID_SETTINGS_CLOSE)
        {
            DestroyWindow(hwnd);
        }

        break;

    case WM_CLOSE:

        DestroyWindow(hwnd);

        break;
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

void ShowSettings(HWND owner)
{
    static bool registered = false;

    HINSTANCE hInst =
        GetModuleHandle(nullptr);

    if (!registered)
    {
        WNDCLASSW wc{};

        wc.lpfnWndProc =
            SettingsProc;

        wc.hInstance =
            hInst;

        wc.lpszClassName =
            L"KeyStatusSettings";

        wc.hbrBackground =
            (HBRUSH)(COLOR_WINDOW + 1);

        RegisterClassW(&wc);

        registered = true;
    }

    HWND hwnd =
        CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            L"KeyStatusSettings",
            L"KeyStatus - Cài đặt",
            WS_OVERLAPPED |
            WS_CAPTION |
            WS_SYSMENU,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            280,
            150,
            owner,
            nullptr,
            hInst,
            nullptr
        );

    ShowWindow(hwnd, SW_SHOW);

    UpdateWindow(hwnd);
}

// ------------------------------------------------------------
// Tray menu
// ------------------------------------------------------------

void ShowTrayMenu(HWND hwnd)
{
    POINT pt;

    GetCursorPos(&pt);

    HMENU menu =
        CreatePopupMenu();

    AppendMenuW(
        menu,
        MF_STRING,
        ID_TRAY_SETTINGS,
        L"Cài đặt"
    );

    AppendMenuW(
        menu,
        MF_STRING |
        (g_startup ? MF_CHECKED : 0),
        ID_TRAY_STARTUP,
        L"Tự chạy cùng Windows"
    );

    AppendMenuW(
        menu,
        MF_SEPARATOR,
        0,
        nullptr
    );

    AppendMenuW(
        menu,
        MF_STRING,
        ID_TRAY_EXIT,
        L"Thoát chương trình"
    );

    SetForegroundWindow(hwnd);

    int command =
        TrackPopupMenu(
            menu,
            TPM_RETURNCMD |
            TPM_RIGHTBUTTON,
            pt.x,
            pt.y,
            0,
            hwnd,
            nullptr
        );

    DestroyMenu(menu);

    switch (command)
    {
    case ID_TRAY_SETTINGS:

        ShowSettings(hwnd);

        break;

    case ID_TRAY_STARTUP:

        SetStartup(!g_startup);

        break;

    case ID_TRAY_EXIT:

        PostMessageW(
            hwnd,
            WM_CLOSE,
            0,
            0
        );

        break;
    }
}

// ------------------------------------------------------------
// Window
// ------------------------------------------------------------

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:

        UpdateTrayIcon();

        break;

    case WM_TRAYICON:

        if (lParam == WM_RBUTTONUP)
        {
            ShowTrayMenu(hwnd);
        }

        if (lParam == WM_LBUTTONDBLCLK)
        {
            ShowSettings(hwnd);
        }

        break;

    case WM_CLOSE:

        Shell_NotifyIconW(
            NIM_DELETE,
            &nid
        );

        if (g_icon)
        {
            DestroyIcon(g_icon);
            g_icon = nullptr;
        }

        DestroyWindow(hwnd);

        break;

    case WM_DESTROY:

        PostQuitMessage(0);

        break;
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int)
{
    // Không cho chạy 2 lần
    HANDLE mutex =
        CreateMutexW(
            nullptr,
            TRUE,
            MUTEX_NAME
        );

    if (GetLastError() ==
        ERROR_ALREADY_EXISTS)
    {
        return 0;
    }

    g_startup =
        IsStartupEnabled();

    // Window class
    WNDCLASSW wc{};

    wc.lpfnWndProc =
        WindowProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        L"KeyStatusMain";

    RegisterClassW(&wc);

    // Window ẩn
    HWND hwnd =
        CreateWindowExW(
            0,
            L"KeyStatusMain",
            APP_NAME,
            WS_OVERLAPPEDWINDOW,
            0,
            0,
            0,
            0,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

    // Tray icon
    nid.cbSize =
        sizeof(NOTIFYICONDATAW);

    nid.hWnd =
        hwnd;

    nid.uID =
        1;

    nid.uFlags =
        NIF_ICON |
        NIF_MESSAGE |
        NIF_TIP;

    nid.uCallbackMessage =
        WM_TRAYICON;

    UpdateTrayIcon();

    Shell_NotifyIconW(
        NIM_ADD,
        &nid
    );

    // Kiểm tra mỗi 200ms
    SetTimer(
        hwnd,
        1,
        200,
        nullptr
    );

    // Message loop
    MSG msg;

    while (
        GetMessageW(
            &msg,
            nullptr,
            0,
            0
        ))
    {
        TranslateMessage(&msg);

        DispatchMessageW(&msg);
    }

    ReleaseMutex(mutex);
    CloseHandle(mutex);

    return 0;
}
