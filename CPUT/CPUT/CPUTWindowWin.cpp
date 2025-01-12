/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

#include "CPUTWindowWin.h"
#include "..\..\imgui\imgui.h"
#include "..\..\imgui\DirectX11\imgui_impl_dx11.h"

#if defined( CPUT_FOR_OGL ) || defined( CPUT_FOR_OGLES )
#include "CPUT_OGL.h"
#endif

IMGUI_API bool   ImGui_ImplDX11_WndProcHandler2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


#ifdef _DEBUG

TCHAR lpMsgBuf[100]; // declare global in case error is about lack of resources
_inline void HandleWin32Error()
{
    DWORD err = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        lpMsgBuf,
        100,
        NULL );
    ASSERT(false, lpMsgBuf);
    
}
#else
_inline void HandleWin32Error() {}
#endif

// static initializers
CPUT* CPUTWindowWin::mCPUT=NULL;
bool CPUTWindowWin::mbMaxMinFullScreen=false;

// Constructor
//-----------------------------------------------------------------------------
CPUTWindowWin::CPUTWindowWin() : 
#ifdef CPUT_OS_ANDROID
    window(NULL),                  
#endif
#ifdef CPUT_OS_WINDOWS
    mhInst(NULL),
    mhWnd(NULL),
#endif

    fullscreen(false),
    mAppClosedReturnCode(0)
{
    mAppTitle.clear();
}

// Destructor
//-----------------------------------------------------------------------------
CPUTWindowWin::~CPUTWindowWin()
{
    mAppTitle.clear();
}



// Create window
//-----------------------------------------------------------------------------


CPUTResult CPUTWindowWin::Create(CPUT* cput, const cString WindowTitle, CPUTWindowCreationParams windowParams)
{
   // If were here, we are sure that window is not created.
    // Take into notice this whole framework is not Thread-Safe.
    ASSERT( (windowParams.windowPositionX < GetSystemMetrics(SM_CXFULLSCREEN) && (windowParams.windowPositionX>=-1)), _L("You are attempting to create a window outside the desktop coordinates.  Check your CPUTWindowCreationParams"));
    ASSERT( (windowParams.windowPositionY < GetSystemMetrics(SM_CYFULLSCREEN) && (windowParams.windowPositionY>=-1)), _L("You are attempting to create a window outside the desktop coordinates.  Check your CPUTWindowCreationParams"));

    // Get the hInstance of this executable
    mhInst = GetModuleHandle(NULL);
    if( !mhInst )
    {
        return CPUT_ERROR_CANNOT_GET_WINDOW_INSTANCE;
    }


  
    // Clear message queue
    MSG msg = { 0 };
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );

    }

    // WNDCLASS              Window;             // Window class
    // DEVMODE               DispDevice;         // Display Device settings
    // DWORD                 Style;              // Window style
    // DWORD                 ExStyle;            // Window extended style
    // RECT                  WindowRect;         // Window rectangle
 
    // Load icon
    HANDLE  hIcon = LoadImage(mhInst, L"CPUT.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE );
    // set up app title (if not specified)
    mAppTitle = WindowTitle;

    if(0==mAppTitle.compare(_L("")))
    {
        mAppTitle = _L("CPUT Sample");
    }

     // Register the Win32 class for this app
    WNDCLASS wc;
    if(TRUE == GetClassInfo(mhInst, mAppTitle.c_str(), &wc))
    {
        // point to the existing one
        mhInst = wc.hInstance;
    }
    else
    {
        // register a new windows class
        ATOM classID;
        classID = MyRegisterClass(mhInst);
        if(0==classID)
        {
            HandleWin32Error();
            return CPUT_ERROR_WINDOW_CANNOT_REGISTER_APP;
        }
    }


    // Perform Win32 instance initialization
    const int nCmdShow = SW_SHOWNORMAL;
    if (false == InitInstance(nCmdShow, windowParams.windowWidth, windowParams.windowHeight, windowParams.windowPositionX, windowParams.windowPositionY))
    {
        return CPUT_ERROR_CANNOT_GET_WINDOW_INSTANCE;
    }

    // store the CPUT pointer
    mCPUT = (CPUT*) cput;

    return CPUT_SUCCESS;
}




// Destroy window
//-----------------------------------------------------------------------------
int CPUTWindowWin::Destroy()
{
    // Returning to old display settings
    if ( fullscreen )
    {
        ChangeDisplaySettings( NULL, 0 );
        ShowCursor( TRUE );
    }

    // Deleting window
    if ( mhWnd )
    {
        DestroyWindow( mhWnd );  
        mhWnd = 0;
    } 
    
    // Unregistering window class
    UnregisterClass( mAppTitle.c_str(), mhInst );
    mCPUT = NULL;
    return true;
}


// Window return code on close
//-----------------------------------------------------------------------------
int CPUTWindowWin::ReturnCode()
{
    return mAppClosedReturnCode;
}

//  Register window class
//-----------------------------------------------------------------------------
ATOM CPUTWindowWin::MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    // load icon from resource file
    LPCTSTR iconPathName= L"CPUT.ico";
    UINT icon_flags = LR_LOADFROMFILE | LR_DEFAULTSIZE;
    HANDLE hIcon = LoadImage(hInstance, iconPathName, IMAGE_ICON, 0, 0, icon_flags);

    // set up RegisterClass struct
    wcex.style            = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = (HICON) hIcon; //LoadIcon(hInstance, iconSTR);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = NULL;
    wcex.lpszClassName    = mAppTitle.c_str();
    wcex.hIconSm        = NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL)); // no small icon for now

    // register the window class
    return RegisterClassEx(&wcex);
}



// InitInstance
// Saves the windows instance handle, creates, and displays the main program
// window
//-----------------------------------------------------------------------------
BOOL CPUTWindowWin::InitInstance(int nCmdShow, int windowWidth, int windowHeight, int windowX, int windowY)
{
    // assure we have a valid hInstance
    ASSERT(NULL!=mhInst, _L(""));

   // zero sized windows means - you choose the size. :)
   if( (0==windowWidth) || (0==windowHeight) )
   {
       GetDesktopDimensions(&windowWidth, &windowHeight);

       // default window size 1280x720
       // but if screen is smaller than 1280x720, then pick 1/3 the screen size 
       // so that it doesn't appear off the edges
       if(1280>windowWidth)
       {
           windowWidth = (2*windowWidth)/3;
           windowHeight = (2*windowHeight)/3;
       }
       else
       {
        
        windowWidth=1280;
        windowHeight=720;
       }
   }

   // set up size structure
   RECT rc = { 0, 0, windowWidth, windowHeight };
   AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

   // if x = -1, then let windows decide where to put it
   if(-1==windowX)
   {
       windowX = CW_USEDEFAULT;
   }

   // create the window
   mhWnd = CreateWindow(mAppTitle.c_str(), mAppTitle.c_str(),
       WS_OVERLAPPEDWINDOW,
       windowX, //CW_USEDEFAULT,
       windowY, //CW_USEDEFAULT,
       rc.right - rc.left,
       rc.bottom - rc.top,
       NULL,
       NULL,
       mhInst,
       NULL);

   if (!mhWnd)
   {
      return FALSE;
   }

   ShowWindow(mhWnd, nCmdShow);
   UpdateWindow(mhWnd);

   return TRUE;
}

// Get the OS window dimensions
//-----------------------------------------------------------------------------
void CPUTWindowWin::GetClientDimensions(int *pX, int *pY, int *pWidth, int *pHeight)
{
    RECT windowRect;
    if(0==GetClientRect(mhWnd, &windowRect))
    {
        return;
    }
    *pX      = windowRect.left;
    *pY      = windowRect.top;
    *pWidth  = windowRect.right - windowRect.left;
    *pHeight = windowRect.bottom - windowRect.top;
}

// Get the OS window client area dimensions
//-----------------------------------------------------------------------------
void CPUTWindowWin::GetClientDimensions(int *pWidth, int *pHeight)
{
    RECT windowRect;
    if(0==GetClientRect(mhWnd, &windowRect))
    {
        return;
    }
    *pWidth  = windowRect.right - windowRect.left;
    *pHeight = windowRect.bottom - windowRect.top;
}

// Get the desktop dimensions
//-----------------------------------------------------------------------------
void CPUTWindowWin::GetDesktopDimensions(int *pWidth, int *pHeight)
{
    *pWidth  = GetSystemMetrics(SM_CXFULLSCREEN);  // alternate method: GetSystemMetrics(SM_CXSCREEN);
    *pHeight = GetSystemMetrics(SM_CYFULLSCREEN);  // alternate method: GetSystemMetrics(SM_CYSCREEN);
}


// Returns true if the window is currently maximized
//-----------------------------------------------------------------------------
bool CPUTWindowWin::IsWindowMaximized()
{
    WINDOWPLACEMENT WindowPlacement;
    WindowPlacement.length = sizeof(WindowPlacement);
    GetWindowPlacement(mhWnd,  &WindowPlacement);

    if(SW_SHOWMAXIMIZED == WindowPlacement.showCmd)
    {
        return true;
    }
    
    return false;
}

// Returns true if the window is currently minimized
//-----------------------------------------------------------------------------
bool CPUTWindowWin::IsWindowMinimized()
{
    WINDOWPLACEMENT WindowPlacement;
    WindowPlacement.length = sizeof(WindowPlacement);
    GetWindowPlacement(mhWnd,  &WindowPlacement);

    if (SW_MINIMIZE        == WindowPlacement.showCmd ||
        SW_SHOWMINIMIZED   == WindowPlacement.showCmd ||
        SW_SHOWMINNOACTIVE == WindowPlacement.showCmd
        )
    {
        return true;
    }
    
    return false;
}

// Returns true if the CPUT window is currently the 'focused' window on the 
// desktop
//-----------------------------------------------------------------------------
bool CPUTWindowWin::DoesWindowHaveFocus()
{
    HWND hFocusedWindow = GetActiveWindow();
    if(mhWnd == hFocusedWindow)
    {
        return true;
    }
    return false;
}

// mouse
// this function 'captures' the mouse and makes it ONLY available to this app
// User cannot click on any other app until you call ReleaseMouse, so use this
// carefully
//-----------------------------------------------------------------------------
void CPUTWindowWin::CaptureMouse()
{
    SetCapture(mhWnd);
}

// Releases a captured mouse
//-----------------------------------------------------------------------------
void CPUTWindowWin::ReleaseMouse()
{
    ReleaseCapture();
}

//
// WndProc
// Handles the main message loop's events/messages
//-----------------------------------------------------------------------------
LRESULT CALLBACK CPUTWindowWin::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CPUTEventHandledCode handledCode = CPUT_EVENT_UNHANDLED;
    LRESULT res;
    static bool sizing = false;

	if (ImGui_ImplDX11_WndProcHandler2(hWnd, message, wParam, lParam) == true)
		return 0;

    switch (message)
    {
    case WM_COMMAND:
        int     wmId, wmEvent;
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        // handle any menu item events here
        // see reference code in file history for examples
        break;

    case WM_KEYDOWN:  // WM_KEYDOWN: gives you EVERY key - including shifts/etc
        if(mCPUT)
        {
			CPUTKeyState state = CPUT_KEY_DOWN;
            CPUTKey key = ConvertVirtualKeyToCPUTKey(wParam);
            if(KEY_NONE!=key)
            {
                handledCode = mCPUT->CPUTHandleKeyboardEvent( key, state );
            }
        }
		break;
	case WM_KEYUP:
		if(mCPUT)
        {
			CPUTKeyState state = CPUT_KEY_UP;
			CPUTKey key = ConvertVirtualKeyToCPUTKey(wParam);
            if(KEY_NONE!=key)
            {
				handledCode = mCPUT->CPUTHandleKeyboardEvent( key, state );
            }
        }
        break;
	case WM_CHAR:
        if(mCPUT)
        {
			CPUTKeyState state = CPUT_KEY_DOWN;
            CPUTKey key = ConvertCharacterToCPUTKey(wParam);
            if(KEY_NONE!=key)
            {
                handledCode = mCPUT->CPUTHandleKeyboardEvent( key, state );
            }
        }
        break;

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        // handle double-click events
        break;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if(mCPUT)
        {
            CPUTMouseState state = ConvertMouseState(wParam);

            short xPos = LOWORD(lParam);
            short yPos = HIWORD(lParam);

            handledCode = mCPUT->CPUTHandleMouseEvent(xPos, yPos, 0, state, CPUT_EVENT_DOWN);
        }
        break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        if(mCPUT)
        {
            CPUTMouseState state = ConvertMouseState(wParam);

            short xPos = LOWORD(lParam);
            short yPos = HIWORD(lParam);

            handledCode = mCPUT->CPUTHandleMouseEvent(xPos, yPos, 0, state, CPUT_EVENT_UP);
        }
        break;
    case WM_MOUSEMOVE:
        if(mCPUT)
        {
            CPUTMouseState state = ConvertMouseState(wParam);

            short xPos = LOWORD(lParam);
            short yPos = HIWORD(lParam);

            handledCode = mCPUT->CPUTHandleMouseEvent(xPos, yPos, 0, state, CPUT_EVENT_MOVE);
        }
        break;

    case WM_MOUSEWHEEL:
        if(mCPUT)
        {
            // get mouse position
            short xPos = LOWORD(lParam);
            short yPos = HIWORD(lParam);

            // get wheel delta
            int wheel = GET_WHEEL_DELTA_WPARAM(wParam);  // one 'click'

            handledCode = mCPUT->CPUTHandleMouseEvent(xPos, yPos, wheel, CPUT_MOUSE_WHEEL, CPUT_EVENT_WHEEL);
        }
        return 0;
        break;

    case WM_PAINT:
        PAINTSTRUCT ps;
        HDC hdc;
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_SIZING:
        sizing = true;
        break;
    case WM_MOVING:
    case WM_ERASEBKGND:
        // overriding this to do nothing avoids flicker and
        // the expense of re-creating tons of gfx contexts as it resizes
        break;

    //case WM_ACTIVATE:
        // check for maximize/minimize
      //  break;

    case WM_SIZE:
        int width, height;
        height = HIWORD(lParam);
        width  = LOWORD(lParam);
            
        RECT windowRect;
        if(0==GetClientRect(hWnd, &windowRect)) // this gets the client area inside the window frame *excluding* frames/menu bar/etc
            break;
        width = windowRect.right - windowRect.left;
        height = windowRect.bottom - windowRect.top;

        // if we have shrunk to 0 width/height - do not pass on this kind of resize - leads to 
        // various render target resizing warnings
        if(0==width || 0==height)
        {
            break;
        }

        if(mCPUT)
        {
            if(!sizing)
            {
            // maximize/minimize effect
            if( (SIZE_MAXIMIZED == wParam) ) 
            {
                // resize for new max/min size
                mCPUT->ResizeWindow(width,height);
                mbMaxMinFullScreen = true;
            }
            else if(SIZE_RESTORED == wParam)
            {
                if(true == mbMaxMinFullScreen)
                {
                    // resize for new max/min size
                    mCPUT->ResizeWindow(width,height);
                    mbMaxMinFullScreen = false;
                }
                else
                {
                        mCPUT->ResizeWindow(width, height);
                    }
                }
            }
            else
            {
                    mCPUT->ResizeWindowSoft(width, height);
                }
            }
        break;
    
    case WM_EXITSIZEMOVE:
        sizing = false;
        // update the system's size and make callback
        if(mCPUT)
        {
            RECT windowRect;
            if(0==GetClientRect(hWnd, &windowRect)) // this gets the client area inside the window frame *excluding* frames/menu bar/etc
                break;

            width = windowRect.right - windowRect.left;
            height = windowRect.bottom - windowRect.top;

            // if we have shrunk to 0 width/height - do not pass on this kind of resize - leads to 
            // various render target resizing warnings
            if(0==width || 0==height)
            {
                break;
            }
            mCPUT->ResizeWindow(width,height);
        }
        break;


    case WM_DESTROY:
        // time to shut down the system
        PostQuitMessage(0);
        break;

    default:
        // we don't handle it - pass it on thru to parent
        res = DefWindowProc(hWnd, message, wParam, lParam);
        return res;
    }

    // translate handled code
    if(CPUT_EVENT_HANDLED == handledCode)
    {
        return 1;
    }

    return 0;
}





//
// Translates a Windows specific virtual key code to a CPUT key code
//
CPUTKey CPUTWindowWin::ConvertVirtualKeyToCPUTKey(WPARAM wParam)
{
	switch(wParam) {
	// numeric keys
	case 0x30:
		return KEY_0;
	case 0x31:
		return KEY_1;
	case 0x32:
		return KEY_2;
	case 0x33:
		return KEY_3;
	case 0x34:
		return KEY_4;
	case 0x35:
		return KEY_5;
	case 0x36:
		return KEY_6;
	case 0x37:
		return KEY_7;
	case 0x38:
		return KEY_8;
	case 0x39:
		return KEY_9;

	// letter keys
	case 0x41:
		return KEY_A;
	case 0x42:
		return KEY_B;
	case 0x43:
		return KEY_C;
	case 0x44:
		return KEY_D;
	case 0x45:
		return KEY_E;
	case 0x46:
		return KEY_F;
	case 0x47:
		return KEY_G;
	case 0x48:
		return KEY_H;
	case 0x49:
		return KEY_I;
	case 0x4A:
		return KEY_J;
	case 0x4B:
		return KEY_K;
	case 0x4C:
		return KEY_L;
	case 0x4D:
		return KEY_M;
	case 0x4E:
		return KEY_N;
	case 0x4F:
		return KEY_O;
	case 0x50:
		return KEY_P;
	case 0x51:
		return KEY_Q;
	case 0x52:
		return KEY_R;
	case 0x53:
		return KEY_S;
	case 0x54:
		return KEY_T;
	case 0x55:
		return KEY_U;
	case 0x56:
		return KEY_V;
	case 0x57:
		return KEY_W;
	case 0x58:
		return KEY_X;
	case 0x59:
		return KEY_Y;
	case 0x5A:
		return KEY_Z;

	// function keys
    case VK_F1:
        return KEY_F1;
    case VK_F2:
        return KEY_F2;
    case VK_F3:
        return KEY_F3;
    case VK_F4:
        return KEY_F4;
    case VK_F5:
        return KEY_F5;
    case VK_F6:
        return KEY_F6;
    case VK_F7:
        return KEY_F7;
    case VK_F8:
        return KEY_F8;
    case VK_F9:
        return KEY_F9;
    case VK_F10:
        return KEY_F10;
    case VK_F11:
        return KEY_F11;
    case VK_F12:
        return KEY_F12;

    // special keys
    case VK_HOME:
        return KEY_HOME;
    case VK_END:
        return KEY_END;
    case VK_PRIOR:
        return KEY_PAGEUP;
    case VK_NEXT:
        return KEY_PAGEDOWN;
    case VK_INSERT:
        return KEY_INSERT;
    case VK_DELETE:
        return KEY_DELETE;

    case VK_BACK:
        return KEY_BACKSPACE;
    case VK_TAB:
        return KEY_TAB;
    case VK_RETURN:
        return KEY_ENTER;

    case VK_PAUSE:
        return KEY_PAUSE;
    case VK_CAPITAL:
        return KEY_CAPSLOCK;
    case VK_ESCAPE:
        return KEY_ESCAPE;
	case VK_SHIFT:
		return KEY_SHIFT;

    case VK_UP:
        return KEY_UP;
    case VK_DOWN:
        return KEY_DOWN;
    case VK_LEFT:
        return KEY_LEFT;
    case VK_RIGHT:
        return KEY_RIGHT;
	}

	return KEY_NONE;
}

//
// Translates a character to a CPUT key code.
//
CPUTKey CPUTWindowWin::ConvertCharacterToCPUTKey(WPARAM wParam)
{
    switch(wParam)
    {
    case 'a':
    case 'A':
        return KEY_A;
    case 'b':
    case 'B':
        return KEY_B;
    case 'c':
    case 'C':
        return KEY_C;
    case 'd':
    case 'D':
        return KEY_D;
    case 'e':
    case 'E':
        return KEY_E;
    case 'f':
    case 'F':
        return KEY_F;
    case 'g':
    case 'G':
        return KEY_G;
    case 'h':
    case 'H':
        return KEY_H;
    case 'i':
    case 'I':
        return KEY_I;
    case 'j':
    case 'J':
        return KEY_J;
    case 'k':
    case 'K':
        return KEY_K;
    case 'l':
    case 'L':
        return KEY_L;
    case 'm':
    case 'M':
        return KEY_M;
    case 'n':
    case 'N':
        return KEY_N;
    case 'o':
    case 'O':
        return KEY_O;
    case 'p':
    case 'P':
        return KEY_P;
    case 'Q':
    case 'q':
        return KEY_Q;
    case 'r':
    case 'R':
        return KEY_R;
    case 's':
    case 'S':
        return KEY_S;
    case 't':
    case 'T':
        return KEY_T;
    case 'u':
    case 'U':
        return KEY_U;
    case 'v':
    case 'V':
        return KEY_V;
    case 'w':
    case 'W':
        return KEY_W;
    case 'x':
    case 'X':
        return KEY_X;
    case 'y':
    case 'Y':
        return KEY_Y;
    case 'z':
    case 'Z':
        return KEY_Z;


        // number keys
    case '1':
        return KEY_1;
    case '2':
        return KEY_2;
    case '3':
        return KEY_3;
    case '4':
        return KEY_4;
    case '5':
        return KEY_5;
    case '6':
        return KEY_6;
    case '7':
        return KEY_7;
    case '8':
        return KEY_8;
    case '9':
        return KEY_9;
    case '0':
        return KEY_0;


    // symbols
    case ' ':
        return KEY_SPACE;
    case '`':
        return KEY_BACKQUOTE;
    case '~':
        return KEY_TILDE;
    case '!':
        return KEY_EXCLAMATION;
    case '@':
        return KEY_AT;
    case '#':
        return KEY_HASH;
    case '$':
        return KEY_$;
    case '%':
        return KEY_PERCENT;
    case '^':
        return KEY_CARROT;
    case '&':
        return KEY_ANDSIGN;
    case '*':
        return KEY_STAR;
    case '(':
        return KEY_OPENPAREN;
    case ')':
        return KEY_CLOSEPARN;
    case '_':
        return KEY__;
    case '-':
        return KEY_MINUS;
    case '+':
        return KEY_PLUS;
    case '[':
        return KEY_OPENBRACKET;
    case ']':
        return KEY_CLOSEBRACKET;
    case '{':
        return KEY_OPENBRACE;
    case '}':
        return KEY_CLOSEBRACE;
    case '\\':
        return KEY_BACKSLASH;
    case '|':
        return KEY_PIPE;
    case ';':
        return KEY_SEMICOLON;
    case ':':
        return KEY_COLON;
    case '\'':
        return KEY_SINGLEQUOTE;
    case '\"':
        return KEY_QUOTE;
    case ',':
        return KEY_COMMA;
    case '.':
        return KEY_PERIOD;
    case '/':
        return KEY_SLASH;
    case '<':
        return KEY_LESS;
    case '>':
        return KEY_GREATER;
    case '?':
        return KEY_QUESTION;
    }

    return KEY_NONE;
}



// Convert mouse state to CPUT state
//-----------------------------------------------------------------------------
CPUTMouseState CPUTWindowWin::ConvertMouseState(WPARAM wParam)
{
    CPUTMouseState eState=CPUT_MOUSE_NONE;

    if( wParam & MK_CONTROL)
        eState = (CPUTMouseState) (eState | static_cast<int>(CPUT_MOUSE_CTRL_DOWN));

    if( wParam & MK_SHIFT)
        eState = (CPUTMouseState) (eState | static_cast<int>(CPUT_MOUSE_SHIFT_DOWN));

    if( wParam & MK_LBUTTON)
        eState = (CPUTMouseState) (eState | static_cast<int>(CPUT_MOUSE_LEFT_DOWN));

    if( wParam & MK_MBUTTON)
        eState = (CPUTMouseState) (eState | static_cast<int>(CPUT_MOUSE_MIDDLE_DOWN));

    if( wParam & MK_RBUTTON)
        eState = (CPUTMouseState) (eState | static_cast<int>(CPUT_MOUSE_RIGHT_DOWN));


    return eState;
}

// Main message pump
//-----------------------------------------------------------------------------
int CPUTWindowWin::StartMessageLoop()
{
    // Clear message queue
MSG msg;
//for(;;)
//{
//    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
//    {
//        if (msg.message == WM_DESTROY)
//            return CPUT_ERROR;
//        TranslateMessage( &msg );
//        DispatchMessage( &msg );
    //
//    }
//}

    //
    // Message pump
    //
    bool fRunning = true;
    while(fRunning)
    {
        // PeekMessage() is a passthru on no events
        // so it allows us to render while no events are present
        if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
        {
            if (msg.message == WM_QUIT)
            {
                PostQuitMessage(0);
                fRunning = false;
            }
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        } else if (IsWindowMinimized()) {
            Sleep(100);
        } else
        {
            // trigger render and other calls
            mCPUT->InnerExecutionLoop();
        }
    }
    
    //
    // Drain out the rest of the message queue.
    //
    while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    if (UnregisterClass(mAppTitle.c_str(), mhInst) == 0) {
        HandleWin32Error();
    }

    //
    // Set the window handle to NULL to indicate window shutdown is complete
    //
    mhWnd = NULL;

    // return code
    mAppClosedReturnCode =  (int) msg.wParam;
    return mAppClosedReturnCode;
}
