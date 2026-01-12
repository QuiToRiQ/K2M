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
#include <unordered_map>
#include <functional>
#include <fstream>    
#include <string>     
#include <algorithm>  
#include <sstream>    
#include <filesystem>

struct UserSettings
{
	bool bHoldCapsLock = true; // decides either to use toggle or hold capslock method
	bool bUseArrows = true; // switches between arrow keys / back&forward mouse buttons
	int baseMouseSpeed = 3;
	int MouseAccelerationMultiplier = 7;
	int baseScrollSpeed = 120;
	int ScrollAccelerationDivider = 4;
	//////////////////////////////////////////////////////////////////////////////////////////////
	// choose between ctrl+lmb or middle mouse button, which either can work depending on app
	// velocities for each mode...
	// High priority startup
	// Settings
	//////////////////////////////////////////////////////////////////////////////////////////////
};
UserSettings US_Profile;

struct CustomSaves
{
	// Load settings from a .ini file
	static void loadSettings(const std::string& filename, UserSettings& settings)
	{
		std::ifstream file(filename);
		if (!file.is_open()) {
			std::cerr << "Failed to open file: " << filename << "\n";
			return;
		}

		std::unordered_map<std::string, std::function<void(const std::string&)>> setters;
		setters["bHoldCapsLock"] = [&](const std::string& val) { settings.bHoldCapsLock = CustomSaves::stringToBool(val); };
		setters["bUseArrows"] = [&](const std::string& val) { settings.bUseArrows = CustomSaves::stringToBool(val); };
		setters["baseMouseSpeed"] = [&](const std::string& val) { settings.baseMouseSpeed = std::stoi(val); };
		setters["MouseAccelerationMultiplier"] = [&](const std::string& val) { settings.MouseAccelerationMultiplier = std::stoi(val); };
		setters["baseScrollSpeed"] = [&](const std::string& val) { settings.baseScrollSpeed = std::stoi(val); };
		setters["ScrollAccelerationDivider"] = [&](const std::string& val) { settings.ScrollAccelerationDivider = std::stoi(val); };

		std::string line;
		while (std::getline(file, line)) {
			line = CustomSaves::trim(line);

			if (line.empty() || line[0] == ';' || line[0] == '#') continue;
			if (line.front() == '[' && line.back() == ']') continue;

			size_t pos = line.find('=');
			if (pos == std::string::npos) continue;

			std::string key = CustomSaves::trim(line.substr(0, pos));
			std::string value = CustomSaves::trim(line.substr(pos + 1));

			auto it = setters.find(key);
			if (it != setters.end()) it->second(value);
		}
	}

private:
	// Trim whitespace from both ends
	static std::string trim(const std::string& s)
	{
		size_t start = s.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) return "";
		size_t end = s.find_last_not_of(" \t\r\n");
		return s.substr(start, end - start + 1);
	}

	// Convert string to bool (accepts true/false or 1/0, case-insensitive)
	static bool stringToBool(const std::string& s)
	{
		std::string val = s;
		std::transform(val.begin(), val.end(), val.begin(), ::tolower);
		return (val == "1" || val == "true");
	}
};

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
		if (US_Profile.bUseArrows) PressKey(VK_LEFT);// Left Arrow Key
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
	bool leftHeld = false;
	bool rightHeld = false;
	bool capsHeld = false;

	bool backHeld = false;
	bool forwardHeld = false;
	bool middleHeld = false;

	while (true)
	{
		// Toggle activation on off && Caps Lock when shift pressed
		// Caps Lock on/off for different profiles
		if (US_Profile.bHoldCapsLock)
		{
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
			{
				if (CapsLock_pressed && !capsHeld) ToggleCapsLock();
			}
			else
			{
				if (CapsLock_pressed && !capsHeld) active = true;
				if (!CapsLock_pressed && capsHeld) active = false;
			}
		}
		else
		{
			if (!CapsLock_pressed && capsHeld) 
			{
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000) ToggleCapsLock();
				else active = !active;
			}

		} capsHeld = CapsLock_pressed;

		if (active)
		{
			int dx = 0, dy = 0, ds = 0; // delta x, delta y, delta scroll

			// Cursor Movements
			if (W_pressed) dy -= US_Profile.baseMouseSpeed;
			if (S_pressed) dy += US_Profile.baseMouseSpeed;
			if (A_pressed) dx -= US_Profile.baseMouseSpeed;
			if (D_pressed) dx += US_Profile.baseMouseSpeed;

			// Scroll
			if (R_pressed)  ds += US_Profile.baseScrollSpeed;
			if (F_pressed)  ds -= US_Profile.baseScrollSpeed;

			// Shift = accelerate, Ctrl = slow
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { dx *= US_Profile.MouseAccelerationMultiplier; dy *= US_Profile.MouseAccelerationMultiplier; }
			if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || (GetAsyncKeyState(VK_MENU) & 0x8000)) { 
				dx *= US_Profile.MouseAccelerationMultiplier; 
				dy *= US_Profile.MouseAccelerationMultiplier;  
				ds /= US_Profile.ScrollAccelerationDivider; 
			}

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

/////////////////////////////////////
void AddAppStartup()
{
    // Ask user about auto-start
    int result = MessageBoxW(nullptr,
                             L"Do you want this app to run at startup?",
                             L"My App",
                             MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES)
    {
        // Get current executable path
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        std::wstring exePath(buffer);

        // Add to registry startup
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                          0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey, L"MyApp", 0, REG_SZ,
                           (const BYTE*)exePath.c_str(),
                           (DWORD)((exePath.size() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    }

}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Global\\CustomMouseKeysInstanceLock");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// Another copy is already running
		MessageBoxW(nullptr, L"The application is already running. Try holding down or pressing the Caps Lock key to see if you can control the cursor with WASD.", L"Error", MB_ICONERROR);
		return 0;
	}

	AddAppStartup();

	CustomSaves::loadSettings("config.ini", US_Profile);
		
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
