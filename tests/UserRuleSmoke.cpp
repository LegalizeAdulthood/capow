#include "CapowGL.hpp"
#include "Tweakca.hpp"
#include "Userpara.hpp"
#include "ca.hpp"

#include <Windows.h>
#include <cstring>
#include <iostream>
#include <string>

BOOL toolbarON = FALSE;
BOOL statusON = FALSE;
BOOL zoomviewflag = FALSE;
HWND masterhwnd = nullptr;
HWND hDlgCycle = nullptr;
HWND hDlgFourier = nullptr;
HWND hDlgOpenGL = nullptr;
HWND hwndStatusBar = nullptr;
HINSTANCE hInst = nullptr;
int statusBarHeight = 0;
int toolBarHeight = 0;
int divider_width = 0;
int update_flag = 0;
short focusflag = 1;
char CA_STYLE_NAME[MAXCASTYLENAMESIZE] = "";
CapowGL *capowgl = nullptr;
CAlist *calife_list = nullptr;

void ForceAlpakaCpuBackend(void)
{
}

void UpdateAlpakaDisplayType(void)
{
}

void recreateUserDialog(void)
{
}

void numlabel(HWND, int, int)
{
}

void removeUserParam(CA *activeCA, BOOL removeVariance)
{
    for (std::size_t index = 0; index < activeCA->userParamAdd.size(); ++index)
    {
        if (activeCA->userParamAdd[index] != nullptr)
            delete static_cast<AdditiveTweakParam *>(activeCA->userParamAdd[index]);
    }
    activeCA->userParamAdd.erase(activeCA->userParamAdd.begin(), activeCA->userParamAdd.end());
    if (!removeVariance)
        (*activeCA->pAddUserParam)(activeCA, "Mutation Strength (0 to 1)", 0.5);
}

namespace
{
using UserInitialize = void (*)(CA *);
using UserRule1 = void (*)(CA *, int);
using UserRule3 = void (*)(CA *, int, int, int);
using UserRule5 = void (*)(CA *, int, int, int, int, int);
using UserRule9 = void (*)(CA *, int, int, int, int, int, int, int, int, int);

struct FakeCAlist
{
    FakeCAlist()
    {
        std::memset(storage, 0, sizeof(storage));
        *reinterpret_cast<HWND *>(storage) = nullptr;
    }

    CAlist *AsList()
    {
        return reinterpret_cast<CAlist *>(storage);
    }

    alignas(CAlist) unsigned char storage[sizeof(CAlist)];
};

template <typename T>
T Procedure(HMODULE library, const char *name)
{
    return reinterpret_cast<T>(GetProcAddress(library, name));
}

int SmokeRule(HMODULE library, CA *ca)
{
    const UserRule1 rule1 = Procedure<UserRule1>(library, "USERRULE_1");
    const UserRule3 rule3 = Procedure<UserRule3>(library, "USERRULE_3");
    const UserRule5 rule5 = Procedure<UserRule5>(library, "USERRULE_5");
    const UserRule9 rule9 = Procedure<UserRule9>(library, "USERRULE_9");
    const int ruleCount = (rule1 != nullptr ? 1 : 0) + (rule3 != nullptr ? 1 : 0) + (rule5 != nullptr ? 1 : 0) +
        (rule9 != nullptr ? 1 : 0);
    if (ruleCount != 1)
    {
        std::cerr << "expected exactly one USERRULE export, found " << ruleCount << "\n";
        return 1;
    }

    if (rule1 != nullptr)
        rule1(ca, 1);
    else if (rule3 != nullptr)
        rule3(ca, 1, 2, 3);
    else if (rule5 != nullptr)
        rule5(ca, CX_2D + 1, CX_2D + 2, 1, CX_2D, (2 * CX_2D) + 1);
    else
        rule9(ca, CX_2D + 1, CX_2D + 2, 2, 1, 0, CX_2D, 2 * CX_2D, (2 * CX_2D) + 1, (2 * CX_2D) + 2);
    return 0;
}
} // namespace

int main(int argc, char **argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (argc < 2)
    {
        std::cerr << "usage: user-rule-smoke <dll-path> [name]\n";
        return 1;
    }

    const std::string dllPath = argv[1];
    const std::string ruleName = argc > 2 ? argv[2] : dllPath;
    HMODULE library = LoadLibraryA(dllPath.c_str());
    if (library == nullptr)
    {
        std::cerr << "failed to load " << ruleName << ": " << GetLastError() << "\n";
        return 1;
    }

    const UserInitialize initialize = Procedure<UserInitialize>(library, "USERINITIALIZE");
    if (initialize == nullptr)
    {
        std::cerr << "missing USERINITIALIZE in " << ruleName << "\n";
        FreeLibrary(library);
        return 1;
    }

    int result = 0;
    {
        FakeCAlist list;
        CA ca(list.AsList());
        initialize(&ca);
        result = SmokeRule(library, &ca);
    }

    FreeLibrary(library);
    return result;
}
