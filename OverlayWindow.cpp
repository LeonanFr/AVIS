#include <windows.h>
#include <thread>
#include <atomic>

static std::atomic<int>* g_directionPtr = nullptr;
static int lastRenderedDirection = 0;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        int dir = lastRenderedDirection;

        const int barWidth = 10;
        COLORREF barColor = RGB(0, 100, 255);

        HBRUSH brush = CreateSolidBrush(barColor);

        if (dir == 1) {
            RECT leftBar = { 0, 0, barWidth, rect.bottom };
            FillRect(hdc, &leftBar, brush);
        }
        else if (dir == 2) {
            RECT rightBar = { rect.right - barWidth, 0, rect.right, rect.bottom };
            FillRect(hdc, &rightBar, brush);
        }

        DeleteObject(brush);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void RunOverlay(std::atomic<int>& directionRef) {
    g_directionPtr = &directionRef;

    const wchar_t CLASS_NAME[] = L"OverlayWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        nullptr,
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr,
        wc.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    std::thread updater([hwnd]() {
        while (IsWindow(hwnd)) {
            int currentDir = g_directionPtr->load();
            if (currentDir != lastRenderedDirection) {
                lastRenderedDirection = currentDir;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            Sleep(5);
        }
        });

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    updater.join();
}