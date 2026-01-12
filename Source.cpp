//Some apps require special strategy (for example in file explorer. if u want to rename a folder, when u disable mouse the focus lefts on another window * current fix is to press alt again to move focus back)
 
/*
use 'z' 'x' 'c' for middle mouse button and arrow/back&forward keys
make a choice between using arrow/back&forward keys and between holding and pressing capslock
*/

#include <Windows.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

struct UserSettings
{
	std::atomic<bool> bHoldCapsLock = false; // decides either to use toggle or hold capslock method
	std::atomic<bool> bUseArrows = false; // switches between arrow keys / back&forward mouse buttons

	//////////////////////////////////////////////////////////////////////////////////////////////
	// choose between ctrl+lmb or middle mouse button, which either can work depending on app
	//////////////////////////////////////////////////////////////////////////////////////////////
};
UserSettings US_Profile;

class MyMouse
{
public:
	static void MoveMouse(int dx, int dy)
	{
		if (dx == 0 && dy == 0) return;
		INPUT i = {};
		i.type = INPUT_MOUSE;
		i.mi.dx = dx;
		i.mi.dy = dy;
		i.mi.dwFlags = MOUSEEVENTF_MOVE;
		SendInput(1, &i, sizeof(INPUT));
	}

	static void LeftDown() { SendMouse(MOUSEEVENTF_LEFTDOWN); }
	static void LeftUp() { SendMouse(MOUSEEVENTF_LEFTUP); }
	static void RightDown() { SendMouse(MOUSEEVENTF_RIGHTDOWN); }
	static void RightUp() { SendMouse(MOUSEEVENTF_RIGHTUP); }
	static void MiddleDown() { SendMouse(MOUSEEVENTF_MIDDLEDOWN); }
	static void MiddleUp() { SendMouse(MOUSEEVENTF_MIDDLEUP); }

	static void Scroll(int amount)
	{
		INPUT i = {};
		i.type = INPUT_MOUSE;
		i.mi.dwFlags = MOUSEEVENTF_WHEEL;
		i.mi.mouseData = amount;
		SendInput(1, &i, sizeof(INPUT));
	}

	static void Back()
	{
		if(US_Profile.bUseArrows) PressKey(VK_LEFT);// Left Arrow Key
		else XButton(XBUTTON1);// Back Mouse Button
	}

	static void Forward()
	{
		if (US_Profile.bUseArrows) PressKey(VK_RIGHT);// Right Arrow Key
		else XButton(XBUTTON2);// Back Mouse Button
	}

private:
	static void SendMouse(DWORD flags)
	{
		INPUT i = {};
		i.type = INPUT_MOUSE;
		i.mi.dwFlags = flags;
		SendInput(1, &i, sizeof(INPUT));
	}

	static void XButton(WORD button)
	{
		INPUT i = {};
		i.type = INPUT_MOUSE;
		i.mi.dwFlags = MOUSEEVENTF_XDOWN;
		i.mi.mouseData = button;
		SendInput(1, &i, sizeof(INPUT));

		ZeroMemory(&i, sizeof(i));
		i.type = INPUT_MOUSE;
		i.mi.dwFlags = MOUSEEVENTF_XUP;
		i.mi.mouseData = button;
		SendInput(1, &i, sizeof(INPUT));
	}

	static void PressKey(WORD vk)
	{
		INPUT i = {};
		i.type = INPUT_KEYBOARD;
		i.ki.wVk = vk;
		SendInput(1, &i, sizeof(INPUT));

		ZeroMemory(&i, sizeof(i));
		i.type = INPUT_KEYBOARD;
		i.ki.wVk = vk;
		i.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &i, sizeof(INPUT));
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
std::atomic<bool> Z_pressed(false); // back
std::atomic<bool> X_pressed(false); // forward
std::atomic<bool> C_pressed(false); // mmb
std::atomic<bool> active(false);

HHOOK keyboardHook;

// Double-tap Left Alt detection
std::chrono::steady_clock::time_point lastAltPress;
bool altDown = false;

// Simulate CapsLock button
void ToggleCapsLock()
{
	INPUT input[2] = {};

	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wScan = MapVirtualKey(VK_CAPITAL, MAPVK_VK_TO_VSC);
	input[0].ki.dwFlags = KEYEVENTF_SCANCODE;

	input[1] = input[0];
	input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

	SendInput(2, input, sizeof(INPUT));
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
		bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
		bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

		// -------------------------------
		// CTRL/WINDOWS = BYPASS (no suppression)
		// -------------------------------
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		{
			return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
		}
		if (GetAsyncKeyState(VK_LWIN) & 0x8000)
		{
			return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
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
		case 'Z': Z_pressed = keyDown; exception_pressed = true; break;
		case 'X': X_pressed = keyDown; exception_pressed = true; break;
		case 'C': C_pressed = keyDown; exception_pressed = true; break;
		}

		if (active && exception_pressed) return 1;
		if (kb->vkCode == VK_CAPITAL)
		{
			bool injected = (kb->flags & LLKHF_INJECTED) != 0;

			if (!injected)
			{
				// Physical CapsLock → use as mouse toggle key
				CapsLock_pressed = keyDown;
				return 1;    // suppress physical CapsLock
			}

			// Injected CapsLock → allow it to reach Windows
		}
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

	bool backHeld = false;
	bool forwardHeld = false;
	bool middleHeld = false;

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

			// Handle back & forward mouse keys / arrows
			if (Z_pressed && !backHeld) MyMouse::Back(); backHeld = Z_pressed;
			if (X_pressed && !forwardHeld) MyMouse::Forward(); forwardHeld = X_pressed;
			
			// Handle middle mouse buttons
			if (C_pressed && !middleHeld) { MyMouse::MiddleDown(); middleHeld = true; }
			if (!C_pressed && middleHeld) { MyMouse::MiddleUp(); middleHeld = false; }
		}
		else
		{
			// Release any held clicks when inactive
			if (leftHeld) { MyMouse::LeftUp(); leftHeld = false; }
			if (rightHeld) { MyMouse::RightUp(); rightHeld = false; }
			if (middleHeld) { MyMouse::MiddleUp(); middleHeld = false; }
		}

		Sleep(10);
	}
}

// Requires for further setup
bool AskYesNo(const wchar_t* text, const wchar_t* title)
{
	int result = MessageBoxW(
		nullptr,
		text,
		title,
		MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2
	);

	if (result == IDYES)
	{
		return true;
	}
	else if (result == IDNO)
	{
		return false;
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
