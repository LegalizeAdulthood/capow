/************************************************************************
    FILE:               capow.cpp
    PROJECT:            CAMCOS CAPOW!
*/

//***********************************************************************/
//====================INCLUDES===============

#include "Capow.hpp"

#if defined(CAPOW_ENABLE_ALPAKA)
#include "AlpakaBackend.hpp"
#endif
// These first two headers are needed for Randomize()
#include "BatchOptions.hpp"
#include "BatchRunner.hpp"
#include "ca.hpp"
#include "resource.h"
#include "Random.h"
#include <commdlg.h>
#include <string.h>
#include <commctrl.h>// For the 32 bit Toolbar control
#include "Userpara.hpp"
#include <stdio.h>
#include "status.hpp"
#include "GUI.hpp"
#include "Comcthlp.h"
#include "CapowGL.hpp"
//Note that you need COMCTL32.LIB in link library list for this.
//Need this header for HTMLHelp
#include "htmlhelp.h"

//==================== FLAGS ===============

//#define TOOL_IN_CAPOW
//#define LOAD_ACTIVE_CAS
//#define FIXED_640_480
//#define MASTERTIMER
/* In Windows95 and WindowsNT, MASTERTIMER works fine.  But in Win98, using it
slows my performance waaaay down! The catch is that if I don't use this,
I lose my speed control. So what I did was to add a handmade timer element
to the PeekMessage loop, and I don't use MASTERTIMER.*/
//====================DEFINE CONSTANTS ===============

#define MAXFILENAME 256  // maximum length of file with pathname
#define PUT_TO_SLEEP 1 // Put CAs to sleep
#define WAKE_UP      0 // Wake CAs up
#define ALL          0
#define FOCUS        1

//Bugfix- the following two defines were swapped in their values, to
//fix the location of the popup menus on the action toolbar.  mike 1/98
#define VIEWMENU_BUTTON   4  // View menu is the thrid button on the toolbar
#define CATYPEMENU_BUTTON 3  // fourth button

#define SEEDMENU_BUTTON 5  // fourth button


//====================CAPTION===============

LPSTR versioncaption = "(";
LPSTR datecaption = "3/8/17) ";

char caption[256] = "";
#ifdef FORCENARROW
LPSTR typecaption = "BorderMaker CAPOW 2017 ";
#else //not FORCENARROW
    #ifdef LITE
    LPSTR typecaption = "CAPOW 2017 LITE! ";
    #else //not LITE
        #ifdef BIG2D
        LPSTR typecaption = "CAPOW 2017 ";
        #else //not BIG2D
            #ifdef BIGGER2D
            LPSTR typecaption = "Capow 2017 ";
            #else //not BIG2D
                #ifdef BIGGEST2D
                LPSTR typecaption = "CAPOW 2017, 600 by 300 ";
                #else //not BIG2D
                    LPSTR typecaption = "CAPOW 2017, Small 2D";
                #endif //BIG2D
            #endif //BIGGER2D
        #endif //BIGEST2D
    #endif //LITE
#endif //FORCENARROW
LPSTR standardcaption = "Continuous-Valued Cellular Automata.";

//====================GLOBAL VARIABLES===============

HINSTANCE hInst;            // Our Executable Instance

HWND masterhwnd    = NULL;  // Handle to Master Window
HWND hwndActionToolbar   = NULL;  // Handle to ToolBar
HWND hwndDialogToolbar   = NULL;  // Handle to ToolBar
HWND hwndStatusBar = NULL;  // Handle to StatusBar
HWND hDlgCycle = 0, hDlgExp = 0, hDlgColor = 0, //Handles to  dialog windows
    hDlgFourier = 0, hDlgAnalog = 0, hDlgCell = 0, hDlgElectric = 0,
    hDlgDigital = 0, hDlgView = 0, hDlgWorld = 0,
    hUserDialog = 0, hDlgGenerators = 0, hDlgOpenGL =0, hDlgConfigure = 0;

HMENU   hMainMenu;  // Handle to our Menu
HMENU   hViewMenu;      // Handle to view sub menu
HMENU   hCATypeMenu;      // Handle to view sub menu
HMENU   hSeedMenu;      // Handle to view sub menu

BOOL  zoomviewflag         =    FALSE;
BOOL  first_time_flag      = TRUE;
BOOL  not_seeded_yet_flag  = TRUE;
BOOL  update_flag          = FALSE; // Updates params, cycle, lookup dialog boxes
BOOL  load_save_cells_flag = FALSE;
BOOL  statusON             = TRUE;  // status bar is on
BOOL  toolbarON            = TRUE;  // toolbar is on  Now this is used to hold
    //ActionToolbar or DialogToolbar.
BOOL  windowIsMinimized     = FALSE;
BOOL  inloadsave = FALSE;
BOOL randomizenow = FALSE;
int divider_width = 1; //Defined in CAPOW.CPP and in CASCREEN.CPP
    //Width of the gray line dividers.
short focusflag            = START_FOCUSFLAG;  //Set in ca.hpp to ALL=0, or FOCUS=1
short WhichToolBar         = 0;    // 1 = NEW  0 = OLD
BOOL ActionToolbar         = 0;   // 0 means off   1 means on
BOOL DialogToolbar        = 1;   // o means off 1 means on
    char CA_STYLE_NAME[256]; //Used in several places to get the current rule name.
int filterflag = 1; //1 means start with .ca in the open file dialog box.

int update_timer_handle = 0; //This will actually be equal to UPDATE_TIMER_ID.
#define UPDATE_TIMER_OPTIONS_COUNT 5 //These are veryslow, slow, medium, fast, fastest
int update_timer_options[UPDATE_TIMER_OPTIONS_COUNT] = {300, 200, 100, 10, 1};
    // These are the times to wait between updates, in milliseconds
    //The difference between FAST and FASTEST is mainly the _blt_line setting,
    //but I pretend that 100 updates vs 1000 updates is a possibility.
int update_millisecs_per_cycle = 10; //msec, actually 50 is about as fast as Windows95 can do it,
    //But maybe Win98 is faster.   RR 2/26/99
int update_timer_speed_index = ID_FAST-ID_VERYSLOW;
    //Start with FAst, not with FAstest
/* I also need another timer for my autorandomizing (and I'll put the same variable
similar thing in CASCREEN.CPP for screen saver).  The randomize_timer_cycle is
a variable that I will safe and load as a profile string, I'll keep it in
AUTORAND.CPP so that the *.SCR and *.EXE can share it. */
int randomize_timer_handle = 0;
//2/16/99 Rudy added this stuff to use with QueryPerformanceCounter.
extern _int64 _start, _end, _freq, _update_ticks_per_cycle;
extern BOOL _performance_counter_present;

//====================GLOBAL DATA===============

CAlist *calife_list = NULL;

char commandline[1024] = { '\0' }; // Stores Commandline passed program
/* Usually you would call this variable szAppName, but we are planning to
make a screen saver version of this program, using a lot of the same code
modules (though the main will be CASCREEN.CPP instead of CAPOW.CPP), and
the screensaver SCRNSAVE.LIB has a static TCHAR szAppName[40]; whose definition
would conflict with my using szAppName in my modules.  So instead I use a
different name */
char *szMyAppName = "CAPOW"; //Don't call szAppName so don't conflict with scrnsave.h
char capowDirectory[256];
extern char userDialogName[];
extern char szScreenSaverFileName[];
extern char szScreenSaverFileShortName[];

LPSTR WinArgv[9]= { NULL };  // Holds all commands from the command line

int  cursormode      = CUR_PICK;        // Current cursor mode
int  oldcursormode   = CUR_PICK;
int  toolBarHeight   = TOOLBARHEIGHT ;  // Variable holding toolbar height
int  statusBarHeight = STATUSBARHEIGHT;  //Holds status bar height
int  nDrawMode       = R2_COPYPEN;
int  cxParent, cyParent;

CapowGL *capowgl;

#if defined(CAPOW_ENABLE_ALPAKA)
static void SyncAlpakaLiveDisplays()
{
    if (calife_list == NULL)
        return;

    for (int i = 0; i < calife_list->Count(); ++i)
        calife_list->GetCA(i)->MarkAlpakaHeat2DDirty();
}

static void UpdateBackendToolbarState()
{
    BOOL gpuAvailable = FALSE;
    BOOL gpuBackend = FALSE;
#if defined(CAPOW_ENABLE_ALPAKA)
    capow::AlpakaManager &backendManager = capow::GetAlpakaManager();
    gpuAvailable = backendManager.IsGpuAvailable() ? TRUE : FALSE;
    gpuBackend = backendManager.GetBackend() == capow::ALPAKA_BACKEND_GPU ? TRUE : FALSE;
#endif
    if (hwndActionToolbar != NULL)
    {
        ToolBar_EnableButton(hwndActionToolbar, IDM_BACKEND_TOGGLE, gpuAvailable);
        ToolBar_CheckButton(hwndActionToolbar, IDM_BACKEND_TOGGLE, gpuBackend);
    }
    if (hwndDialogToolbar != NULL)
    {
        ToolBar_EnableButton(hwndDialogToolbar, IDM_BACKEND_TOGGLE, gpuAvailable);
        ToolBar_CheckButton(hwndDialogToolbar, IDM_BACKEND_TOGGLE, gpuBackend);
    }
}

static bool IsLiveGpuRuleType(int type)
{
    return type == CA_HEAT_2D || type == CA_WAVE_2D ||
        type == CA_OSCILLATOR || type == CA_DIVERSE_OSCILLATOR ||
        type == ALT_CA_OSCILLATOR_WAVE ||
        type == ALT_CA_DIVERSE_OSCILLATOR_WAVE || type == CA_ULAM_WAVE ||
        type == ALT_CA_ULAM_WAVE || type == CA_AUTO_ULAM_WAVE ||
        type == CA_CUBIC_ULAM_WAVE;
}

static capow::AlpakaRule AlpakaRuleForCAType(int type)
{
    switch (type)
    {
    case CA_WAVE_2D:
        return capow::ALPAKA_RULE_CA_WAVE_2D;
    case CA_OSCILLATOR:
        return capow::ALPAKA_RULE_CA_OSCILLATOR;
    case CA_DIVERSE_OSCILLATOR:
        return capow::ALPAKA_RULE_CA_DIVERSE_OSCILLATOR;
    case ALT_CA_OSCILLATOR_WAVE:
        return capow::ALPAKA_RULE_ALT_CA_OSCILLATOR_WAVE;
    case ALT_CA_DIVERSE_OSCILLATOR_WAVE:
        return capow::ALPAKA_RULE_ALT_CA_DIVERSE_OSCILLATOR_WAVE;
    case CA_ULAM_WAVE:
    case ALT_CA_ULAM_WAVE:
        return capow::ALPAKA_RULE_CA_ULAM_WAVE;
    case CA_AUTO_ULAM_WAVE:
        return capow::ALPAKA_RULE_CA_AUTO_ULAM_WAVE;
    case CA_CUBIC_ULAM_WAVE:
        return capow::ALPAKA_RULE_CA_CUBIC_ULAM_WAVE;
    default:
        return capow::ALPAKA_RULE_CA_HEAT_2D;
    }
}

static bool IsLiveGpuViewSupported(CA *focus)
{
    if (focus == nullptr)
        return false;

    const int type = focus->Gettype();
    if (type == CA_HEAT_2D || type == CA_WAVE_2D)
        return focus->Getviewmode() == IDC_2D_VIEW;
    return focus->Getviewmode() == IDC_DOWN_VIEW ||
        focus->Getviewmode() == IDC_SCROLL_VIEW;
}

static bool CanShowLiveGpu()
{
    if (capowgl == nullptr || calife_list == nullptr || !zoomviewflag)
        return false;

    CA *focus = calife_list->FocusCA();
    capow::AlpakaManager &backendManager = capow::GetAlpakaManager();
    return backendManager.GetBackend() == capow::ALPAKA_BACKEND_GPU && focus != nullptr &&
        IsLiveGpuRuleType(focus->Gettype()) && backendManager.CanRunGpu(AlpakaRuleForCAType(focus->Gettype())) &&
        IsLiveGpuViewSupported(focus);
}

static void UpdateAlpakaDisplayType()
{
    if (capowgl == NULL)
        return;

    if (CanShowLiveGpu())
    {
        if (capowgl->Type() != LIVE_GPU)
        {
            SyncAlpakaLiveDisplays();
            capowgl->Type(LIVE_GPU);
            InvalidateRect(masterhwnd, NULL, FALSE);
        }
        return;
    }

    if (capowgl->Type() == LIVE_GPU)
    {
        SyncAlpakaLiveDisplays();
        capowgl->Type(FLATCOLOR);
        InvalidateRect(masterhwnd, NULL, FALSE);
    }
}

#else
static void UpdateBackendToolbarState()
{
    if (hwndActionToolbar != NULL)
    {
        ToolBar_EnableButton(hwndActionToolbar, IDM_BACKEND_TOGGLE, FALSE);
        ToolBar_CheckButton(hwndActionToolbar, IDM_BACKEND_TOGGLE, FALSE);
    }
    if (hwndDialogToolbar != NULL)
    {
        ToolBar_EnableButton(hwndDialogToolbar, IDM_BACKEND_TOGGLE, FALSE);
        ToolBar_CheckButton(hwndDialogToolbar, IDM_BACKEND_TOGGLE, FALSE);
    }
}
#endif


/* Here is a flag I use in CONFIGURE.CPP to decide whether that dialog's code
is for the dialog of the *.EXE or for the initializer of the *.SCR */
int buildtype = BUILD_EXE;

//====================LOCAL FUNCTIONS ===============
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void Cellmain(HWND); //This is the continually running thing.

void ParseCommandLine ( char commandline[], char* WinArgv[] );
void GrabExtension ( LPSTR lpszCmdParam, char Extension[] );
BOOL CheckExtension ( char Extension[], char DesiredExtension[] );


//====================EXTERNAL DATA===============

extern DWORD dwStatusBarStyles;
extern BOOL compressFile;
//The following are from Autorand.cpp, they are shared with this project and with the
//Cascreen project.
extern UINT fRandFlags;
extern int randomize_timer_cycle; // defualt is 120000 for 2 minutes.

//====================EXTERNAL FUNCTIONS===============

extern BOOL CALLBACK CycleProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK AboutProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK ExpProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK ColorProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK FourierProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK AnalogProc( HWND, UINT, WPARAM, LPARAM );
extern BOOL CALLBACK ElectricProc( HWND, UINT, WPARAM, LPARAM );
extern BOOL CALLBACK CellProc( HWND, UINT, WPARAM, LPARAM );
extern BOOL CALLBACK DigitalProc( HWND, UINT, WPARAM, LPARAM );
extern BOOL CALLBACK WorldProc( HWND, UINT, WPARAM, LPARAM );
extern BOOL CALLBACK ViewProc( HWND, UINT, WPARAM, LPARAM );

extern BOOL CALLBACK GeneratorsProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK OpenGLProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK SaveFileProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK ConfigureProc(HWND , UINT , WPARAM,  LPARAM );

extern LRESULT CALLBACK userDialogProc( HWND, UINT, WPARAM, LPARAM );

extern LRESULT ToolBarNotify(HWND hwnd, int idForm, NMHDR  * pnmhdr);

extern void createUserDialog();
extern HWND RebuildToolBar (HWND hwndParent, WORD wFlag);
//Keep this in Autorand.cpp so that the capow.scr project can use it too.
extern void setTimerCycle(HWND hwnd, int &timer_handle, int timer_ID, int millisecs);
extern void setPerformanceTimerCycle(int millisecs);



//====================WIN MAIN===============
// Windows Overhead


int WINAPI CapowWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpszCmdParam, int nCmdShow)
{
    HACCEL hAccel;
    MSG msg;
    WNDCLASSA wndclass;
    const capow::BatchParseResult batchParse =
        capow::ParseBatchCommandLine(lpszCmdParam);
    if (batchParse.batch && !batchParse.ok)
    {
        OutputDebugStringA(batchParse.error.c_str());
        OutputDebugStringA("\n");
        return 2;
    }
#if defined(CAPOW_ENABLE_ALPAKA)
    OutputDebugStringA(capow::GetAlpakaManager().GetAvailabilityMessage());
    OutputDebugStringA("\n");
#endif
    if (!batchParse.batch)
    {
        strcpy ( commandline, lpszCmdParam );
        ParseCommandLine ( commandline, WinArgv ); // ~ Copies Command line to a global char array
    }
    if (!hPrevInstance)
    {
        wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wndclass.lpfnWndProc = WndProc;
        wndclass.cbClsExtra = 0;
        wndclass.cbWndExtra = 0;
        wndclass.hInstance = hInstance;
        wndclass.hIcon = LoadIconA( hInstance, szMyAppName );
        wndclass.hCursor = LoadCursor( NULL, IDC_ARROW );
        wndclass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
        wndclass.lpszMenuName = szMyAppName;
        wndclass.lpszClassName = szMyAppName;

        RegisterClassA (&wndclass);

        wndclass.style = CS_HREDRAW | CS_VREDRAW  | CS_DBLCLKS;
        wndclass.lpfnWndProc = userDialogProc;
        wndclass.cbClsExtra = 0;
        wndclass.cbWndExtra = 0;
        wndclass.hInstance = hInstance;
        wndclass.hIcon = LoadIconA( hInstance, szMyAppName );
        wndclass.hCursor = LoadCursor( NULL, IDC_ARROW );
        wndclass.hbrBackground = (HBRUSH)GetStockObject( LTGRAY_BRUSH );
        wndclass.lpszMenuName = userDialogName;
        wndclass.lpszClassName = userDialogName;

        RegisterClassA (&wndclass);

    }

    hInst = hInstance;  // Make Copy of program instance

    lstrcatA(caption, typecaption);
    lstrcatA(caption, versioncaption);
    lstrcatA(caption, datecaption);
    lstrcatA(caption, standardcaption);
    GetCurrentDirectoryA(256, capowDirectory);
    WriteProfileStringA(szMyAppName, "Directory", capowDirectory);
    strcpy(szScreenSaverFileName, capowDirectory);
    strcat(szScreenSaverFileName, "\\Files To Open\\");
    strcat(szScreenSaverFileName, szScreenSaverFileShortName);

    masterhwnd = CreateWindowA(szMyAppName,      // window class name
         caption,
            WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,        // window style
                                    //CLIPCHILDREN and CLIPSIBLINGS are for the sake of opengl
            CW_USEDEFAULT,              // initial x position
            CW_USEDEFAULT,              // initial y position
        INITIAL_XSIZE,              // initial x size
        INITIAL_YSIZE,              // initial y size
            NULL,                   // parent window handle
            NULL,                   // window menu handle
            hInstance,              // program instance handle
            NULL);                  // creation parameters
         //If you get an error message here, it is because you are
         //doing a 32 bit compile and you need to comment out
         // the caption switch stuff just above.

    ShowWindow(masterhwnd, batchParse.batch ? SW_HIDE : nCmdShow);
    UpdateWindow(masterhwnd);

    if (batchParse.batch)
    {
        const int batchExit = capow::RunBatchMode(batchParse.options);
        DestroyWindow(masterhwnd);
        return batchExit;
    }

    hAccel = LoadAcceleratorsA ( hInstance, "Capow_Accelerators" );


    while (TRUE) //Keep it growing.  See Petzold's RANDRECT example.
    {
        if ( PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            if (!(
                    (hDlgColor && IsDialogMessageA(hDlgColor, &msg)) ||
                    (hDlgCycle && IsDialogMessageA(hDlgCycle, &msg)) ||
                    (hDlgExp && IsDialogMessageA(hDlgExp, &msg)) ||
                    (hDlgAnalog && IsDialogMessageA( hDlgAnalog, &msg)) ||
                    (hDlgElectric && IsDialogMessageA( hDlgElectric, &msg)) ||
                    (hDlgDigital && IsDialogMessageA( hDlgDigital, &msg)) ||
                    (hDlgView && IsDialogMessageA( hDlgView, &msg)) ||
                    (hDlgWorld && IsDialogMessageA( hDlgWorld, &msg)) ||
                    (hDlgConfigure && IsDialogMessageA( hDlgConfigure, &msg)) ||
                    (hDlgCell && IsDialogMessageA( hDlgCell, &msg )) ||
                    (hDlgFourier && IsDialogMessageA(hDlgFourier, &msg)) ||
                    (hDlgGenerators && IsDialogMessageA(hDlgGenerators, &msg)) ||
                    (hDlgOpenGL && IsDialogMessageA(hDlgOpenGL, &msg))
                ) )
                {
                    if (msg.message == WM_QUIT)
                        break;

                    if ( !TranslateAcceleratorA ( masterhwnd, hAccel, &msg ) )
                    {
                        TranslateMessage(&msg);
                        DispatchMessageA(&msg);
                    }
                }
        }
#ifndef MASTERTIMER
        else
        {
            QueryPerformanceCounter((LARGE_INTEGER*)&_end);
            if ((_end - _start) >= _update_ticks_per_cycle)
            /* If you set update_ticks_per_cycle unrealistically low, then you are going
            to spend so much time in here that your program will be unresponsive.
            And don't be greedy and try and work a "while" instead of an "if" to
            do multiple updates here. Typical values of start and end are in the
            trillions, or higher; it's counting the total machine cycles
            during the program run. A typical value for end-start running on a
            400 MHz machine is end-start = 31,366,904 ticks.  Note that on this
            machine, we can use QueryPerformanceFrequency to find that the machine
            is running at 400,090,000 ticks per second.  In our setTimerCycle
            function in AUTORAND.CPP we use  QueryPerformanceCounter to set the
            update_ticks_per_cycle on the basis of the update_millisecs_per_cycle,
             on our 400 MHz test machine, we get values like this:
            update_millisecs_per_cycle  update_ticks_per_cycle
            10 msec                     4,009,000 ticks
            100 msec                    40,090,000 ticks
             */
            {
                Cellmain(masterhwnd);
                _start = _end;
            }
        }
#endif //MASTERTIMER
    }
    return msg.wParam;

}

//====================MESSAGE CRACKERS ===============
/* The message handlers we use are, in this order:
MyWnd_CREATE
MyWnd_PAINT
MyWnd_SIZE
MyWnd_MOVE
MyWnd_COMMAND
MyWnd_MENUSELECT
MyWnd_LBUTTONDOWN
MyWnd_RBUTTONDOWN
MyWnd_CLOSE
MyWnd_DESTROY
MyWnd_INITDIALOG
MyWnd_MOUSEMOVE
MyWnd_LBUTTONUP
MyWnd_INITMENUPOPUP
MyWnd_TIMER
/*********************************************************/

BOOL MyWnd_CREATE(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    Randomize();  // Seed the randomizer
    //When debugging, comment this line out so that each run is the same,
    //For release comment it in so that runs are pleasingly surprising.

    char Extension[5] = { '\0' };

    capowgl = new CapowGL(hwnd);
    capowgl->Size(hwnd);

    calife_list = new CAlist(hwnd, MAX_CAS); //Calls CA:Allocate for members

    hViewMenu   = LoadMenuA ( hInst, "ViewPopMenu" );
    hCATypeMenu = LoadMenuA ( hInst, "CATYPEPOPMENU" );
    hSeedMenu   = LoadMenuA ( hInst, "SEEDPOPMENU" );
    hViewMenu   = GetSubMenu ( hViewMenu, 0 );
    hCATypeMenu = GetSubMenu ( hCATypeMenu, 0 );
    hSeedMenu   = GetSubMenu ( hSeedMenu, 0 );


//=============== Loading Previously Saved Experiment =====================

#ifdef LOAD_ACTIVE_CAS

    if (calife_list->Loadall("ACTIVE.CAS", TRUE))
    //TRUE means startup, means don't send a WM_SIZE
    {
        if (calife_list->Get_justloadedcells())
            not_seeded_yet_flag = 0; //Don't Seed it in WM_SIZE
        else
            not_seeded_yet_flag = 1; // seed in   WM_SIZE

        MessageBoxA( hwnd,
                    (LPSTR)"(If you ever crash, delete ACTIVE.CAS.)",
                    (LPSTR)"Good! ACTIVE.CAS Has Loaded Successfully.",
                    MB_OK | MB_ICONEXCLAMATION );
    }
#endif //LOAD_ACTIVE_CAS

//=============== Intialize CommonControls =====================

    hwndStatusBar = InitStatusBar ( hwnd );  // Loads Status Bar
    hwndActionToolbar   = InitActionToolBar   ( hwnd );  // Loads Tool Bar
    hwndDialogToolbar   = InitDialogToolBar   ( hwnd );  // Loads Tool Bar
    UpdateBackendToolbarState();
    if (toolbarON)
        ShowWindow (hwndDialogToolbar, SW_SHOW);

    setPerformanceTimerCycle(update_millisecs_per_cycle);
    calife_list->FocusCA()->GetCAStyleName ( CA_STYLE_NAME );
    Status_SetText(hwndStatusBar, 1, 0, CA_STYLE_NAME );
//put hwndActionToolbar here if you'd rather start with that
    if ( !hwndStatusBar | !hwndActionToolbar | !hwndDialogToolbar )
        return FALSE;
    return TRUE;
}

/*********************************************************/

static void MyWnd_PAINT(HWND hwnd)
{   //((fn)(hwnd), 0L)

    PAINTSTRUCT ps;
    HDC         hdc = BeginPaint (hwnd, &ps) ;

    calife_list->Show(hdc, ps.rcPaint);

    EndPaint (hwnd, &ps) ;
}

/*********************************************************/


static void MyWnd_SIZE(HWND hwnd, UINT state, int cx, int cy)
{
    //((fn)((hwnd), (UINT)(wParam), (int)LOWORD(lParam), (int)HIWORD(lParam)), 0L)
    RECT rect;
    RECT rWindow;

    windowIsMinimized = (state==SIZE_MINIMIZED);
    if (windowIsMinimized)
        return;

    GetClientRect(hwnd, &rect);


#ifdef FIXED_640_480
//rect has left and top fields 0, so width, height are right, bottom

    RECT scr;
    int  framepixels, width, height;
#ifdef FORCENARROW
    const int maxClientWidth = FORCEXSIZE;
    const int maxClientHeight = FORCEYSIZE;
#else //not FORCENARROW
    const int maxClientWidth = 640;
    const int maxClientHeight = 480;
#endif //FORCENARROW
    if (rect.right > maxClientWidth || rect.bottom > maxClientHeight)
    {   //Correct one or both measurements of the window.
        GetWindowRect(hwnd, &scr);
        width = scr.right - scr.left; //window width
        height = scr.bottom - scr.top;  //window height
        if (rect.right > maxClientWidth)
        {
            framepixels = width - rect.right;
            //window width - client width
            width = maxClientWidth + framepixels;
        }
        if (rect.bottom > maxClientHeight)
        {
            framepixels = height - rect.bottom; //window height - client height
            height = maxClientHeight + framepixels;
        }
        //Then resize window to the correct rect.

        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, width, height,
                    SWP_NOMOVE); //Last param means only change size.
        GetClientRect(hwnd, &rect); //Reset the rect.
    }
#endif //FIXED_640_480


    calife_list->Locate();

// The IDM_CLEAR erases the bitmap and the screen and draws the
// focus box on both of them.

//if paused, zoomed, and focus is 2D, then don't clear the bitmap
//because we need the colors that are stored on it.
//otherwise, clear it. mike 11-1-97
    if (!(calife_list->GetSleep() && zoomviewflag && calife_list->Focus()->Getdimension()==2))
        SendMessage(hwnd, WM_COMMAND, IDM_CLEAR, 0L);

// Open controls button bar

    if (not_seeded_yet_flag) //need to have set size to FourierSeed
    { //You didn't find "ACTIVE.CAS", or ACTIVE.CAS didn't store cells
        calife_list->FourierSeed();
        not_seeded_yet_flag = FALSE;
    }


// Adjust status bar size.
    if (hwndStatusBar)
    {
        GetWindowRect (hwndStatusBar, &rWindow) ;
        statusBarHeight = rWindow.bottom - rWindow.top ;
        MoveWindow (hwndStatusBar, 0, cy - statusBarHeight,
                                              cx, statusBarHeight, TRUE) ;
    }

        SendMessage(hwndActionToolbar, WM_SIZE, state, MAKELONG(cx, cy));
        SendMessage(hwndDialogToolbar, WM_SIZE, state, MAKELONG(cx, cy));

    //adjust viewport for opengl
        capowgl->Size(hwnd);

}
/*********************************************************/

static void MyWnd_MOVE(HWND hwnd, int x, int y)
{
    //((fn)((hwnd), (int)LOWORD(lParam), (int)HIWORD(lParam)), 0L)
}

/*********************************************************/

static void TrackToolbarButtonMenu(HWND window, HWND toolbar, int buttonId, HMENU menu)
{
    RECT r1;
    ToolBar_GetItemRect(toolbar, buttonId, &r1);
    POINT point;
    point.x = r1.left;
    point.y = r1.bottom;
    ClientToScreen(window, &point);
    TrackPopupMenu(menu, 0, point.x, point.y, 0, window, NULL);
}

static void MyWnd_COMMAND(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    //((fn)((hwnd), (int)(wParam), (HWND)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)
    HGLOBAL hDib;
    BOOL clipboardSet;
    RECT rect;
    RECT CaptureRect; // Rect to Capture Screen
    RECT r;
    char buffer[20];
    char szFileName[MAXFILENAME];
    char szFileTitle[MAXFILENAME];
    OPENFILENAMEA ofn;
    char szFilter [128] =
        "Experiment File (*.CAs)\0*.CAs\0CA Files (*.CA)\0*.CA\0User Rules (*.dll)\0*.dll\0 All Files (*.*)\0*.*\0";
//End commdlg stuff=======================

// Message Processing

    switch(id)
    {

// START FILE MENU====================================
        case IDM_VIEW_MENU:
            TrackToolbarButtonMenu(hwnd, hwndActionToolbar, VIEWMENU_BUTTON, hViewMenu);
        break;

        case IDM_CATYPE_MENU:
            TrackToolbarButtonMenu(hwnd, hwndActionToolbar, CATYPEMENU_BUTTON, hCATypeMenu);
        break;

        case IDM_SEED_MENU:
            TrackToolbarButtonMenu(hwnd, hwndActionToolbar, SEEDMENU_BUTTON, hSeedMenu);
        break;

        case IDM_BACKEND_TOGGLE:
#if defined(CAPOW_ENABLE_ALPAKA)
        {
            capow::AlpakaManager &backendManager = capow::GetAlpakaManager();
            if (backendManager.GetBackend() == capow::ALPAKA_BACKEND_GPU)
            {
                SyncAlpakaLiveDisplays();
                backendManager.SetBackend(capow::ALPAKA_BACKEND_CPU);
                if (capowgl != NULL && capowgl->Type() == LIVE_GPU)
                {
                    capowgl->Type(FLATCOLOR);
                    InvalidateRect(masterhwnd, NULL, FALSE);
                }
            }
            else if (backendManager.IsGpuAvailable())
            {
                backendManager.SetBackend(capow::ALPAKA_BACKEND_GPU);
                UpdateAlpakaDisplayType();
            }
            else
            {
                MessageBoxA(hwnd, backendManager.GetAvailabilityMessage(), "GPU Backend", MB_OK | MB_ICONINFORMATION);
            }
            UpdateBackendToolbarState();
            break;
        }
#else
            MessageBoxA(hwnd, "GPU backend is not compiled.", "GPU Backend",
                MB_OK | MB_ICONINFORMATION);
            break;
#endif

        // For View Drop down Menu... Call ViewProc to handle it
        // Call to ViewProc used to avoid duplicate code
        case RADIO_DOWN_VIEW:
        case RADIO_SCROLL_VIEW:
        case RADIO_WIRE_VIEW:
        case RADIO_GRAPH_VIEW:
        case RADIO_SPLIT_VIEW:
        case RADIO_POINT_VIEW:
            ViewProc( hDlgView, WM_COMMAND, id, 0L );
#if defined(CAPOW_ENABLE_ALPAKA)
            UpdateAlpakaDisplayType();
#endif
            break;

        // For CA Drop down Menu...
        case CA_STANDARD:
        case CA_REVERSIBLE:
        case CA_HEATWAVE:
        case CA_HEATWAVE2:
        case ALT_CA_WAVE:
        case CA_WAVE2:
        case CA_OSCILLATOR:
        case CA_DIVERSE_OSCILLATOR:
        case ALT_CA_OSCILLATOR_WAVE:
        case ALT_CA_DIVERSE_OSCILLATOR_WAVE:
        case ALT_CA_ULAM_WAVE:
        case CA_CUBIC_ULAM_WAVE:
        case CA_AUTO_ULAM_WAVE:
        case CA_WAVE_2D:
        case CA_HEAT_2D:
            if (focusflag)
                calife_list->SetCAType(calife_list->FocusCA(), id,TRUE);
            else // Second argument says adjust for the rule to be stable.
                calife_list->SetAllType(id, TRUE);
            capowgl->AdjustHeightFactor(calife_list->FocusCA());
#if defined(CAPOW_ENABLE_ALPAKA)
            UpdateAlpakaDisplayType();
#endif
            if (hDlgOpenGL)
                InvalidateRect(hDlgOpenGL, NULL, TRUE);
        break;

        // For Seed Drop down Menu... Call WorldProc to handle it
        // Call to WorldProc used to avoid duplicate code
        case IDC_ONESEED:
        case IDC_ZEROSEED:
        case IDC_SMOOTH:
        case IDC_SINESEED:
        case IDC_RANDOMSEED:
        case IDC_FOURIERSEED:
        case IDC_RANDOMTOUCH:
        case IDC_SEED_HALFMAX:
            WorldProc( 0, WM_COMMAND, id, 0L);
            break;

        case IDM_SWAP:
            if ( ActionToolbar == 1 )
                SendMessage ( masterhwnd, WM_COMMAND, IDM_OLDTOOLBAR, 0L );
            else
                if ( DialogToolbar == 1 )
                    SendMessage ( masterhwnd, WM_COMMAND, IDM_NEWTOOLBAR, 0L );
            break;

    // Open and Save Commands
        case IDM_LOADRULEFOCUS:
            {
            SetCursor(LoadCursor(NULL, IDC_WAIT)); // Wait, I'm working!
            short temp = 1;
            if (!calife_list->LoadUserRule(masterhwnd, temp))
                break;
            recreateUserDialog();
            calife_list->SetCAType(calife_list->FocusCA(), CA_USER, TRUE);
            update_flag = 1;
            /* Redraws lookup dialog menu */
            if (hDlgWorld)
                SendMessage(hDlgWorld, WM_COMMAND, SC_UPDATE, 0L);
            SetCursor(LoadCursor(NULL, IDC_ARROW)); //I'm done!
            break;
            }
        case IDM_LOADRULEALL:
          {
            short temp = 0;
            SetCursor(LoadCursor(NULL, IDC_WAIT)); // Wait, I'm working!
            if (!calife_list->LoadUserRule(masterhwnd, temp))
                break;
            recreateUserDialog();
            calife_list->SetAllType(CA_USER, TRUE);

            update_flag = 1;
            /* Redraws lookup dialog menu */
            if (hDlgWorld)
                SendMessage(hDlgWorld, WM_COMMAND, SC_UPDATE, 0L);
            SetCursor(LoadCursor(NULL, IDC_ARROW)); //I'm done!
            break;
          }

        case IDM_FILE_SAVE:   //Opens Modal Save Dialog
            inloadsave = TRUE;   //Stop running while you get ready to save
            DialogBoxA(hInst, "SAVE", hwnd, (DLGPROC)SaveFileProc);
            inloadsave = FALSE;  //Go back
            break;

        case IDM_OPEN:
            // fill in non-variant fields of OPENFILENAMEA struct.
            ofn.lStructSize       = sizeof(OPENFILENAMEA);
            ofn.hwndOwner         = hwnd;
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter    = 0;
            ofn.nFilterIndex      = 1;
            ofn.nMaxFile          = MAXFILENAME;
            ofn.lpstrInitialDir   = NULL;
            ofn.lpstrFileTitle    = szFileTitle;
            ofn.nMaxFileTitle     = MAXFILENAME;
            ofn.lpstrTitle        = NULL;
            ofn.Flags             = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;


            strcpy(szFileName, "");
            ofn.nFilterIndex = filterflag;
            ofn.lpstrFile = szFileName;

            ofn.lpstrFilter = szFilter;
            ofn.lpstrDefExt   = "CAS";
            inloadsave = TRUE; //Don't do updates while you're in here
            if( GetOpenFileNameA((LPOPENFILENAMEA)&ofn) ){
                char* str2 = strupr(ofn.lpstrFileTitle);
                SetCursor(LoadCursor(NULL, IDC_WAIT)); // Wait, I'm working!

                if(strstr(str2, ".CAS"))
                {
                    filterflag = 1;
                    calife_list->Loadall(szFileName);

                }
                else if(strstr(str2, ".CA"))
                {
                    filterflag = 2;
                    if (focusflag)
                        calife_list->Load_Individual(szFileName, calife_list->FocusCA());
                    else
                        calife_list->Loadall_Individual(szFileName);
                }
                else if(strstr(str2, ".DLL"))
                {
                    filterflag = 3;
                    calife_list->LoadUserRule(masterhwnd, szFileName, focusflag);

                    update_flag = 1;
                    /* Redraws lookup dialog menu */
                    if (hDlgWorld)
                        SendMessage(hDlgWorld, WM_COMMAND, SC_UPDATE, 0L);
                }
                else
                {
                    MessageBoxA(hwnd, "Please select again", "Invalid Format", MB_OK);
                    SendMessage(hwnd, WM_COMMAND, IDM_OPEN, 0);
                }
            SetCursor(LoadCursor(NULL, IDC_ARROW)); //I'm done!
            }
            inloadsave = FALSE; //Go back to updating.
            update_flag = TRUE;
            break;

        case IDM_RANDOMIZE:                         // Randomize the CAs
            calife_list->Randomize();
            break;
        case ID_VERYSLOW:
        case ID_SLOW:
        case ID_MEDIUM:
        case ID_FAST:
        case ID_FASTEST:
            int new_blt_lines;
            if (id == ID_FASTEST)
                new_blt_lines = 3;
            else
                new_blt_lines = 1;
            calife_list->set_blt_lines(new_blt_lines);
            update_timer_speed_index = id - ID_VERYSLOW;
            CLAMP(update_timer_speed_index, 0, UPDATE_TIMER_OPTIONS_COUNT-1);
            update_millisecs_per_cycle = update_timer_options[update_timer_speed_index];
            setPerformanceTimerCycle(update_millisecs_per_cycle);
        /*When MASTERTIMER is on, setTimerCycle changes the purpose of the update_timer_handle
        timer.  When MASTERTIMER is off, setTimerCycle instead changes the
        update_ticks_per_cycle variable used in the PeekMessage loop. */
            break;

        case IDM_PAUSE:                             // Puts all CAs to Sleep
            Status_GetText(hwndStatusBar, 0, buffer);
            if ( !strcmp ( buffer , "Ready" ) )
                Status_SetText(hwndStatusBar, 0, 0,"Paused");
            else
                Status_SetText(hwndStatusBar, 0, 0,"Ready");
            calife_list->ToggleSleep();
            capowgl->FocusIsActive(calife_list->GetSleep());
            break;

        case IDM_CLEAR:                             // Clears all CAs
            calife_list->ClearDisplayImages();
            calife_list->ResetAllGenerationCount();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case IDM_EXIT:                              // Exit Program
            SendMessage(hwnd,WM_CLOSE,0,0L);
            break;


// END FILE MENU====================================
// START EDIT MENU====================================

        case IDM_CAPTURE:       // Captures Client Rect to Clipboard

            hDib = 0;
            if (calife_list->GetOpenGLDisplayRect(&CaptureRect))
                hDib = capowgl->CaptureBackBufferDIB(hwnd, calife_list, CaptureRect);

            clipboardSet = FALSE;
            if (hDib != 0 && OpenClipboard ( masterhwnd ))
            {
                EmptyClipboard();
                clipboardSet = SetClipboardData ( CF_DIB, hDib ) != 0;
                CloseClipboard();
            }

            if (!clipboardSet && hDib != 0)
                GlobalFree(hDib);
            break;

// END EDIT MENU====================================
// START VIEW MENU====================================

        case IDM_SHOW_STATUS:               // Hide or Show Status Bar
            if (hwndStatusBar)
            {
                statusON = !IsWindowVisible(hwndStatusBar);  //flip value
                ShowWindow(hwndStatusBar, (statusON)? SW_SHOW:SW_HIDE);
            }

            // Resize other windows.
            calife_list->Locate();
            InvalidateRect(masterhwnd, NULL, FALSE);
            capowgl->Size(hwnd);
            break;

        case IDM_NEWTOOLBAR:
            ActionToolbar = !ActionToolbar;
            DialogToolbar = 0;
            toolbarON=ActionToolbar;
            ShowWindow(hwndActionToolbar, ((ActionToolbar && toolbarON)? SW_SHOW:SW_HIDE));
            ShowWindow(hwndDialogToolbar, ((DialogToolbar && toolbarON)?SW_SHOW:SW_HIDE));
            calife_list->Locate();
            capowgl->Size(hwnd);
            InvalidateRect(masterhwnd, NULL, FALSE);
            break;
        case IDM_OLDTOOLBAR:
            DialogToolbar = !DialogToolbar;
            ActionToolbar = 0;
            toolbarON = DialogToolbar;
            ShowWindow(hwndActionToolbar, ((ActionToolbar && toolbarON)? SW_SHOW:SW_HIDE));
            ShowWindow(hwndDialogToolbar, ((DialogToolbar && toolbarON)?SW_SHOW:SW_HIDE));
            calife_list->Locate();
            capowgl->Size(hwnd);
            InvalidateRect(masterhwnd, NULL, FALSE);
            break;
            // Both Dialogs are off.

// END VIEW MENU====================================
// START CONTROLS MENU and DIALOG CONTROLS =========

        case IDM_WORLD:         // Open World Dialog
            if( !hDlgWorld )
            {
                hDlgWorld = CreateDialogA( hInst, (LPCSTR)"WORLD", hwnd,
                                             (DLGPROC)WorldProc);
                GetWindowRect( hDlgWorld, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"WORLDX", 100 );
                rect.top  = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"WORLDY", 100 );
                if( GetSystemMetrics(SM_CXSCREEN) < rect.left-10 ) rect.left = 25;
                if( GetSystemMetrics(SM_CYSCREEN) < rect.top -10 ) rect.top = 25;
                MoveWindow( hDlgWorld, rect.left, rect.top, rect.right, rect.bottom,
                                FALSE);
                ShowWindow( hDlgWorld, TRUE );
            }
            else
                DestroyWindow( hDlgWorld );

            break;

        case IDM_CONFIGURE:         // ScreenSaver settings Dialog
            if( !hDlgConfigure )
            {
                hDlgConfigure = CreateDialogA( hInst, (LPCSTR)"CONFIGURE", hwnd,
                                             (DLGPROC)ConfigureProc);
                GetWindowRect( hDlgConfigure, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"CONFIGUREX", 100 );
                rect.top  = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"CONFIGUREY", 100 );
                if( GetSystemMetrics(SM_CXSCREEN) < rect.left-10 ) rect.left = 25;
                if( GetSystemMetrics(SM_CYSCREEN) < rect.top -10 ) rect.top = 25;
                MoveWindow( hDlgConfigure, rect.left, rect.top, rect.right, rect.bottom,
                                FALSE);
                ShowWindow( hDlgConfigure, TRUE );
            }
            else
                DestroyWindow( hDlgConfigure );

            break;


        case IDM_COLOR:         // Open Color Dialog
            if (!hDlgColor)
            {
                hDlgColor = CreateDialogA (hInst, (LPSTR)"COLOR", hwnd,
                                         (DLGPROC)ColorProc);
                GetWindowRect(hDlgColor, &rect);
                rect.bottom-=rect.top;
                rect.right-=rect.left;
                rect.left = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"COLORX",100);
                rect.top = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"COLORY",100);
                if (GetSystemMetrics(SM_CXSCREEN)<rect.left-10)
                    rect.left=25;
                if (GetSystemMetrics(SM_CYSCREEN)<rect.top-10)
                    rect.top=25;
                MoveWindow(hDlgColor, rect.left,rect.top,rect.right,rect.bottom,FALSE);
                ShowWindow(hDlgColor, TRUE);
            }
            else
                DestroyWindow(hDlgColor);

            break;



        case IDM_VIEW:          // Open View Dialog
            if(!hDlgView)
            {   hDlgView = CreateDialogA( hInst, (LPSTR)"VIEW", hwnd,
                               (DLGPROC)ViewProc );
                GetWindowRect( hDlgView, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"VIEWX",100);
                rect.top  = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"VIEWY",100);
                if (GetSystemMetrics(SM_CXSCREEN) < rect.left-10) rect.left = 25;
                if (GetSystemMetrics(SM_CYSCREEN) < rect.top -10) rect.top = 25;
                MoveWindow(hDlgView, rect.left, rect.top, rect.right, rect.bottom, FALSE);
                ShowWindow(hDlgView, TRUE);
            } // if
            else
                DestroyWindow(hDlgView);
            break;

        case IDM_DIGITAL:           // Open Digital Dialog
            if( !hDlgDigital )
            {
                hDlgDigital = CreateDialogA( hInst, (LPSTR)"DIGITAL", hwnd,
                                              (DLGPROC)DigitalProc );
                GetWindowRect( hDlgDigital, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"DIGITALX", 100 );
                rect.top  = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"DIGITALY", 100 );
                if( GetSystemMetrics(SM_CXSCREEN) < rect.left-10 ) rect.left = 25;
                if( GetSystemMetrics(SM_CYSCREEN) < rect.top -10 ) rect.top = 25;
                MoveWindow( hDlgDigital, rect.left, rect.top, rect.right, rect.bottom,
                            FALSE);
                ShowWindow( hDlgDigital, TRUE );
            } // if
            else
                DestroyWindow( hDlgDigital );
            break;


        case IDM_ANALOG:            // Open Analog Dialog
            if( !hDlgAnalog )
            {
                hDlgAnalog = CreateDialogA( hInst, (LPSTR)"ANALOG", hwnd,
                                         (DLGPROC)AnalogProc );
                GetWindowRect( hDlgAnalog, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"ANALOGX", 100 );
                rect.top  = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"ANALOGY", 100 );
                if( GetSystemMetrics(SM_CXSCREEN) < rect.left-10 ) rect.left = 25;
                if( GetSystemMetrics(SM_CYSCREEN) < rect.top -10 ) rect.top = 25;
                MoveWindow( hDlgAnalog, rect.left, rect.top, rect.right, rect.bottom,
                            FALSE);
                ShowWindow( hDlgAnalog, TRUE );
            } // if
            else
                DestroyWindow( hDlgAnalog );
            break;

        case IDM_ELECTRIC:          // Opens Electric Dialog
            if( !hDlgElectric )
            {
                hDlgElectric = CreateDialogA( hInst, (LPSTR)"ELECTRIC", hwnd,
                                           (DLGPROC)ElectricProc );
                GetWindowRect( hDlgElectric, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"ELECTRICX", 100 );
                rect.top  = (int)GetProfileIntA( (LPSTR)szMyAppName, (LPSTR)"ELECTRICY", 100 );
                if( GetSystemMetrics(SM_CXSCREEN) < rect.left-10 ) rect.left = 25;
                if( GetSystemMetrics(SM_CYSCREEN) < rect.top -10 ) rect.top = 25;
                MoveWindow( hDlgElectric, rect.left, rect.top, rect.right, rect.bottom,
                            FALSE);
                ShowWindow( hDlgElectric, TRUE );
            } // if
            else
                DestroyWindow( hDlgElectric );
            break;

        case IDM_FOURIER:           // Opens the Fourier Dialog
            if (!hDlgFourier)
            {
                hDlgFourier = CreateDialogA (hInst, (LPSTR)"FOURIER", hwnd,
                                           (DLGPROC)FourierProc);
                GetWindowRect(hDlgFourier, &rect);
                rect.bottom -=rect.top;
                rect.right  -=rect.left;
                rect.left    = (int)GetProfileIntA((LPSTR)szMyAppName,
                                     (LPSTR)"FOURIERX",100);
                rect.top     = (int)GetProfileIntA((LPSTR)szMyAppName,
                                         (LPSTR)"FOURIERY",100);
                if (GetSystemMetrics(SM_CXSCREEN)<rect.left-10)
                    rect.left=25;
                if (GetSystemMetrics(SM_CYSCREEN)<rect.top-10)
                    rect.top=25;
                MoveWindow(hDlgFourier, rect.left,rect.top,rect.right,
                    rect.bottom,FALSE);
                ShowWindow(hDlgFourier, TRUE);
            }
            else
                DestroyWindow(hDlgFourier);

            break;

        case IDM_USERDIALOG:    // Opens the User Dialog
            createUserDialog();
            break;


        case IDM_CYCLE:
            if (!hDlgCycle)
            {
                hDlgCycle = CreateDialogA (hInst, (LPSTR)"CYCLE", hwnd,
                                         (DLGPROC)CycleProc);
                GetWindowRect(hDlgCycle, &rect);
                rect.bottom-=rect.top;
                rect.right-=rect.left;
                rect.left = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"CYCLEX",100);
                rect.top = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"CYCLEY",100);
                if (GetSystemMetrics(SM_CXSCREEN)<rect.left-10)
                    rect.left=25;
                if (GetSystemMetrics(SM_CYSCREEN)<rect.top-10)
                    rect.top=25;
                MoveWindow(hDlgCycle, rect.left,rect.top,rect.right,rect.bottom,FALSE);
                ShowWindow(hDlgCycle, TRUE);
            }
            else
                DestroyWindow(hDlgCycle);
            break;

        case IDM_EXP:
            if (!hDlgExp)
            {
                hDlgExp = CreateDialogA (
                hInst, (LPSTR)"EXPERIMENT", hwnd, (DLGPROC)ExpProc);
                GetWindowRect(hDlgExp, &rect);
                rect.bottom-=rect.top;
                rect.right-=rect.left;
                rect.left = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"EXPERIMENTX",100);
                rect.top = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"EXPERIMENTY",100);
                if (GetSystemMetrics(SM_CXSCREEN)<rect.left-10)
                    rect.left=25;
                if (GetSystemMetrics(SM_CYSCREEN)<rect.top-10)
                    rect.top=25;
                MoveWindow(hDlgExp, rect.left,rect.top,rect.right,rect.bottom,FALSE);
                ShowWindow(hDlgExp, TRUE);
            }
            else
            {
                DestroyWindow(hDlgExp);
            }

            break;


        case IDM_CELL:
            if(!hDlgCell)
            {
                hDlgCell = CreateDialogA( hInst, (LPSTR)"CELL", hwnd,
                                           (DLGPROC)CellProc );
                GetWindowRect( hDlgCell, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"CELLX",100);
                rect.top  = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"CELLY",100);
                if (GetSystemMetrics(SM_CXSCREEN) < rect.left-10) rect.left = 25;
                if (GetSystemMetrics(SM_CYSCREEN) < rect.top -10) rect.top = 25;
                MoveWindow(hDlgCell, rect.left, rect.top, rect.right, rect.bottom, FALSE);
                ShowWindow(hDlgCell, TRUE);
            } // if
            else
                DestroyWindow(hDlgCell);
            break;

        case IDM_LOAD_USER_RULE:

            if (!calife_list->LoadUserRule(masterhwnd, focusflag))
                    break;

            recreateUserDialog();
            if (focusflag)
                calife_list->SetCAType(calife_list->FocusCA(), CA_USER,TRUE);
            else // Second argument says adjust for the rule to be stable.
                calife_list->SetAllType(CA_USER, TRUE);

            break;

        case IDM_CLOSE:
            // if any dialog box is open
            if (hDlgCycle || hDlgExp || hDlgColor || hDlgFourier ||
                hDlgAnalog ||   hDlgCell || hDlgElectric ||
                hDlgDigital || hDlgView || hDlgWorld || hDlgConfigure ||
                hUserDialog || hDlgGenerators || hDlgOpenGL )
            {
                if (hDlgGenerators)
                    DestroyWindow(hDlgGenerators );
                if (hDlgOpenGL)
                    DestroyWindow(hDlgOpenGL );
                if (hDlgCycle)
                    DestroyWindow(hDlgCycle);
                if (hDlgExp)
                    DestroyWindow(hDlgExp);
                if (hDlgColor)
                    DestroyWindow(hDlgColor);
                if (hDlgFourier)
                    DestroyWindow(hDlgFourier);
                if (hDlgAnalog)
                    DestroyWindow(hDlgAnalog);
                if (hDlgCell)
                    DestroyWindow(hDlgCell);
                if (hDlgElectric)
                    DestroyWindow(hDlgElectric);
                if (hDlgDigital)
                    DestroyWindow(hDlgDigital);
                if (hDlgView)
                    DestroyWindow(hDlgView);
                if (hDlgWorld)
                    DestroyWindow(hDlgWorld);
                if (hDlgConfigure)
                    DestroyWindow(hDlgConfigure);
                if (hUserDialog)
                        DestroyWindow(hUserDialog);
                if (hDlgGenerators)
                    DestroyWindow(hDlgGenerators);
            }
            break;



// END CONTROLS MENU and DIALOG CONTROLS =============
// START TOOLS MENU====================================

        case CUR_PICK:
        case CUR_ZAP:
        case CUR_TOUCH:
        case CUR_COPY:
        case CUR_GENERATOR:
            if ( id == CUR_PICK )
            {
                    ToolBar_CheckButton(hwndActionToolbar, CUR_PICK, TRUE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_TOUCH, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_GENERATOR, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_ZAP, FALSE );
            }
            else
            if ( id == CUR_TOUCH )
            {
                    ToolBar_CheckButton(hwndActionToolbar, CUR_TOUCH, TRUE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_PICK, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_GENERATOR, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_ZAP, FALSE );
            }
            else
            if ( id == CUR_ZAP )
            {
                    ToolBar_CheckButton(hwndActionToolbar, CUR_ZAP, TRUE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_TOUCH, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_PICK, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_GENERATOR, FALSE );
            }
            else
            if ( id == CUR_GENERATOR )
            {
                    ToolBar_CheckButton(hwndActionToolbar, CUR_GENERATOR, TRUE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_TOUCH, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_PICK, FALSE );
                    ToolBar_CheckButton(hwndActionToolbar, CUR_ZAP, FALSE );

            }
            SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR) LoadCursor(hInst, MAKEINTRESOURCE(id)));
            oldcursormode = cursormode;
            cursormode = id;
            break;

        case IDM_CHANGEALL:
            focusflag = 1;
            ToolBar_ChangeBitmap(hwndActionToolbar, IDM_CHANGEALL, BUT_CHANGEFOCUSLARGE);
            ToolBar_SetCmdID(hwndActionToolbar, CHANGEALLFOCUS_BUTTON, IDM_CHANGEFOCUS);
            update_flag = 1; //Have to update the focus/all radio button in the dialogs.
            break;

        case IDM_CHANGEFOCUS:
            focusflag = 0;
            ToolBar_ChangeBitmap(hwndActionToolbar, IDM_CHANGEFOCUS, BUT_CHANGEALLLARGE);
            ToolBar_SetCmdID(hwndActionToolbar, CHANGEALLFOCUS_BUTTON, IDM_CHANGEALL);
            update_flag = 1; //Have to update the focus/all radio button in the dialogs.
            break;

        case IDM_CHANGEFOCUSMENU:
            focusflag = 1;
            ToolBar_ChangeBitmap(hwndActionToolbar, IDM_CHANGEALL, BUT_CHANGEFOCUSLARGE);
            ToolBar_SetCmdID(hwndActionToolbar, CHANGEALLFOCUS_BUTTON, IDM_CHANGEFOCUS);
            update_flag = 1; //Have to update the focus/all radio button in the dialogs.
            break;
        case IDM_CHANGEALLMENU:
            focusflag = 0;
            ToolBar_ChangeBitmap(hwndActionToolbar, IDM_CHANGEFOCUS, BUT_CHANGEALLLARGE);
            ToolBar_SetCmdID(hwndActionToolbar, CHANGEALLFOCUS_BUTTON, IDM_CHANGEALL);
            update_flag = 1; //Have to update the focus/all radio button in the dialogs.
            break;

// END TOOLS MENU====================================
// START HELP MENU====================================

        case IDM_HELP:                              // Calls Help
                ShellExecuteA(0, 0, "http://www.rudyrucker.com/capow/capowhelp.htm", 0, 0, SW_SHOW); //BEST solution, found in 2017.  Keep the help file online
                    //and let the users go read it online.  Easy to update this way.  ShellExecute does the job!

                break;

        case IDM_ABOUT:         // Opens About Dialog
                DialogBoxA(hInst, "ABOUT", hwnd, (DLGPROC)AboutProc);
                break;
        case IDM_GENERATORS:
            if(!hDlgGenerators)
            {
                hDlgGenerators = CreateDialogA( hInst, (LPSTR)"GENERATORS", hwnd,
                                           (DLGPROC)GeneratorsProc );
                GetWindowRect( hDlgGenerators, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"GENERATORSX",100);
                rect.top  = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"GENERATORSY",100);
                if (GetSystemMetrics(SM_CXSCREEN) < rect.left-10) rect.left = 25;
                if (GetSystemMetrics(SM_CYSCREEN) < rect.top -10) rect.top = 25;
                MoveWindow(hDlgGenerators, rect.left, rect.top, rect.right, rect.bottom, FALSE);
                ShowWindow(hDlgGenerators, TRUE);
            } // if
            else
                DestroyWindow(hDlgGenerators);
            break;
        case IDM_OPENGL:
            if(!hDlgOpenGL)
            {
                hDlgOpenGL = CreateDialogA( hInst, (LPSTR)"OPENGL", hwnd,
                                           (DLGPROC)OpenGLProc);
                GetWindowRect( hDlgOpenGL, &rect );
                rect.bottom -= rect.top;
                rect.right  -= rect.left;
                rect.left = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"OPENGLX",100);
                rect.top  = (int)GetProfileIntA((LPSTR)szMyAppName,(LPSTR)"OPENGLY",100);
                if (GetSystemMetrics(SM_CXSCREEN) < rect.left-10) rect.left = 25;
                if (GetSystemMetrics(SM_CYSCREEN) < rect.top -10) rect.top = 25;
                MoveWindow(hDlgOpenGL, rect.left, rect.top, rect.right, rect.bottom, FALSE);
                ShowWindow(hDlgOpenGL, TRUE);
            } // if
            else
                DestroyWindow(hDlgOpenGL);
            break;
    }  // End of main message processing Switch
}
/*********************************************************/


LRESULT MyWnd_MENUSELECT(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
     return Statusbar_MenuSelect ( hwnd, MAKEWPARAM(item, flags), LPARAM( hmenu ) );
}

/*********************************************************/

static void MyWnd_LBUTTONDOWN(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    //((fn)((hwnd), FALSE, (int)LOWORD(lParam), (int)HIWORD(lParam), (UINT)(wParam)), 0L)
    RECT rect;

    HDC hdc = GetDC(hwnd);
    SetCapture(hwnd);
    CA*  oldfocus = calife_list->FocusCA();

    switch(cursormode)
    {
/* If you are unzoomed and click on the focus, this zooms.
    If you are zoomed and click on the focus, this unzooms.
    If you are unzoomed and click on another, it changes the
        focus to the one you clicked on */
        case CUR_PICK:
            if (!zoomviewflag)
            {

                if (calife_list->Setfocus(hdc, calife_list->
                    Getfocus(x, y)) == 1)
                    // If == 1 then the user is clicking on the focused ca, so zoom
                {
                    calife_list->Zoom(1);
                    zoomviewflag = TRUE;
                    calife_list->Locate();
                    SendMessage(hwnd, WM_COMMAND, IDM_CLEAR, 0L);
/*The next two lines fix a bug relating to the OpenGL view of 2D Cas.  Often
when you zoom in on a 2D CA this view was coming up dead with no action and
would only wake up when the user resized the window.  So we fake a resize,
and this gets rid of the bug! Rudy 5/21/97. By the way, just doing the
obviously relevant line from the _SIZE code isn't enough, the obvious
line being:             capowgl->Size(hwnd);//mike
*/

/*mike 10/97: I believe the problem with capowgl->Size(hwnd) was that upon startup,
it was called too early, before the window was created. So after moving that
function, the problem seems fixed.  As a test, I've commented out the old fix*/

                    capowgl->AdjustHeightFactor(calife_list->FocusCA());
#if defined(CAPOW_ENABLE_ALPAKA)
                    UpdateAlpakaDisplayType();
#endif
                if( hDlgOpenGL )
                    InvalidateRect(hDlgOpenGL, NULL, TRUE);

                if( hDlgGenerators )  // Initialize list box
                    SendMessage( hDlgGenerators, WM_INITDIALOG, 0, 0L );

                }
                else //Unzoomed, and User not clicking on focus CA
                {

                    update_flag = TRUE; //Maybe changing focus

                    zoomviewflag = FALSE;
#if defined(CAPOW_ENABLE_ALPAKA)
                    UpdateAlpakaDisplayType();
#endif
/* If I have about six user parameters then when I shift focus to something with
one user parameter and then come back to the six guy not all six are showing
if I only do recreateUserDialog(), but the following works: */
                    if (hUserDialog)
                    {
                        DestroyWindow(hUserDialog);
                        hUserDialog = 0;
                        SendMessage(hwnd, WM_COMMAND, IDM_USERDIALOG, 0L);
                    }
                }
            }
            else
            {   //ifzoomed on a 2d CA
                if(calife_list->FocusCA()->Getviewmode() ==IDC_2D_VIEW)
                    capowgl->LeftButtonDown(fDoubleClick, x, y, keyFlags);

            }
            if( hDlgFourier )  // Adjust Slider bar to position of focus
                SendMessage( hDlgFourier, WM_INITDIALOG, 0, 0L );
            break;

/* This just copy mutates the focus to all other CA's.  If you click on
    a CA other that the focus, it changes focus to the one you clicked on
    and copy mutates that CA to all others. */
        case CUR_ZAP:
            if (calife_list->Setfocus(hdc, calife_list->Getfocus(x, y)) != 1)
            {   // focus changed
                recreateUserDialog();
            }
            calife_list->Copymutate();
            SendMessage(hwnd, WM_COMMAND, IDM_CLEAR, 0L);
            break;


/* This copies the focused CA into the CA that you left click on.
    It makes an exact duplicate copy regardless of CA type.
    If you right click, it changes the focus */
        case CUR_COPY:
            calife_list->CopyCA(calife_list->FocusCA(),
            calife_list->Getfocus(x, y));
            GetClientRect(hwnd, &rect);
            calife_list->Locate(rect.right,rect.bottom); // Make sure they're at the top
            SendMessage(hwnd, WM_COMMAND, IDM_CLEAR, 0L);
            break;

        case CUR_TOUCH:
    // should also do an if focused check
            calife_list->Touch_CA(x,y,WM_LBUTTONDOWN);
            break;

        case CUR_GENERATOR:
            calife_list->LocateNewGenerator(x,y,WM_LBUTTONDOWN);
            if (hDlgGenerators)
                SendMessage(hDlgGenerators, WM_COMMAND, SETCURSEL, 0L);
            break;

    }  // End Switch ( cursor Mode )

    ReleaseDC(hwnd, hdc);
}

/*********************************************************/

static void MyWnd_RBUTTONDOWN(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    //((fn)((hwnd), FALSE, (int)LOWORD(lParam), (int)HIWORD(lParam), (UINT)(wParam)), 0L)
    HDC hdc = GetDC(hwnd);

    switch(cursormode)
    {
        case CUR_PICK:
        case CUR_GENERATOR:
            if (calife_list->Zoom(0))
            {
                zoomviewflag = FALSE;
#if defined(CAPOW_ENABLE_ALPAKA)
                UpdateAlpakaDisplayType();
#endif
                calife_list->Locate();
                SendMessage(hwnd, WM_COMMAND, IDM_CLEAR, 0L);

                if( hDlgOpenGL )
                    InvalidateRect(hDlgOpenGL, NULL, TRUE);
            }
            break;

/* This just changes the focus.  I thought it would be neat to copy with
    the left button and change focus with the right */
        case CUR_COPY:
            calife_list->Setfocus(hdc, calife_list->Getfocus(x, y));
            update_flag = TRUE;
            break;

        case CUR_TOUCH: // should also do an if focused check
            calife_list->Touch_CA(x,y,WM_RBUTTONDOWN);
            break;

    } // End Switch ( cursormode )

    ReleaseDC(hwnd, hdc);

}

/*********************************************************/

static void MyWnd_CLOSE(HWND hwnd)   //((fn)(hwnd), 0L)
{
#ifdef QUERY_ON_CLOSE
    switch ( MessageBoxA( hwnd, (LPSTR)"Save Current Experiment?",
                       (LPSTR)"Ready To Exit CAPOW!", MB_YESNOCANCEL ) )
    {
        case IDCANCEL:
                return;

        case IDYES:
            load_save_cells_flag = TRUE;
            calife_list->Saveall("ACTIVE.CAS", TRUE);

            //The TRUE argument means do an automatic overwrite of
            //any existing ACTIVE.CAS.
            // Now drop down to IDNO case.

        case IDNO:
#endif QUERY_ON_CLOSE
            //Same old exit code
            // Close any boxes that may be open
        if (randomize_timer_handle)
            KillTimer(hwnd, randomize_timer_handle);
        if (update_timer_handle)
            KillTimer(hwnd, update_timer_handle);
        if (hDlgCycle)
        {
            DestroyWindow( hDlgCycle);
            hDlgCycle = 0;
        }

        if (hDlgExp)
        {
            DestroyWindow( hDlgExp);
            hDlgExp = 0;
        }

        //About dialog is modal, so there is no hDlgAbout

        if (hDlgColor)
        {
            DestroyWindow( hDlgColor);
            hDlgColor = 0;
        }

        if (hDlgFourier)
        {
            DestroyWindow( hDlgFourier);
            hDlgFourier = 0;
        }

        if (hDlgAnalog)
        {
            DestroyWindow (hDlgAnalog);
            hDlgAnalog = 0;
        }

        if ( hDlgElectric )
        {
            DestroyWindow( hDlgElectric );
            hDlgElectric = 0;
        }

        if( hDlgDigital )
        {
            DestroyWindow( hDlgDigital );
            hDlgDigital = 0;
        }

        if (hDlgView)
        {
            DestroyWindow( hDlgView);
            hDlgView = 0;
        }

        if( hDlgWorld )
        {
            DestroyWindow( hDlgWorld );
            hDlgWorld = 0;
        }

        if( hDlgConfigure )
        {
            DestroyWindow( hDlgConfigure );
            hDlgConfigure = 0;
        }

        if( hDlgCell )
        {
            DestroyWindow( hDlgCell );
            hDlgCell = 0;
        }

        if (hUserDialog)
        {
            DestroyWindow(hUserDialog);
            hUserDialog = 0;
        } // if

        if (hDlgGenerators)
        {
            DestroyWindow(hDlgGenerators);
            hDlgGenerators = 0;
        }

        if (hDlgOpenGL)
        {
            DestroyWindow(hDlgOpenGL);
            hDlgOpenGL = 0;
        }

        delete capowgl;
        delete calife_list;
        calife_list = NULL; //This way you can avoid update after it's gone.
        //Calls dll_list destructor.  Important to call FreeLibrary on DLLS.

        PostQuitMessage (0);
#ifdef QUERY_ON_CLOSE
    }  // End Switch ( MessageBox YES NO )
#endif //QUREY_ON_CLOSE

}

/*********************************************************/

static void MyWnd_DESTROY(HWND hwnd)     //((fn)(hwnd), 0L)
{
    MyWnd_CLOSE(hwnd);
}

/*********************************************************/

BOOL MyWnd_INITDIALOG(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    //(LRESULT)(DWORD)(UINT)(BOOL)(fn)((hwnd), (HWND)(wParam), lParam)
    return 0;
}


static void MyWnd_MOUSEMOVE(HWND hwnd, int x, int y, UINT flags)
{
  //  ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
    switch(cursormode)
    {
    case CUR_PICK:
        if(zoomviewflag && calife_list->FocusCA()->Getviewmode() ==IDC_2D_VIEW)
            capowgl->MouseMove(x, y, flags);
        break;
    }
}

static void MyWnd_LBUTTONUP(HWND hwnd, int x, int y, UINT flags)
{
//    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)
    switch(cursormode)
    {
    case CUR_PICK:
        if(zoomviewflag && calife_list->FocusCA()->Getviewmode() ==IDC_2D_VIEW)
            capowgl->LeftButtonUp(x, y, flags);
        break;
    }
    ReleaseCapture();

}

static void MyWnd_INITMENUPOPUP(HWND hwnd,  HMENU menu, UINT menuindex, BOOL x )
{
    switch ( menuindex )
    {
        case 0:         // File Menu
            CheckMenuItem(menu, IDM_COMPRESS, MF_BYCOMMAND |
            (compressFile?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu,IDM_PAUSE,MF_BYCOMMAND|
            ((calife_list->GetSleep())?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu,IDM_CONFIGURE,MF_BYCOMMAND|
            ((randomize_timer_handle)?MF_CHECKED:MF_UNCHECKED));

            int i;
            for (i=ID_VERYSLOW; i<= ID_FASTEST; i++)
                CheckMenuItem(menu, i, MF_BYCOMMAND |
                (i==ID_VERYSLOW+update_timer_speed_index?MF_CHECKED:MF_UNCHECKED));
            break;


        case 2:  // Control Menu

            if ( focusflag ) // Change all change focus
            {
                CheckMenuItem(menu, IDM_CHANGEALLMENU,   MF_BYCOMMAND | MF_UNCHECKED   );
                CheckMenuItem(menu, IDM_CHANGEFOCUSMENU, MF_BYCOMMAND | MF_CHECKED );
            }
            else
            {
                CheckMenuItem(menu, IDM_CHANGEALLMENU, MF_BYCOMMAND   | MF_CHECKED );
                CheckMenuItem(menu, IDM_CHANGEFOCUSMENU, MF_BYCOMMAND | MF_UNCHECKED   );
            }

            CheckMenuItem(menu, IDM_EXP, MF_BYCOMMAND |
            (hDlgExp?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_CYCLE, MF_BYCOMMAND |
            (hDlgCycle?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_COLOR, MF_BYCOMMAND |
            (hDlgColor?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_FOURIER, MF_BYCOMMAND |
            (hDlgFourier ?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_ANALOG, MF_BYCOMMAND |
            (hDlgAnalog?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_CELL, MF_BYCOMMAND |
            (hDlgCell?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_ELECTRIC, MF_BYCOMMAND |
            (hDlgElectric?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_DIGITAL, MF_BYCOMMAND |
            (hDlgDigital ?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_VIEW, MF_BYCOMMAND |
            (hDlgView ?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_WORLD, MF_BYCOMMAND |
            (hDlgWorld?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_CONFIGURE, MF_BYCOMMAND |
            (hDlgConfigure?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_USERDIALOG, MF_BYCOMMAND |
            (hUserDialog ?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_GENERATORS, MF_BYCOMMAND |
            (hDlgGenerators?MF_CHECKED:MF_UNCHECKED));

            CheckMenuItem(menu, IDM_OPENGL, MF_BYCOMMAND |
            (hDlgOpenGL?MF_CHECKED:MF_UNCHECKED));
            break;
        case 3:  // view Menu
            CheckMenuItem(menu, IDM_SHOW_STATUS, MF_BYCOMMAND |
            (statusON?MF_CHECKED:MF_UNCHECKED));
            CheckMenuItem(menu, IDM_SHOW_TOOLBAR, MF_BYCOMMAND |
            (toolbarON?MF_CHECKED:MF_UNCHECKED));

            if ( ActionToolbar )  // New or Old Toolbar
            {
                CheckMenuItem(menu, IDM_NEWTOOLBAR, MF_BYCOMMAND | MF_CHECKED   );
                CheckMenuItem(menu, IDM_OLDTOOLBAR, MF_BYCOMMAND | MF_UNCHECKED );
            }
            if ( DialogToolbar )
            {
                CheckMenuItem(menu, IDM_NEWTOOLBAR, MF_BYCOMMAND | MF_UNCHECKED );
                CheckMenuItem(menu, IDM_OLDTOOLBAR, MF_BYCOMMAND | MF_CHECKED   );
            }
            if ( DialogToolbar == 0 && ActionToolbar == 0 )
            {
                CheckMenuItem(menu, IDM_NEWTOOLBAR, MF_BYCOMMAND | MF_UNCHECKED );
                CheckMenuItem(menu, IDM_OLDTOOLBAR, MF_BYCOMMAND | MF_UNCHECKED   );
            }
            break;
        case 4:  // cursor Menu



            CheckMenuItem(menu, CUR_PICK,   MF_BYCOMMAND | MF_UNCHECKED   );
            CheckMenuItem(menu, CUR_ZAP,    MF_BYCOMMAND | MF_UNCHECKED   );
            CheckMenuItem(menu, CUR_GENERATOR, MF_BYCOMMAND | MF_UNCHECKED   );
            CheckMenuItem(menu, CUR_TOUCH,  MF_BYCOMMAND | MF_UNCHECKED   );

            switch ( cursormode )
            {
                case CUR_PICK:
                    CheckMenuItem(menu, CUR_PICK,   MF_BYCOMMAND | MF_CHECKED   );
                    break;

                case CUR_ZAP:
                    CheckMenuItem(menu, CUR_ZAP,    MF_BYCOMMAND | MF_CHECKED   );
                    break;

                case CUR_TOUCH:
                    CheckMenuItem(menu, CUR_TOUCH,  MF_BYCOMMAND | MF_CHECKED   );
                    break;
                case CUR_GENERATOR:
                    CheckMenuItem(menu, CUR_GENERATOR, MF_BYCOMMAND | MF_CHECKED   );
                    break;
            }       // end switch ( cusormode )
            break;
        } // end switch ( menuindex )

}


static void MyWnd_TIMER(HWND hwnd, UINT timerid)
{

#ifdef MASTERTIMER
        if (timerid == UPDATE_TIMER_ID)
        {
            Cellmain(hwnd);
            return;
        }
#endif //MASTERTIMER
     if (timerid == RANDOMIZE_TIMER_ID)
        randomizenow = TRUE;
}


/*===================================================================*/
//                      WINPROC


LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT uiStringBase = 100;


    switch (message)
    {
        HANDLE_MSG(hwnd,WM_CREATE, MyWnd_CREATE);
        HANDLE_MSG(hwnd,WM_PAINT, MyWnd_PAINT);
        HANDLE_MSG(hwnd,WM_SIZE,MyWnd_SIZE);
        HANDLE_MSG(hwnd,WM_MOVE,MyWnd_MOVE);
        HANDLE_MSG(hwnd,WM_COMMAND,MyWnd_COMMAND);
        HANDLE_MSG(hwnd,WM_LBUTTONDOWN,MyWnd_LBUTTONDOWN);
        HANDLE_MSG(hwnd,WM_RBUTTONDOWN,MyWnd_RBUTTONDOWN);
        HANDLE_MSG(hwnd,WM_CLOSE,MyWnd_CLOSE);
        HANDLE_MSG(hwnd,WM_DESTROY,MyWnd_DESTROY);
        HANDLE_MSG(hwnd,WM_INITDIALOG,MyWnd_INITDIALOG);
        HANDLE_MSG(hwnd,WM_NOTIFY,ToolBarNotify);  // Handles Toolbar
        HANDLE_MSG(hwnd, WM_MENUSELECT, MyWnd_MENUSELECT);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, MyWnd_MOUSEMOVE);
        HANDLE_MSG(hwnd,WM_LBUTTONUP,MyWnd_LBUTTONUP);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP,MyWnd_INITMENUPOPUP);
        HANDLE_MSG(hwnd, WM_TIMER, MyWnd_TIMER);
    }
    return DefWindowProcA (hwnd, message, wParam, lParam) ;
}
//=======================================================================
//=======================================================================
//=======================================================================
// Local Function Definitions



//-------------------Continually running ca update procedure

void Cellmain(HWND hwnd)
{
    HDC hdc;
    static long GenCount;
    static char GenCountChar[10];

    MSG msg;
/*  There are sometimes you don't want to run.  First you don't want to run
if this call happens before WM_CREATE or WM_DESTROY, as it possibly might.
Second you don't want to run while you're saving a CA, you want it to stay the
same till you finish saving it.  Third you want to be extra sure not to run
when a window is minimized, though the next check would take care of this as
well. */
    if (!calife_list || inloadsave || windowIsMinimized) //2017 put || instead of |
        return;
/* If GetForegroundWindow (or GetFocus would work as well) is not my main
window or one of its modeless dialogs, then I won't do an update.  This check
needn't take very long because a boolean combo gets shortcircuted
in evaluation, and if the focus is on the main hwnd, the eval bails after 1st
term. Rudy 12/2/97.  The reason for this is to be a better "citizen" and let
other programs run.  We could add a _backgroundrun flag to override this good
behavior if we wanted to.*/
    HWND activewnd = GetForegroundWindow();
    if ( hwnd != activewnd &&
    hDlgCycle != activewnd && hDlgExp != activewnd && hDlgColor != activewnd &&
    hDlgFourier != activewnd && hDlgAnalog != activewnd && hDlgCell != activewnd &&
    hDlgElectric != activewnd &&    hDlgDigital != activewnd &&
    hDlgView != activewnd && hDlgWorld != activewnd && hUserDialog != activewnd &&
    hDlgGenerators != activewnd && hDlgOpenGL != activewnd &&
    hDlgConfigure != activewnd)
        return;
    if (randomizenow)
    {
        calife_list->Randomize(fRandFlags);
        randomizenow = FALSE;
    }

    hdc = GetDC(hwnd);

    calife_list->Update_and_Show(hdc); //Does the 3D stuff.

    if ( !calife_list->GetSleep() )
    {
        calife_list->UpdateGenerationCount();
        GenCount = calife_list->FocusCA()->GetGenerationCount();
        ltoa ( GenCount, GenCountChar, 10 );
        Status_SetText(hwndStatusBar, 2, 0, GenCountChar);
#if defined(CAPOW_ENABLE_ALPAKA)
        if (capowgl != nullptr && capowgl->Type() == LIVE_GPU)
            UpdateWindow(hwndStatusBar);
#endif
    }

/* if update_flag is TRUE, then some procedure changed some vital information
    of the focus CA, and we need to update the information in these
    boxes */
    if (update_flag)
    {
        if (hDlgCycle)
            SendMessage(hDlgCycle, WM_COMMAND, SC_UPDATE, 0L);
        if (hDlgExp)
            SendMessage(hDlgExp, WM_COMMAND, SC_UPDATE, 0L);
        if (hDlgColor)
            SendMessage(hDlgColor, WM_COMMAND, SC_UPDATE, 0L);
        if (hDlgAnalog)
            SendMessage (hDlgAnalog, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgCell )
            SendMessage( hDlgCell, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgElectric )
            SendMessage( hDlgElectric, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgView )
            SendMessage( hDlgView, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgDigital )
            SendMessage( hDlgDigital, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgWorld )
            SendMessage( hDlgWorld, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgFourier )
            SendMessage( hDlgFourier, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgGenerators )
            SendMessage( hDlgGenerators, WM_COMMAND, SC_UPDATE, 0L );
        if( hDlgOpenGL )
            SendMessage( hDlgOpenGL, WM_COMMAND, SC_UPDATE, 0L );

        update_flag = FALSE;
    }

    ReleaseDC(hwnd, hdc);
}

//////////////////////
void ParseCommandLine ( char commandline[], char* WinArgv[] )
{
    int i=0;
    char *token;
    //WinArgv holds up to 3 file names

     token = strtok ( commandline, " " );
    if ( !token )
        return;
    WinArgv[i] = new char [ strlen ( token ) +1 ];
    strcpy ( WinArgv[i], token ) ;
    i++;
    while ( token = strtok ( NULL, " " ) )
    {
        WinArgv[i] = new char [ strlen ( token ) +1 ];
        strcpy ( WinArgv[i++], token );
    }

}

BOOL CheckExtension ( char Extension[], char DesiredExtension[] )
{
    int i=0;

    while ( Extension[i] != '\0' )
      {
            Extension[i] = toupper ( Extension[i] );

            i++;
      }
    i = 0;
    while ( DesiredExtension[i] != '\0' )
      {
            DesiredExtension[i] = toupper ( DesiredExtension[i] );
            i++;
      }

        if ( strcmp ( DesiredExtension, Extension ) == 0 )
            return TRUE;
        else
            return FALSE;
}

void GrabExtension ( LPSTR lpszCmdParam, char Extension[] )
{
    char *endp, temp[256];

    if ( lpszCmdParam == NULL )
        return;
    strcpy ( temp, lpszCmdParam );

    if ( !( strchr ( temp, '.' ) ) )
        Extension[0] = '\0';
    else
    {
        while ( endp = strchr ( temp, '.' )  )
            strcpy ( temp, ++endp );
        strncpy ( Extension, temp, 3 );
        Extension[3] = '\0';
    }
    return;
}
