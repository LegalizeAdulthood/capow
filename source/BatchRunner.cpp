#include "BatchRunner.hpp"

#if defined(CAPOW_ENABLE_ALPAKA)
#include "AlpakaBackend.hpp"
#endif
#include "BatchImage.hpp"
#include "Random.h"
#include "ca.hpp"

#include <Windows.h>

#include <string>

extern CAlist *calife_list;
extern HWND masterhwnd;
extern WindowBitmap *WBM;

namespace
{

int CaTypeForRule(capow::BatchRule rule)
{
    switch (rule)
    {
    case capow::BATCH_RULE_CA_HEAT_2D:
        return CA_HEAT_2D;
    case capow::BATCH_RULE_CA_WAVE_2D:
        return CA_WAVE_2D;
    }
    return CA_HEAT_2D;
}

#if defined(CAPOW_ENABLE_ALPAKA)
capow::AlpakaRule AlpakaRuleForBatchRule(capow::BatchRule rule)
{
    switch (rule)
    {
    case capow::BATCH_RULE_CA_HEAT_2D:
        return capow::ALPAKA_RULE_CA_HEAT_2D;
    case capow::BATCH_RULE_CA_WAVE_2D:
        return capow::ALPAKA_RULE_CA_WAVE_2D;
    }
    return capow::ALPAKA_RULE_CA_HEAT_2D;
}
#endif

int FocusImageWidth(CA *focus)
{
    if (focus->Getdimension() == 2)
    {
        return focus->HorzCount2D();
    }
    return focus->HorzCount();
}

int FocusImageHeight(CA *focus)
{
    if (focus->Getdimension() == 2)
    {
        return focus->VertCount2D();
    }
    return focus->VertCount();
}

bool SelectBatchBackend(const capow::BatchOptions &options)
{
#if defined(CAPOW_ENABLE_ALPAKA)
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (options.backend == capow::BATCH_BACKEND_CPU)
    {
        manager.SetBackend(capow::ALPAKA_BACKEND_CPU);
        return true;
    }

    const capow::AlpakaRule rule = AlpakaRuleForBatchRule(options.rule);
    manager.SetBackend(capow::ALPAKA_BACKEND_GPU);
    if (manager.CanRunGpu(rule))
    {
        return true;
    }
    if (!manager.IsGpuAvailable())
    {
        OutputDebugStringA("batch GPU backend is unavailable: ");
        OutputDebugStringA(manager.GetAvailabilityMessage());
        OutputDebugStringA("\n");
    }
    else
    {
        OutputDebugStringA("batch GPU backend has no path for ");
        OutputDebugStringA(capow::AlpakaRuleName(rule));
        OutputDebugStringA("\n");
    }
    return false;
#else
    if (options.backend == capow::BATCH_BACKEND_CPU)
    {
        return true;
    }
    OutputDebugStringA("batch GPU backend is not compiled\n");
    return false;
#endif
}

void LogBatchError(const std::string &error)
{
    if (!error.empty())
    {
        OutputDebugStringA(error.c_str());
        OutputDebugStringA("\n");
    }
}

} // namespace

namespace capow
{

int RunBatchMode(const BatchOptions &options)
{
    std::string error;
    if (!SelectBatchBackend(options))
    {
        return 3;
    }
    if (calife_list == 0 || WBM == 0 || masterhwnd == 0)
    {
        OutputDebugStringA("batch mode is missing application state\n");
        return 4;
    }

    HDC hdc = GetDC(masterhwnd);
    if (hdc == 0)
    {
        OutputDebugStringA("batch mode could not get window DC\n");
        return 5;
    }

    rseed(1946);
    calife_list->Changecount(1);
    calife_list->SetSleep(FALSE);

    CA *focus = calife_list->FocusCA();
    calife_list->SetCAType(focus, CaTypeForRule(options.rule), TRUE);
    focus->Setwrapflag(WF_WRAP);
    calife_list->Locate();
    focus->FourierSeed();
    focus->ResetGenerationCount();

    for (int i = 0; i < options.steps; ++i)
    {
        calife_list->Update_and_Show(hdc);
        calife_list->UpdateGenerationCount();
    }

    const bool ok = WriteBmpFromHdc(WBM->GetHDC(), focus->Minx(), focus->Miny(), FocusImageWidth(focus),
        FocusImageHeight(focus), options.output.c_str(), &error);

    ReleaseDC(masterhwnd, hdc);
    if (!ok)
    {
        LogBatchError(error);
        return 6;
    }
    return 0;
}

} // namespace capow
