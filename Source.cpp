#include <Windows.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

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

    static void LeftDown() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &i, sizeof(INPUT)); }
    static void LeftUp() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_LEFTUP;   SendInput(1, &i, sizeof(INPUT)); }
    static void RightDown() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; SendInput(1, &i, sizeof(INPUT)); }
    static void RightUp() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_RIGHTUP;   SendInput(1, &i, sizeof(INPUT)); }

    static void Scroll(int amount)
    {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = amount;
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
std::atomic<bool> R_pressed(false); // scroll up
std::atomic<bool> F_pressed(false); // scroll down
std::atomic<bool> active(false);

HHOOK keyboardHook;

// Double-tap Left Alt detection
std::chrono::steady_clock::time_point lastAltPress;
bool altDown = false;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        // Double-tap Left Alt toggle
        if (kb->vkCode == VK_LMENU && keyDown && !altDown)
        {
            altDown = true;
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAltPress).count();
            if (duration <= 300)
            {
                active = !active;
                std::cout << "Active = " << active << "\n";
            }
            lastAltPress = now;
        }
        if (kb->vkCode == VK_LMENU && keyUp)
        {
            altDown = false;
        }

        // Only handle keys when active
        if (active)
        {
            switch (kb->vkCode)
            {
            case 'W': W_pressed = keyDown; return 1;
            case 'A': A_pressed = keyDown; return 1;
            case 'S': S_pressed = keyDown; return 1;
            case 'D': D_pressed = keyDown; return 1;
            case 'Q': Q_pressed = keyDown; return 1;
            case 'E': E_pressed = keyDown; return 1;
            case 'R': R_pressed = keyDown; return 1;
            case 'F': F_pressed = keyDown; return 1;
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Input loop
void InputLoop()
{
    const int baseSpeed = 5;
    bool leftHeld = false;
    bool rightHeld = false;

    while (true)
    {
        if (active)
        {
            int dx = 0, dy = 0;
            if (W_pressed) dy -= baseSpeed;
            if (S_pressed) dy += baseSpeed;
            if (A_pressed) dx -= baseSpeed;
            if (D_pressed) dx += baseSpeed;

            // Shift = accelerate, Ctrl = slow
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { dx *= 2; dy *= 2; }
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) { dx /= 2; dy /= 2; }

            MyMouse::MoveMouse(dx, dy);

            // Handle left click hold
            if (Q_pressed && !leftHeld) { MyMouse::LeftDown(); leftHeld = true; }
            if (!Q_pressed && leftHeld) { MyMouse::LeftUp(); leftHeld = false; }

            // Handle right click hold
            if (E_pressed && !rightHeld) { MyMouse::RightDown(); rightHeld = true; }
            if (!E_pressed && rightHeld) { MyMouse::RightUp(); rightHeld = false; }

            // Scroll
            if (R_pressed) MyMouse::Scroll(120); // scroll up
            if (F_pressed) MyMouse::Scroll(-120); // scroll down
        }
        else
        {
            // Release any held clicks when inactive
            if (leftHeld) { MyMouse::LeftUp(); leftHeld = false; }
            if (rightHeld) { MyMouse::RightUp(); rightHeld = false; }
        }

        Sleep(10);
    }
}

int main()
{
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!keyboardHook) { std::cout << "Failed to install hook!\n"; return 1; }

    std::cout << "Double-tap Left Alt to toggle functionality ON/OFF\n";
    std::cout << "W/A/S/D = move, Q = left click, E = right click, R = scroll up, F = scroll down\n";
    std::cout << "Shift = accelerate, Ctrl = slow\n";

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
