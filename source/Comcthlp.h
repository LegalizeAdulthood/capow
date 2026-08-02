/*******************************************************************************
    FILE:               comcthlp.h
    PROJECT:            CAMCOS CAPOW!
    ENVIRONMENT:        MS Visual C++ 5.0/MS Windows 95/NT


    FILE DESCRIPTION:   This file contains macros to simply sending of
                        messages to common files.

    COPYRIGHT INFO:

        COMCTHLP.H -- Helper macros for common controls
                 (c) Paul Yao, 1996

        Portions Copyright (c) 1992-1996, Microsoft Corp.

    UPDATE LOG:

*******************************************************************************/
//-------------------------------------------------------------------
// Hot-Key Helper Macros
//-------------------------------------------------------------------
#define HotKey_SetHotKey(hwnd, bVKHotKey, bfMods) \
    (void)SendMessageA((hwnd), HKM_SETHOTKEY, MAKEWORD(bVKHotKey, bfMods), 0L)

#define HotKey_GetHotKey(hwnd) \
    (WORD)SendMessageA((hwnd), HKM_GETHOTKEY, 0, 0L)

#define HotKey_SetRules(hwnd, fwCombInv, fwModInv) \
    (void)SendMessageA((hwnd), HKM_SETRULES, (WPARAM) fwCombInv, MAKELPARAM(fwModInv, 0))

//-------------------------------------------------------------------
// Progress Bar Helper Macros
//-------------------------------------------------------------------
#define Progress_SetRange(hwnd, nMinRange, nMaxRange) \
    (DWORD)SendMessageA((hwnd), PBM_SETRANGE, 0, MAKELPARAM(nMinRange, nMaxRange))

#define Progress_SetPos(hwnd, nNewPos) \
    (int)SendMessageA((hwnd), PBM_SETPOS, (WPARAM) nNewPos, 0L)

#define Progress_DeltaPos(hwnd, nIncrement) \
    (int)SendMessageA((hwnd), PBM_DELTAPOS, (WPARAM) nIncrement, 0L)

#define Progress_SetStep(hwnd, nStepInc) \
    (int)SendMessageA((hwnd), PBM_SETSTEP, (WPARAM) nStepInc, 0L)

#define Progress_StepIt(hwnd) \
    (int)SendMessageA((hwnd), PBM_STEPIT, 0, 0L)

//-------------------------------------------------------------------
// Rich Edit Control Helper Macros
//-------------------------------------------------------------------

//---------------- Begin Macros Copied from windowsx.h---------------
#define RichEdit_Enable(hwndCtl, fEnable) \
    (BOOL)EnableWindow((hwndCtl), (fEnable))

#define RichEdit_GetText(hwndCtl, lpch, cchMax) \
    (int)GetWindowTextA((hwndCtl), (lpch), (cchMax))

#define RichEdit_GetTextLength(hwndCtl) \
    (int)GetWindowTextLengthA(hwndCtl)

#define RichEdit_SetText(hwndCtl, lpsz)  \
    (BOOL)SetWindowTextA((hwndCtl), (lpsz))

#define RichEdit_LimitText(hwndCtl, cchMax) \
    ((void)SendMessageA((hwndCtl), EM_LIMITTEXT, (WPARAM)(cchMax), 0L))

#define RichEdit_GetLineCount(hwndCtl) \
    ((int)(DWORD)SendMessageA((hwndCtl), EM_GETLINECOUNT, 0L, 0L))

#define RichEdit_GetLine(hwndCtl, line, lpch, cchMax) \
    ((*((int *)(lpch)) = (cchMax)), ((int)(DWORD)SendMessageA((hwndCtl), EM_GETLINE, (WPARAM)(int)(line), (LPARAM)(LPSTR)(lpch))))

#define RichEdit_GetRect(hwndCtl, lprc) \
    ((void)SendMessageA((hwndCtl), EM_GETRECT, 0L, (LPARAM)(RECT *)(lprc)))

#define RichEdit_SetRect(hwndCtl, lprc) \
    ((void)SendMessageA((hwndCtl), EM_SETRECT, 0L, (LPARAM)(const RECT *)(lprc)))

#define RichEdit_GetSel(hwndCtl) \
    ((DWORD)SendMessageA((hwndCtl), EM_GETSEL, 0L, 0L))

#define RichEdit_SetSel(hwndCtl, ichStart, ichEnd) \
    ((void)SendMessageA((hwndCtl), EM_SETSEL, (ichStart), (ichEnd)))

#define RichEdit_ReplaceSel(hwndCtl, lpszReplace) \
    ((void)SendMessageA((hwndCtl), EM_REPLACESEL, 0L, (LPARAM)(LPCSTR)(lpszReplace)))

#define RichEdit_GetModify(hwndCtl) \
    ((BOOL)(DWORD)SendMessageA((hwndCtl), EM_GETMODIFY, 0L, 0L))

#define RichEdit_SetModify(hwndCtl, fModified) \
    ((void)SendMessageA((hwndCtl), EM_SETMODIFY, (WPARAM)(UINT)(fModified), 0L))

#define RichEdit_ScrollCaret(hwndCtl) \
    ((BOOL)(DWORD)SendMessageA((hwndCtl), EM_SCROLLCARET, 0, 0L))

#define RichEdit_LineFromChar(hwndCtl, ich) \
    ((int)(DWORD)SendMessageA((hwndCtl), EM_LINEFROMCHAR, (WPARAM)(int)(ich), 0L))

#define RichEdit_LineIndex(hwndCtl, line) \
    ((int)(DWORD)SendMessageA((hwndCtl), EM_LINEINDEX, (WPARAM)(int)(line), 0L))

#define RichEdit_LineLength(hwndCtl, line) \
    ((int)(DWORD)SendMessageA((hwndCtl), EM_LINELENGTH, (WPARAM)(int)(line), 0L))

#define RichEdit_Scroll(hwndCtl, dv, dh) \
    ((void)SendMessageA((hwndCtl), EM_LINESCROLL, (WPARAM)(dh), (LPARAM)(dv)))

#define RichEdit_CanUndo(hwndCtl)  \
    ((BOOL)(DWORD)SendMessageA((hwndCtl), EM_CANUNDO, 0L, 0L))

#define RichEdit_Undo(hwndCtl)  \
    ((BOOL)(DWORD)SendMessageA((hwndCtl), EM_UNDO, 0L, 0L))

#define RichEdit_EmptyUndoBuffer(hwndCtl) \
    ((void)SendMessageA((hwndCtl), EM_EMPTYUNDOBUFFER, 0L, 0L))

#define RichEdit_GetFirstVisibleLine(hwndCtl) \
    ((int)(DWORD)SendMessageA((hwndCtl), EM_GETFIRSTVISIBLELINE, 0L, 0L))

#define RichEdit_SetReadOnly(hwndCtl, fReadOnly) \
    ((BOOL)(DWORD)SendMessageA((hwndCtl), EM_SETREADONLY, (WPARAM)(BOOL)(fReadOnly), 0L))

#define RichEdit_SetWordBreakProc(hwndCtl, lpfnWordBreak) \
    ((void)SendMessageA((hwndCtl), EM_SETWORDBREAKPROC, 0L, (LPARAM)(EDITWORDBREAKPROC)(lpfnWordBreak)))

#define RichEdit_GetWordBreakProc(hwndCtl) \
    ((EDITWORDBREAKPROC)SendMessageA((hwndCtl), EM_GETWORDBREAKPROC, 0L, 0L))

#define RichEdit_CanPaste(hwnd, uFormat) \
    (BOOL)SendMessageA((hwnd), EM_CANPASTE, (WPARAM) (UINT) uFormat, 0L)

#define RichEdit_CharFromPos(hwnd, x, y) \
    (DWORD)SendMessageA((hwnd), EM_CHARFROMPOS, 0, MAKELPARAM(x, y))

#define RichEdit_DisplayBand(hwnd, lprc) \
    (BOOL)SendMessageA((hwnd), EM_DISPLAYBAND, 0, (LPARAM) (LPRECT) lprc)

#define RichEdit_ExGetSel(hwnd, lpchr) \
    (void)SendMessageA((hwnd), EM_EXGETSEL, 0, (LPARAM) (CHARRANGE FAR *) lpchr)

#define RichEdit_ExLimitText(hwnd, cchTextMax) \
    (void)SendMessageA((hwnd), EM_EXLIMITTEXT, 0, (LPARAM) (DWORD) cchTextMax)

#define RichEdit_ExLineFromChar(hwnd, ichCharPos) \
    (int)SendMessageA((hwnd), EM_EXLINEFROMCHAR, 0, (LPARAM) (DWORD) ichCharPos)

#define RichEdit_ExSetSel(hwnd, ichCharRange) \
    (int)SendMessageA((hwnd), EM_EXSETSEL, 0, (LPARAM) (CHARRANGE FAR *) ichCharRange)

#define RichEdit_FindText(hwnd, fuFlags, lpFindText) \
    (int)SendMessageA((hwnd), EM_FINDTEXT, (WPARAM) (UINT) fuFlags, (LPARAM) (FINDTEXTA FAR *) lpFindText)

#define RichEdit_FindTextEx(hwnd, fuFlags, lpFindText) \
    (int)SendMessageA((hwnd), EM_FINDTEXTEX, (WPARAM) (UINT) fuFlags, (LPARAM) (FINDTEXTEXA FAR *) lpFindText)

#define RichEdit_FindWordBreak(hwnd, code, ichStart) \
    (int)SendMessageA((hwnd), EM_FINDWORDBREAK, (WPARAM) (UINT) code, (LPARAM) (DWORD) ichStart)

#define RichEdit_FormatRange(hwnd, fRender, lpFmt) \
    (int)SendMessageA((hwnd), EM_FORMATRANGE, (WPARAM) (BOOL) fRender, (LPARAM) (FORMATRANGE FAR *) lpFmt)

#define RichEdit_GetCharFormat(hwnd, fSelection, lpFmt) \
    (DWORD)SendMessageA((hwnd), EM_GETCHARFORMAT, (WPARAM) (BOOL) fSelection, (LPARAM) (CHARFORMATA FAR *) lpFmt)

#define RichEdit_GetEventMask(hwnd) \
    (DWORD)SendMessageA((hwnd), EM_GETEVENTMASK, 0, 0L)

#define RichEdit_GetLimitText(hwnd) \
    (int)SendMessageA((hwnd), EM_GETLIMITTEXT, 0, 0L)

#define RichEdit_GetOleInterface(hwnd, ppObject) \
    (BOOL)SendMessageA((hwnd), EM_GETOLEINTERFACE, 0, (LPARAM) (LPVOID FAR *) ppObject)

#define RichEdit_GetOptions(hwnd) \
    (UINT)SendMessageA((hwnd), EM_GETOPTIONS, 0, 0L)

#define RichEdit_GetParaFormat(hwnd, lpFmt) \
    (DWORD)SendMessageA((hwnd), EM_GETPARAFORMAT, 0, (LPARAM) (PARAFORMAT FAR *) lpFmt)

#define RichEdit_GetSelText(hwnd, lpBuf) \
    (int)SendMessageA((hwnd), EM_GETSELTEXT, 0, (LPARAM) (LPSTR) lpBuf)

#define RichEdit_GetTextRange(hwnd, lpRange) \
    (int)SendMessageA((hwnd), EM_GETTEXTRANGE, 0, (LPARAM) (TEXTRANGEA FAR *) lpRange)

#define RichEdit_GetWordBreakProcEx(hwnd) \
    (EDITWORDBREAKPROCEX *)SendMessageA((hwnd), EM_GETWORDBREAKPROCEX, 0, 0L)
//----------------- End Macros Copied from windowsx.h----------------

#define RichEdit_HideSelection(hwnd, fHide, fChangeStyle) \
    (void)SendMessageA((hwnd), EM_HIDESELECTION, (WPARAM) (BOOL) fHide, (LPARAM) (BOOL) fChangeStyle)

#define RichEdit_PasteSpecial(hwnd, uFormat) \
    (void)SendMessageA((hwnd), EM_PASTESPECIAL, (WPARAM) (UINT) uFormat, 0L)

#define RichEdit_PosFromChar(hwnd, wCharIndex) \
    (DWORD)SendMessageA((hwnd), EM_POSFROMCHAR, (WPARAM)wCharIndex, 0L)

#define RichEdit_RequestResize(hwnd) \
    (void)SendMessageA((hwnd), EM_REQUESTRESIZE, 0, 0L)

#define RichEdit_SelectionType(hwnd) \
    (int)SendMessageA((hwnd), EM_SELECTIONTYPE, 0, 0L)

#define RichEdit_SetBkgndColor(hwnd, fUseSysColor, clr) \
    (COLORREF)SendMessageA((hwnd), EM_SETBKGNDCOLOR, (WPARAM) (BOOL) fUseSysColor, (LPARAM) (COLORREF) clr)

#define RichEdit_SetCharFormat(hwnd, uFlags, lpFmt) \
    (BOOL)SendMessageA((hwnd), EM_SETCHARFORMAT, (WPARAM) (UINT) uFlags, (LPARAM) (CHARFORMATA FAR *) lpFmt)

#define RichEdit_SetEventMask(hwnd, dwMask) \
    (DWORD)SendMessageA((hwnd), EM_SETEVENTMASK, 0, (LPARAM) (DWORD) dwMask)

#define RichEdit_SetOleCallback(hwnd, lpObj) \
    (BOOL)SendMessageA((hwnd), EM_SETOLECALLBACK, 0, (LPARAM) (IRichEditOleCallback FAR *) lpObj)

#define RichEdit_SetOptions(hwnd, fOperation, fOptions) \
    (UINT)SendMessageA((hwnd), EM_SETOPTIONS, (WPARAM) (UINT) fOperation, (LPARAM) (UINT) fOptions)

#define RichEdit_SetParaFormat(hwnd, lpFmt) \
    (BOOL)SendMessageA((hwnd), EM_SETPARAFORMAT, 0, (LPARAM) (PARAFORMAT FAR *) lpFmt)

#define RichEdit_SetTargetDevice(hwnd, hdcTarget, cxLineWidth) \
    (BOOL)SendMessageA((hwnd), EM_SETTARGETDEVICE, (WPARAM) (HDC) hdcTarget, (LPARAM) (int) cxLineWidth)

#define RichEdit_SetWordBreakProcEx(hwnd, pfnWordBreakProcEx) \
    (EDITWORDBREAKPROCEX *)SendMessageA((hwnd), EM_SETWORDBREAKPROCEX, 0, (LPARAM) (EDITWORDBREAKPROCEX *)pfnWordBreakProcEx)

#define RichEdit_StreamIn(hwnd, uFormat, lpStream) \
    (int)SendMessageA((hwnd), EM_STREAMIN, (WPARAM) (UINT) uFormat, (LPARAM) (EDITSTREAM FAR *) lpStream)

#define RichEdit_StreamOut(hwnd, uFormat, lpStream) \
    (int)SendMessageA((hwnd), EM_STREAMOUT, (WPARAM) (UINT) uFormat, (LPARAM) (EDITSTREAM FAR *) lpStream)

//-------------------------------------------------------------------
// Status Bar Helper Macros
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

//-------------------------------------------------------------------
// Tool Bar Helper Macros
//-------------------------------------------------------------------

#define ToolBar_AddBitmap(hwnd, nButtons, lptbab) \
    (int)SendMessageA((hwnd), TB_ADDBITMAP, (WPARAM)nButtons, (LPARAM)(LPTBADDBITMAP) lptbab)

#define ToolBar_AddButtons(hwnd, uNumButtons, lpButtons) \
    (BOOL)SendMessageA((hwnd), TB_ADDBUTTONS, (WPARAM)(UINT)uNumButtons, (LPARAM)(LPTBBUTTON)lpButtons)

#define ToolBar_AddString(hwnd, hinst, idString) \
    (int)SendMessageA((hwnd), TB_ADDSTRINGA, (WPARAM)(HINSTANCE)hinst, (LPARAM)idString)

#define ToolBar_AutoSize(hwnd) \
    (void)SendMessageA((hwnd), TB_AUTOSIZE, 0, 0L)

#define ToolBar_ButtonCount(hwnd) \
    (int)SendMessageA((hwnd), TB_BUTTONCOUNT, 0, 0L)

#define ToolBar_ButtonStructSize(hwnd) \
    (void)SendMessageA((hwnd), TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L)

#define ToolBar_ChangeBitmap(hwnd, idButton, iBitmap) \
    (BOOL)SendMessageA((hwnd), TB_CHANGEBITMAP, (WPARAM) idButton, (LPARAM)iBitmap);

#define ToolBar_CheckButton(hwnd, idButton, fCheck ) \
    (BOOL)SendMessageA((hwnd), TB_CHECKBUTTON, (WPARAM) idButton, (LPARAM) MAKELONG(fCheck, 0))

#define ToolBar_CommandToIndex(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_COMMANDTOINDEX, (WPARAM) idButton, 0L)

#define ToolBar_Customize(hwnd) \
    (void)SendMessageA((hwnd), TB_CUSTOMIZE, 0, 0L)

#define ToolBar_DeleteButton(hwnd, idButton) \
    (BOOL)SendMessageA((hwnd), TB_DELETEBUTTON, (WPARAM) idButton, 0L)

#define ToolBar_EnableButton(hwnd, idButton, fEnable ) \
    (BOOL)SendMessageA((hwnd), TB_ENABLEBUTTON, (WPARAM) idButton, (LPARAM) MAKELONG(fEnable, 0))

#define ToolBar_GetBitmap(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_GETBITMAP, (WPARAM) idButton, 0L)

#define ToolBar_GetBitmapFlags(hwnd) \
    (int)SendMessageA((hwnd), TB_GETBITMAPFLAGS, 0, 0L)

#define ToolBar_GetButton(hwnd, idButton, lpButton) \
    (BOOL)SendMessageA((hwnd), TB_GETBUTTON, (WPARAM)idButton, (LPARAM)(LPTBBUTTON) lpButton)

#define ToolBar_GetButtonText(hwnd, idButton, lpszText) \
    (int)SendMessageA((hwnd), TB_GETBUTTONTEXTA, (WPARAM) idButton, (LPARAM)(LPSTR)lpszText)

#define ToolBar_GetItemRect(hwnd, idButton, lprc) \
    (BOOL)SendMessageA((hwnd), TB_GETITEMRECT, (WPARAM)idButton, (LPARAM)(LPRECT)lprc)

#define ToolBar_GetRows(hwnd) \
    (int)SendMessageA((hwnd), TB_GETROWS, 0, 0L)

#define ToolBar_GetState(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_GETSTATE, (WPARAM) idButton, 0L)

#define ToolBar_GetToolTips(hwnd) \
    (HWND)SendMessageA((hwnd), TB_GETTOOLTIPS, 0, 0L)

#define ToolBar_HideButton(hwnd, idButton, fShow) \
    (BOOL)SendMessageA((hwnd), TB_HIDEBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fShow, 0))

#define ToolBar_Indeterminate(hwnd, idButton, fIndeterminate) \
    (BOOL)SendMessageA((hwnd), TB_INDETERMINATE, (WPARAM)idButton, (LPARAM) MAKELONG(fIndeterminate, 0))

#define ToolBar_InsertButton(hwnd, idButton, lpButton) \
    (BOOL)SendMessageA((hwnd), TB_INSERTBUTTON, (WPARAM)idButton, (LPARAM)(LPTBBUTTON)lpButton)

#define ToolBar_IsButtonChecked(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_ISBUTTONCHECKED, (WPARAM)idButton, 0L)

#define ToolBar_IsButtonEnabled(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_ISBUTTONENABLED, (WPARAM) idButton, 0L)

#define ToolBar_IsButtonHidden(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_ISBUTTONHIDDEN, (WPARAM) idButton, 0L)

#define ToolBar_IsButtonIndeterminate(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_ISBUTTONINDETERMINATE, (WPARAM) idButton, 0L)

#define ToolBar_IsButtonPressed(hwnd, idButton) \
    (int)SendMessageA((hwnd), TB_ISBUTTONPRESSED, (WPARAM) idButton, 0L

#define ToolBar_PressButton(hwnd, idButton, fPress) \
    (BOOL)SendMessageA((hwnd), TB_PRESSBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fPress, 0))

#define ToolBar_SaveRestore(hwnd, fSave, ptbsp) \
    (void)SendMessageA((hwnd), TB_SAVERESTORE, (WPARAM)(BOOL)fSave, (LPARAM)(TBSAVEPARAMSA *)ptbsp)

#define ToolBar_SetBitmapSize(hwnd, dxBitmap, dyBitmap) \
    (BOOL)SendMessageA((hwnd), TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(dxBitmap, dyBitmap))

#define ToolBar_SetButtonSize(hwnd, dxBitmap, dyBitmap) \
    (BOOL)SendMessageA((hwnd), TB_SETBUTTONSIZE, 0, (LPARAM)MAKELONG(dxBitmap, dyBitmap))

#define ToolBar_SetCmdID(hwnd, index, cmdId) \
    (BOOL)SendMessageA((hwnd), TB_SETCMDID, (WPARAM)(UINT)index, (WPARAM)(UINT)cmdId)

#define ToolBar_SetParent(hwnd, hwndParent) \
    (void)SendMessageA((hwnd), TB_SETPARENT, (WPARAM) (HWND) hwndParent, 0L)

#define ToolBar_SetRows(hwnd, cRows, fLarger, lprc) \
    (void)SendMessageA((hwnd), TB_SETROWS, (WPARAM)MAKEWPARAM(cRows, fLarger),(LPARAM)(LPRECT)lprc)

#define ToolBar_SetState(hwnd, idButton, fState) \
    (BOOL)SendMessageA((hwnd), TB_SETSTATE, (WPARAM)idButton, (LPARAM)MAKELONG(fState, 0))

#define ToolBar_SetToolTips(hwnd) \
    (void)SendMessageA((hwnd), TB_SETTOOLTIPS, (WPARAM)(HWND) hwndToolTip, 0L)

//-------------------------------------------------------------------
// Tool Tip Helper Macros
//-------------------------------------------------------------------
#define ToolTip_Activate(hwnd, fActivate) \
    (void)SendMessageA((hwnd), TTM_ACTIVATE, (WPARAM) (BOOL) fActivate, 0L)

#define ToolTip_AddTool(hwnd, lpti) \
    (BOOL)SendMessageA((hwnd), TTM_ADDTOOLA, 0, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_DelTool(hwnd, lpti) \
    (void)SendMessageA((hwnd), TTM_DELTOOLA, 0, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_EnumTools(hwnd, iTool, lpti) \
    (BOOL)SendMessageA((hwnd), TTM_ENUMTOOLSA, (WPARAM) (UINT) iTool, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_GetCurrentTool(hwnd, lpti) \
    (BOOL)SendMessageA((hwnd), TTM_GETCURRENTTOOLA, 0, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_GetText(hwnd, lpti) \
    (void)SendMessageA((hwnd), TTM_GETTEXTA, 0, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_GetToolCount(hwnd) \
    (int)SendMessageA((hwnd), TTM_GETTOOLCOUNT, 0, 0L)

#define ToolTip_GetToolInfo(hwnd, lpti) \
    (BOOL)SendMessageA((hwnd), TTM_GETTOOLINFOA, 0, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_HitText(hwnd, lphti) \
    (BOOL)SendMessageA((hwnd), TTM_HITTESTA, 0, (LPARAM) (LPHITTESTINFOA) lphti)

#define ToolTip_NewToolRect(hwnd, lpti) \
    (void)SendMessageA((hwnd), TTM_NEWTOOLRECTA, 0, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_RelayEvent(hwnd, lpmsg) \
    (void)SendMessageA((hwnd), TTM_RELAYEVENT, 0, (LPARAM) (LPMSG) lpmsg)

#define ToolTip_SetDelayTime(hwnd, uFlag, iDelay) \
    (void)SendMessageA((hwnd), TTM_SETDELAYTIME, (WPARAM) uFlag, (LPARAM) (int) iDelay)

#define ToolTip_SetToolInfo(hwnd, lpti) \
    (void)SendMessageA((hwnd), TTM_SETTOOLINFOA, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_UpdateTipText(hwnd, lpti) \
    (void)SendMessageA((hwnd), TTM_UPDATETIPTEXTA, 0, (LPARAM) (LPTOOLINFOA) lpti)

#define ToolTip_WindowFromPoint(hwnd, lppt) \
    (HWND)SendMessageA((hwnd), TTM_WINDOWFROMPOINT, 0, (POINT FAR *) lppt)

//-------------------------------------------------------------------
// Track Bar Helper Macros
//-------------------------------------------------------------------
#define TrackBar_ClearSel(hwnd, fRedraw) \
    (void)SendMessageA((hwnd), TBM_CLEARSEL, (WPARAM) (BOOL) fRedraw, 0L)

#define TrackBar_ClearTics(hwnd, fRedraw) \
    (void)SendMessageA((hwnd), TBM_CLEARTICS, (WPARAM) (BOOL) fRedraw, 0L)

#define TrackBar_GetChannelRect(hwnd, lprc) \
    (void)SendMessageA((hwnd), TBM_GETCHANNELRECT, 0, (LPARAM) (LPRECT) lprc)

#define TrackBar_GetLineSize(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETLINESIZE, 0, 0L)

#define TrackBar_GetNumTics(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETNUMTICS, 0, 0L)

#define TrackBar_GetPageSize(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETPAGESIZE, 0, 0L)

#define TrackBar_GetPos(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETPOS, 0, 0L)

#define TrackBar_GetPTics(hwnd) \
    (LPLONG)SendMessageA((hwnd), TBM_GETPTICS, 0, 0L)

#define TrackBar_GetRangeMax(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETRANGEMAX, 0, 0L)

#define TrackBar_GetRangeMin(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETRANGEMIN, 0, 0L)

#define TrackBar_GetSelEnd(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETSELEND, 0, 0L)

#define TrackBar_GetSelStart(hwnd) \
    (LONG)SendMessageA((hwnd), TBM_GETSELSTART, 0, 0L)

#define TrackBar_GetThumbLength(hwnd) \
    (UINT)SendMessageA((hwnd), TBM_GETTHUMBLENGTH, 0, 0L)

#define TrackBar_GetThumbRect(hwnd, lprc) \
    (void)SendMessageA((hwnd), TBM_GETTHUMBRECT, 0, (LPARAM) (LPRECT) lprc)

#define TrackBar_GetTic(hwnd, iTic) \
    (LONG)SendMessageA((hwnd), TBM_GETTIC, (WPARAM) (WORD) iTic, 0L)

#define TrackBar_GetTicPos(hwnd, iTic) \
    (LONG)SendMessageA((hwnd), TBM_GETTICPOS, (WPARAM) (WORD) iTic, 0L)

#define TrackBar_SetLineSize(hwnd, lLineSize) \
    (LONG)SendMessageA((hwnd), TBM_SETLINESIZE, 0, (LONG) lLineSize)

#define TrackBar_SetPageSize(hwnd, lPageSize) \
    (LONG)SendMessageA((hwnd), TBM_SETPAGESIZE, 0, (LONG) lPageSize)

#define TrackBar_SetPos(hwnd, bPosition, lPosition) \
    (void)SendMessageA((hwnd), TBM_SETPOS, (WPARAM) (BOOL) bPosition, (LPARAM) (LONG) lPosition)

#define TrackBar_SetRange(hwnd, bRedraw, lMinimum, lMaximum) \
    (void)SendMessageA((hwnd), TBM_SETRANGE, (WPARAM) (BOOL) bRedraw, (LPARAM) MAKELONG(lMinimum, lMaximum))

#define TrackBar_SetRangeMax(hwnd, bRedraw, lMaximum) \
    (void)SendMessageA((hwnd), TBM_SETRANGEMAX, (WPARAM) bRedraw, (LPARAM) lMaximum)

#define TrackBar_SetRangeMin(hwnd, bRedraw, lMinimum) \
    (void)SendMessageA((hwnd), TBM_SETRANGEMIN, (WPARAM) bRedraw, (LPARAM) lMinimum)

#define TrackBar_SetSel(hwnd, bRedraw, lMinimum, lMaximum) \
    (void)SendMessageA((hwnd), TBM_SETSEL, (WPARAM) (BOOL) bRedraw, (LPARAM) MAKELONG(lMinimum, lMaximum))

#define TrackBar_SetSelEnd(hwnd, bRedraw, lEnd) \
    (void)SendMessageA((hwnd), TBM_SETSELEND, (WPARAM) (BOOL) bRedraw, (LPARAM) (LONG) lEnd)

#define TrackBar_SetSelStart(hwnd, bRedraw, lStart) \
    (void)SendMessageA((hwnd), TBM_SETSELSTART, (WPARAM) (BOOL) bRedraw, (LPARAM) (LONG) lStart)

#define TrackBar_SetThumbLength(hwnd, iLength) \
    (void)SendMessageA((hwnd), TBM_SETTHUMBLENGTH, (WPARAM) (UINT) iLength, 0L)

#define TrackBar_SetTic(hwnd, lPosition) \
    (BOOL)SendMessageA((hwnd), TBM_SETTIC, 0, (LPARAM) (LONG) lPosition)

#define TrackBar_SetTicFreq(hwnd, wFreq, lPosition) \
    (void)SendMessageA((hwnd), TBM_SETTICFREQ, (WPARAM) wFreq, (LPARAM) (LONG) lPosition)

//-------------------------------------------------------------------
// Up / Down Control Helper Macros
//-------------------------------------------------------------------
#define UpDown_GetAccel(hwnd, cAccels, paAccels) \
    (int)SendMessageA((hwnd), UDM_GETACCEL, (WPARAM) cAccels, (LPARAM) (LPUDACCEL) paAccels)

#define UpDown_GetBase(hwnd) \
    (int)SendMessageA((hwnd), UDM_GETBASE, 0, 0L)

#define UpDown_GetBuddy(hwnd) \
    (HWND)SendMessageA((hwnd), UDM_GETBUDDY, 0, 0L)

#define UpDown_GetPos(hwnd) \
    (DWORD)SendMessageA((hwnd), UDM_GETPOS, 0, 0L)

#define UpDown_GetRange(hwnd) \
    (DWORD)SendMessageA((hwnd), UDM_GETRANGE, 0, 0L)

#define UpDown_SetAccel(hwnd, nAccels, aAccels) \
    (BOOL)SendMessageA((hwnd), UDM_SETACCEL, (WPARAM) nAccels, (LPARAM) (LPUDACCEL) aAccels)

#define UpDown_SetBase(hwnd, nBase) \
    (int)SendMessageA((hwnd), UDM_SETBASE, (WPARAM) nBase, 0L)

#define UpDown_SetBuddy(hwnd, hwndBuddy) \
    (HWND)SendMessageA((hwnd), UDM_SETBUDDY, (WPARAM) (HWND) hwndBuddy, 0L)

#define UpDown_SetPos(hwnd, nPos) \
    (short)SendMessageA((hwnd), UDM_SETPOS, 0, (LPARAM) MAKELONG((short) nPos, 0))

#define UpDown_SetRange(hwnd, nUpper, nLower) \
    (void)SendMessageA((hwnd), UDM_SETRANGE, 0, (LPARAM) MAKELONG((short) nUpper, (short) nLower))
