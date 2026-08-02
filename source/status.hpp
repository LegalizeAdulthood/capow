//Status.h
#ifndef STATUS_H
#define STATUS_H

//-------------------------------------------------------------------
// Status Bar Helper Macros   Petzold Chap. 12
//-------------------------------------------------------------------
#define Status_GetBorders(hwnd, aBorders) \
    (BOOL)SendMessageA((hwnd), SB_GETBORDERS, 0, (LPARAM) (LPINT) aBorders)

#define Status_GetParts(hwnd, nParts, aRightCoord) \
    (int)SendMessageA((hwnd), SB_GETPARTS, (WPARAM) nParts, (LPARAM) (LPINT) aRightCoord)

#define Status_GetRect(hwnd, iPart, lprc) \
    (BOOL)SendMessageA((hwnd), SB_GETRECT, (WPARAM) iPart, (LPARAM) (LPRECT) lprc)

#define Status_GetText(hwnd, iPart, szText) \
    (DWORD)SendMessageA((hwnd), SB_GETTEXTA, (WPARAM) iPart, (LPARAM) (LPSTR) szText)

#define Status_GetTextLength(hwnd, iPart) \
    (DWORD)SendMessageA((hwnd), SB_GETTEXTLENGTHA, (WPARAM) iPart, 0L)

#define Status_SetMinHeight(hwnd, minHeight) \
    (void)SendMessageA((hwnd), SB_SETMINHEIGHT, (WPARAM) minHeight, 0L)

#define Status_SetParts(hwnd, nParts, aWidths) \
    (BOOL)SendMessageA((hwnd), SB_SETPARTS, (WPARAM) nParts, (LPARAM) (LPINT) aWidths)

#define Status_SetText(hwnd, iPart, uType, szText) \
    (BOOL)SendMessageA((hwnd), SB_SETTEXTA, (WPARAM) (iPart | uType), (LPARAM) (LPSTR) szText)

#define Status_Simple(hwnd, fSimple) \
    (BOOL)SendMessageA((hwnd), SB_SIMPLE, (WPARAM) (BOOL) fSimple, 0L)



HWND InitStatusBar ( HWND hwndParent );
HWND RebuildStatusBar ( HWND hwndParent, WORD wFLAG );
void StatusBarMessage ( HWND hwndSB, WORD wMsg );
LRESULT Statusbar_MenuSelect (HWND hwnd, WPARAM wParam, LPARAM lParam);
#endif  // STATUS_H
