// keyrdpcpp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "keyrdpcpp.h"

#include <Windows.h>
#include <string>
#include <Wtsapi32.h>
#include <iostream>

#pragma comment(lib, "Wtsapi32.lib")
#define MAX_LOADSTRING 100


HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

DWORD GetActiveSessionId() {
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF) {
        std::cerr << "Failed to get active session ID." << std::endl;
    }
    return sessionId;
}

void ShowErrorMessage(const std::wstring& message, std::wostream& errorStream) {
    errorStream << "Error: " << message << " (Code: " << GetLastError() << ")" << std::endl;
    MessageBox(nullptr, message.c_str(), L"Error", MB_ICONERROR | MB_OK);
}

BOOL SetPrivilege(HANDLE hToken, LPCWSTR privilegeName, BOOL enablePrivilege) {
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValueW(nullptr, privilegeName, &luid)) {
        std::cerr << "LookupPrivilegeValue failed. Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr)) {
        std::cerr << "AdjustTokenPrivileges failed. Error: " << GetLastError() << std::endl;
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        std::cerr << "The token does not have the specified privilege." << std::endl;
        return FALSE;
    }

    return TRUE;
}

PHANDLE GetCurrentUserToken()
{
    PHANDLE currentToken = 0;
    PHANDLE primaryToken = 0;

    int dwSessionId = 0;
    PHANDLE hUserToken = 0;
    PHANDLE hTokenDup = 0;

    PWTS_SESSION_INFO pSessionInfo = 0;
    DWORD dwCount = 0;

    // Get the list of all terminal sessions 
    WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1,
        &pSessionInfo, &dwCount);

    int dataSize = sizeof(WTS_SESSION_INFO);

    // look over obtained list in search of the active session
    for (DWORD i = 0; i < dwCount; ++i)
    {
        WTS_SESSION_INFO si = pSessionInfo[i];
        if (WTSActive == si.State)
        {
            // If the current session is active – store its ID
            dwSessionId = si.SessionId;
            break;
        }
    }

    WTSFreeMemory(pSessionInfo);

    // Get token of the logged in user by the active session ID
    BOOL bRet = WTSQueryUserToken(dwSessionId, currentToken);
    if (bRet == false)
    {
        return 0;
    }

    bRet = DuplicateTokenEx(currentToken,
        TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS,
        0, SecurityImpersonation, TokenPrimary, primaryToken);
    if (bRet == false)
    {
        return 0;
    }

    return primaryToken;
}


bool LaunchCalculatorInRemoteSession(DWORD sessionId) {
    HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;
    HANDLE hToken = GetCurrentUserToken();
  //  LPWSTR pWinStationName = NULL;
    WTS_CONNECTSTATE_CLASS connectState;
  
    LPWSTR pWinStationName = NULL;

    // First, get the required buffer size
    DWORD bufferSize = 0;

    DWORD bytesReturned = 0;
    LPTSTR pData = NULL;

   

   

    // Get token of the logged in user by the active session ID
   // BOOL bRet = WTSQueryUserToken(sessionId, &hToken);

    SetPrivilege(hToken, SE_ASSIGNPRIMARYTOKEN_NAME, TRUE);
    SetPrivilege(hToken, SE_INCREASE_QUOTA_NAME, TRUE);
    SetPrivilege(hToken, SE_TCB_NAME, TRUE);



    pWinStationName = (LPWSTR)malloc(bufferSize);

    // Get the WinStation name for the session
    //if (WTSQuerySessionInformationW(hServer, sessionId, WTSWinStationName, NULL, &bufferSize)) {
    //    // Open the primary access token for the logged-on user
    //    if (WTSQueryUserToken(sessionId, &hToken)) {
            // Query the session's connection state
            if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, WTSSessionId, &pData, &bytesReturned)) {
               // if (connectState == WTSActive) {
                    // Session is active, try to create process in the session
                    PROCESS_INFORMATION processInfo;
                    STARTUPINFO startupInfo;

                    ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
                    startupInfo.cb = sizeof(STARTUPINFO);
                    startupInfo.lpDesktop = pWinStationName;


                    /////////////////////////////
                   
                    //////////////////////////////




                    if (CreateProcessAsUser(hToken, L"calc.exe", NULL, NULL, NULL, FALSE,
                        CREATE_UNICODE_ENVIRONMENT | HIGH_PRIORITY_CLASS, NULL, NULL, &startupInfo, &processInfo)) {
                        std::cout << "Calculator launched in remote session." << std::endl;
                        CloseHandle(processInfo.hProcess);
                        CloseHandle(processInfo.hThread);
                        CloseHandle(hToken);
                        WTSFreeMemory(pWinStationName);
                        return true;
                    }
                    else {
                        ShowErrorMessage(L"An error occurred. Please check your settings.", std::wcerr);
                        std::cerr << "Failed to launch Calculator in remote session. Error: " << GetLastError() << std::endl;
                    }
               /* }
                else {
                    std::cerr << "Session is not in active state." << std::endl;
                }*/
            }
            else {
                ShowErrorMessage(L"An error occurred. Please check your settings.", std::wcerr);


                //std::cerr << "Failed to query session information. Error: " << GetLastError() << std::endl;
            }
            CloseHandle(hToken);
       /* }
        else {
            std::cerr << "Failed to open user token. Error: " << GetLastError() << std::endl;
        }
        WTSFreeMemory(pWinStationName);
    }
    else {
        std::cerr << "Failed to query session information. Error: " << GetLastError() << std::endl;
    }*/

    return false;
}





#define MAX_LOADSTRING 100

// Global Variables:
                             // current instance
                // The title bar text
           // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_KEYRDPCPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYRDPCPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KEYRDPCPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_KEYRDPCPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


HWND g_buttonHandle;
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    case WM_CREATE:
        // Create a button during the window creation
        g_buttonHandle = CreateWindowW(L"BUTTON", L"Launch Calculator",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 10, 150, 30,
            hWnd, (HMENU)1, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
        break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {


        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case 1: // Button click event
            // Get the active session ID dynamically
            DWORD activeSessionId = GetActiveSessionId();

            // Launch Calculator in the remote session
            LaunchCalculatorInRemoteSession(activeSessionId);
            break;
       // default:
         //   return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
