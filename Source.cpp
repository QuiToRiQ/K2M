//Remove control functionality because its not comfortable to use.

//Use alt instead of control

//Try making mouse slow by default and two acceleration modifiers alt and shift

//Some apps require special strategy (for example in file explorer. if u want to rename a folder, when u disable mouse the focus lefts on another window * current fix is to press alt again to move focus back)

//If changing speed intrepts mouse click (resets it if was clicked previously)

/*Try using double Caps Lock instead of double ALT for on/off*/ /*double Caps Lock can be used to simulate arrow keys while double Alt for mouse actions*/

/*
Completely suppres Caps Lock
and use double caps lock for caps lock
and single caps lock for on/off mouse switch


Keyboard To Mouse
Suppress: Caps Lock
Caps Lock -> on off
Shift + Caps Lock -> Caps Lock input
*/

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
std::atomic<bool> CapsLock_pressed(false); // enable/disable
std::atomic<bool> W_pressed(false); // up
std::atomic<bool> A_pressed(false); // left
std::atomic<bool> S_pressed(false); // down
std::atomic<bool> D_pressed(false); // right
std::atomic<bool> Q_pressed(false); // lmb
std::atomic<bool> E_pressed(false);	// rmb
std::atomic<bool> R_pressed(false); // scroll up
std::atomic<bool> F_pressed(false); // scroll down
std::atomic<bool> was_exception_pressed(false); // checks if in the previous loop any exception button was pressed
std::atomic<bool> active(false);

HHOOK keyboardHook;

// Double-tap Left Alt detection
std::chrono::steady_clock::time_point lastAltPress;
bool altDown = false;

// Simulate CapsLock button
void ToggleCapsLock()
{
	
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
		bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
		bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

		// -------------------------------
		// CTRL = BYPASS (no suppression)
		// -------------------------------
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		{
			return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
		}

		// -------------------------------
		// Double-tap Left Alt toggle
		// -------------------------------
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

		// -------------------------------
		// Only suppress keys when active
		// -------------------------------
		bool exception_pressed = false;
		switch (kb->vkCode)
		{
		case 'A': A_pressed = keyDown; exception_pressed = true; break;
		case 'W': W_pressed = keyDown; exception_pressed = true; break;
		case 'S': S_pressed = keyDown; exception_pressed = true; break;
		case 'D': D_pressed = keyDown; exception_pressed = true; break;
		case 'Q': Q_pressed = keyDown; exception_pressed = true; break;
		case 'E': E_pressed = keyDown; exception_pressed = true; break;
		case 'R': R_pressed = keyDown; exception_pressed = true; break;
		case 'F': F_pressed = keyDown; exception_pressed = true; break;
		}

		was_exception_pressed = exception_pressed;
		if (active && exception_pressed) return 1;
		if (kb->vkCode == 0x14) { CapsLock_pressed = keyDown; return 1; }
	}

	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Input loop
void InputLoop()
{
	const int baseMouseSpeed = 5;
	const int baseScrollSpeed = 120;
	bool leftHeld = false;
	bool rightHeld = false;
	bool capsHeld = false;

	while (true)
	{
		// Toggle activation on off && Caps Lock when shift pressed
		if (!CapsLock_pressed && capsHeld) 
		{ 
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000) ToggleCapsLock();
			else active = !active; 
		
		} capsHeld = CapsLock_pressed;

		if (active)
		{
			int dx = 0, dy = 0, ds = 0; // delta x, delta y, delta scroll
	
			// Cursor Movements
			if (W_pressed) dy -= baseMouseSpeed;
			if (S_pressed) dy += baseMouseSpeed;
			if (A_pressed) dx -= baseMouseSpeed;
			if (D_pressed) dx += baseMouseSpeed;

			// Scroll
			if (R_pressed)  ds += 120;
			if (F_pressed)  ds -= 120;

			// Shift = accelerate, Ctrl = slow
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { dx *= 3; dy *= 3; }
			//if (GetAsyncKeyState(VK_MENU) & 0x8000) { dx *= 3; dy *= 2; }
			if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || (GetAsyncKeyState(VK_MENU) & 0x8000)) { dx /= 2; dy /= 2;  ds /= 4; }

			if (dx != 0 || dy != 0)MyMouse::MoveMouse(dx, dy);
			if (ds != 0) MyMouse::Scroll(ds);

			// Handle left click hold
			if (Q_pressed && !leftHeld) { MyMouse::LeftDown(); leftHeld = true; }
			if (!Q_pressed && leftHeld) { MyMouse::LeftUp(); leftHeld = false; }

			// Handle right click hold
			if (E_pressed && !rightHeld) { MyMouse::RightDown(); rightHeld = true; }
			if (!E_pressed && rightHeld) { MyMouse::RightUp(); rightHeld = false; }

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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
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
