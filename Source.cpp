#include <Windows.h>
#include <iostream>
#include <atomic>
#include <thread>

class MyMouse
{
public:
    static void MoveMouse(int dx, int dy)
    {
        if (dx == 0 && dy == 0) return;

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = dx;
        input.mi.dy = dy;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;

        SendInput(1, &input, sizeof(INPUT));
    }

    static void ClickLeftMouse()
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    static void ClickRightMouse()
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
};

// Key states
std::atomic<bool> W_pressed(false);
std::atomic<bool> A_pressed(false);
std::atomic<bool> S_pressed(false);
std::atomic<bool> D_pressed(false);
std::atomic<bool> Q_pressed(false);
std::atomic<bool> E_pressed(false);
std::atomic<bool> active(false); // ON/OFF switch

// Track click state to prevent repeated clicks
std::atomic<bool> Q_handled(false);
std::atomic<bool> E_handled(false);

HHOOK keyboardHook;

// Hook callback
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;

        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        // F6 always works to toggle
        if (kb->vkCode == VK_F6 && keyDown)
        {
            active = !active;
            std::cout << "Active = " << active << "\n";
            return 1; // suppress F6 to avoid typing in editor
        }

        // Only handle our custom keys when active
        if (active)
        {
            switch (kb->vkCode)
            {
            case 'W': W_pressed = keyDown; return 1;
            case 'A': A_pressed = keyDown; return 1;
            case 'S': S_pressed = keyDown; return 1;
            case 'D': D_pressed = keyDown; return 1;

            case 'Q':
                if (keyDown && !Q_handled) { Q_pressed = true; Q_handled = true; }
                if (keyUp) { Q_pressed = false; Q_handled = false; }
                return 1;

            case 'E':
                if (keyDown && !E_handled) { E_pressed = true; E_handled = true; }
                if (keyUp) { E_pressed = false; E_handled = false; }
                return 1;
            }
        }
    }

    // Let all other keys pass through normally
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
} 

// Input loop
void InputLoop()
{
    const int speed = 5; // pixels per loop
    while (true)
    {
        if (active) // Only move/click if activated
        {
            int dx = 0, dy = 0;

            if (W_pressed) dy -= speed;
            if (S_pressed) dy += speed;
            if (A_pressed) dx -= speed;
            if (D_pressed) dx += speed;

            MyMouse::MoveMouse(dx, dy);

            // Only click once per key press
            if (Q_pressed) { MyMouse::ClickLeftMouse(); Q_pressed = false; }
            if (E_pressed) { MyMouse::ClickRightMouse(); E_pressed = false; }

            // Detect Shift / Ctrl
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            if (shift) std::cout << "Shift held\n";
            if (ctrl)  std::cout << "Ctrl held\n";
        }

        Sleep(10); // smooth update interval
    }
}

int main()
{
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!keyboardHook)
    {
        std::cout << "Failed to install hook!\n";
        return 1;
    }

    std::cout << "Hook installed. Press F6 to toggle ON/OFF.\n";

    std::thread loopThread(InputLoop);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    loopThread.join();
    UnhookWindowsHookEx(keyboardHook);
    return 0;
}
