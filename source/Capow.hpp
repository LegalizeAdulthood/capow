#ifndef CAPOW_HPP
#define CAPOW_HPP

#include <Windows.h>

int WINAPI CapowWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow);

#if defined(CAPOW_ENABLE_ALPAKA)
void ForceAlpakaCpuBackend(void);
void UpdateAlpakaDisplayType(void);
#endif

#endif
