#include <windows.h>
#include <shellapi.h>
#include <thread>
#include <atomic>
#include <iostream>

// Globale Variablen
static NOTIFYICONDATA nid = {0};
static HWND hWnd = NULL;

static void (*leftClickCallback)() = NULL;
static void (*rightClickCallback)() = NULL;
static std::atomic<bool> loopRunning(false);

// Fensterprozedur
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_APP + 1)
  {
    if (lParam == WM_LBUTTONDOWN)
    {
      if (leftClickCallback)
        leftClickCallback();
    }
    else if (lParam == WM_RBUTTONDOWN)
    {
      if (rightClickCallback)
        rightClickCallback();
    }
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Verstecktes Fenster erstellen
HWND CreateHiddenWindow()
{
  WNDCLASS wc = {0};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpszClassName = "TrayWindowClass";
  RegisterClass(&wc);

  HWND hWnd = CreateWindow(
      "TrayWindowClass", NULL, 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
  return hWnd;
}

// DLL-Einstiegspunkt
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  if (ul_reason_for_call == DLL_PROCESS_ATTACH && !hWnd)
  {
    hWnd = CreateHiddenWindow();
    if (!hWnd)
      return FALSE;

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP + 1;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy(nid.szTip, "Deno Tray App");
  }
  return TRUE;
}

// Exportierte Funktionen
extern "C" __declspec(dllexport) void AddTrayIcon()
{
  Shell_NotifyIcon(NIM_ADD, &nid);
}

extern "C" __declspec(dllexport) void RemoveTrayIcon()
{
  Shell_NotifyIcon(NIM_DELETE, &nid);
}

extern "C" __declspec(dllexport) void SetLeftClickCallback(void (*callback)())
{
  leftClickCallback = callback;
}

extern "C" __declspec(dllexport) void SetRightClickCallback(void (*callback)())
{
  rightClickCallback = callback;
}

// Hintergrund-Thread für Message Loop
void MessageLoopThread()
{
  MSG msg;
  loopRunning = true;
  while (loopRunning && GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

// Startet die Message Loop im Hintergrund-Thread
extern "C" __declspec(dllexport) void RunMessageLoop()
{
  std::thread(MessageLoopThread).detach();
}

// Beendet die Message Loop
extern "C" __declspec(dllexport) void QuitMessageLoop()
{
  loopRunning = false;
  PostQuitMessage(0);
}
