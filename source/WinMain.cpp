#include <Windows.h>

#include "Capow.hpp"

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpszCmdParam, int nCmdShow)
{
    return CapowWinMain(hInstance, hPrevInstance, lpszCmdParam, nCmdShow);
}
