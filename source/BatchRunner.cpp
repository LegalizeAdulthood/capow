#include "BatchRunner.hpp"

#if defined(CAPOW_ENABLE_ALPAKA)
#include "AlpakaBackend.hpp"
#include "AlpakaHeat1D.hpp"
#include "AlpakaHeat2D.hpp"
#include "AlpakaSyntheticHeat.hpp"
#include "AlpakaWave2D.hpp"
#endif
#include "BatchImage.hpp"
#include "Random.h"
#include "ca.hpp"

#include <Windows.h>

#include <exception>
#include <string>
#include <vector>

extern CAlist *calife_list;
extern HWND masterhwnd;

namespace
{

int CaTypeForRule(capow::BatchRule rule)
{
    switch (rule)
    {
    case capow::BATCH_RULE_SYNTHETIC_HEAT_2D:
        break;
    case capow::BATCH_RULE_CA_HEAT_2D:
        return CA_HEAT_2D;
    case capow::BATCH_RULE_CA_WAVE_2D:
        return CA_WAVE_2D;
    case capow::BATCH_RULE_CA_HEATWAVE:
        return CA_HEATWAVE;
    case capow::BATCH_RULE_CA_HEATWAVE2:
        return CA_HEATWAVE2;
    }
    return CA_HEAT_2D;
}

int WrapFlagForBatchMode(capow::BatchWrapMode wrapMode)
{
    switch (wrapMode)
    {
    case capow::BATCH_WRAP_ZERO:
        return WF_ZERO;
    case capow::BATCH_WRAP_FIXED:
        return WF_FIXED;
    case capow::BATCH_WRAP_WRAP:
        return WF_WRAP;
    case capow::BATCH_WRAP_FREE:
        return WF_FREE;
    case capow::BATCH_WRAP_ABSORB:
        return WF_ABSORB;
    }
    return WF_WRAP;
}

#if defined(CAPOW_ENABLE_ALPAKA)
capow::AlpakaRule AlpakaRuleForBatchRule(capow::BatchRule rule)
{
    switch (rule)
    {
    case capow::BATCH_RULE_SYNTHETIC_HEAT_2D:
        return capow::ALPAKA_RULE_SYNTHETIC_HEAT_2D;
    case capow::BATCH_RULE_CA_HEAT_2D:
        return capow::ALPAKA_RULE_CA_HEAT_2D;
    case capow::BATCH_RULE_CA_WAVE_2D:
        return capow::ALPAKA_RULE_CA_WAVE_2D;
    case capow::BATCH_RULE_CA_HEATWAVE:
        return capow::ALPAKA_RULE_CA_HEATWAVE;
    case capow::BATCH_RULE_CA_HEATWAVE2:
        return capow::ALPAKA_RULE_CA_HEATWAVE2;
    }
    return capow::ALPAKA_RULE_CA_HEAT_2D;
}

capow::Heat2DBoundaryMode Heat2DBoundaryModeForBatchMode(capow::BatchWrapMode wrapMode)
{
    switch (wrapMode)
    {
    case capow::BATCH_WRAP_ZERO:
        return capow::HEAT_2D_BOUNDARY_ZERO;
    case capow::BATCH_WRAP_FIXED:
        return capow::HEAT_2D_BOUNDARY_FIXED;
    case capow::BATCH_WRAP_WRAP:
        return capow::HEAT_2D_BOUNDARY_WRAP;
    case capow::BATCH_WRAP_FREE:
        return capow::HEAT_2D_BOUNDARY_FREE;
    case capow::BATCH_WRAP_ABSORB:
        return capow::HEAT_2D_BOUNDARY_ABSORB;
    }
    return capow::HEAT_2D_BOUNDARY_WRAP;
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

bool IsSyntheticBatchRule(capow::BatchRule rule)
{
    return rule == capow::BATCH_RULE_SYNTHETIC_HEAT_2D;
}

bool IsHeat2DBatchRule(capow::BatchRule rule)
{
    return rule == capow::BATCH_RULE_CA_HEAT_2D;
}

bool IsWave2DBatchRule(capow::BatchRule rule)
{
    return rule == capow::BATCH_RULE_CA_WAVE_2D;
}

bool IsHeat1DBatchRule(capow::BatchRule rule)
{
    return rule == capow::BATCH_RULE_CA_HEATWAVE || rule == capow::BATCH_RULE_CA_HEATWAVE2;
}

void LogBatchError(const std::string &error)
{
    if (!error.empty())
    {
        OutputDebugStringA(error.c_str());
        OutputDebugStringA("\n");
    }
}

#if defined(CAPOW_ENABLE_ALPAKA)
std::vector<capow::AlpakaPlaneValue> NormalizeHeat2DIntensity(
    const capow::Heat2DOptions &options, const capow::Heat2DFields &fields)
{
    std::vector<capow::AlpakaPlaneValue> normalized;
    normalized.resize(fields.intensityField.size());
    const capow::AlpakaPlaneValue scale = capow::AlpakaPlaneValue(2) * options.maxIntensity;
    for (std::size_t index = 0; index < fields.intensityField.size(); ++index)
    {
        normalized[index] = (fields.intensityField[index] + options.maxIntensity) / scale;
    }
    return normalized;
}

std::vector<capow::AlpakaPlaneValue> NormalizeWave2DIntensity(
    const capow::Wave2DOptions &options, const capow::Wave2DFields &fields)
{
    std::vector<capow::AlpakaPlaneValue> normalized;
    normalized.resize(fields.intensityField.size());
    const capow::AlpakaPlaneValue scale = capow::AlpakaPlaneValue(2) * options.maxIntensity;
    for (std::size_t index = 0; index < fields.intensityField.size(); ++index)
    {
        normalized[index] = (fields.intensityField[index] + options.maxIntensity) / scale;
    }
    return normalized;
}

std::vector<capow::AlpakaPlaneValue> NormalizeHeat1DIntensity(
    const capow::Heat1DOptions &options, const capow::Heat1DFields &fields)
{
    std::vector<capow::AlpakaPlaneValue> normalized;
    normalized.resize(fields.intensityField.size());
    const capow::AlpakaPlaneValue scale = capow::AlpakaPlaneValue(2) * options.maxIntensity;
    for (std::size_t index = 0; index < fields.intensityField.size(); ++index)
    {
        normalized[index] = (fields.intensityField[index] + options.maxIntensity) / scale;
    }
    return normalized;
}

int RunSyntheticBatchMode(const capow::BatchOptions &options, CA *focus)
{
    capow::SyntheticHeat2DOptions syntheticOptions;
    syntheticOptions.width = focus->HorzCount2D();
    syntheticOptions.height = focus->VertCount2D();
    syntheticOptions.steps = options.steps;

    try
    {
        std::vector<capow::AlpakaPlaneValue> initial;
        std::vector<capow::AlpakaPlaneValue> result;
        capow::MakeSyntheticHeat2DInitial(syntheticOptions, &initial);
        if (options.backend == capow::BATCH_BACKEND_CPU)
        {
            capow::RunSyntheticHeat2DHost(syntheticOptions, initial, &result);
        }
        else
        {
            capow::RunSyntheticHeat2DGpu(syntheticOptions, initial, &result);
        }

        std::string error;
        const bool ok = capow::WriteBmpFromIntensityPlane(
            result.data(), syntheticOptions.width, syntheticOptions.height, options.output.c_str(), &error);
        if (!ok)
        {
            LogBatchError(error);
            return 6;
        }
    }
    catch (const std::exception &exception)
    {
        OutputDebugStringA("synthetic heat batch failed: ");
        OutputDebugStringA(exception.what());
        OutputDebugStringA("\n");
        return 7;
    }

    return 0;
}

int RunHeat2DBatchMode(const capow::BatchOptions &options, CA *focus)
{
    capow::Heat2DOptions heatOptions;
    heatOptions.width = focus->HorzCount2D();
    heatOptions.height = focus->VertCount2D();
    heatOptions.steps = options.steps;
    heatOptions.boundaryMode = Heat2DBoundaryModeForBatchMode(options.wrapMode);

    try
    {
        std::vector<capow::AlpakaPlaneValue> initial;
        capow::Heat2DFields result;
        capow::MakeHeat2DInitial(heatOptions, &initial);
        if (options.backend == capow::BATCH_BACKEND_CPU)
        {
            capow::RunHeat2DHost(heatOptions, initial, &result);
        }
        else
        {
            capow::RunHeat2DGpu(heatOptions, initial, &result);
        }

        const std::vector<capow::AlpakaPlaneValue> normalized = NormalizeHeat2DIntensity(heatOptions, result);
        std::string error;
        const bool ok = capow::WriteBmpFromIntensityPlane(
            normalized.data(), heatOptions.width, heatOptions.height, options.output.c_str(), &error);
        if (!ok)
        {
            LogBatchError(error);
            return 6;
        }
    }
    catch (const std::exception &exception)
    {
        OutputDebugStringA("CA_HEAT_2D batch failed: ");
        OutputDebugStringA(exception.what());
        OutputDebugStringA("\n");
        return 7;
    }

    return 0;
}

int RunWave2DBatchMode(const capow::BatchOptions &options, CA *focus)
{
    if (options.wrapMode != capow::BATCH_WRAP_WRAP)
    {
        OutputDebugStringA("CA_WAVE_2D batch currently supports wrap mode only\n");
        return 3;
    }

    capow::Wave2DOptions waveOptions;
    waveOptions.width = focus->HorzCount2D();
    waveOptions.height = focus->VertCount2D();
    waveOptions.steps = options.steps;

    try
    {
        std::vector<capow::AlpakaPlaneValue> source;
        std::vector<capow::AlpakaPlaneValue> past;
        capow::Wave2DFields result;
        capow::MakeWave2DInitial(waveOptions, &source, &past);
        if (options.backend == capow::BATCH_BACKEND_CPU)
        {
            capow::RunWave2DHost(waveOptions, source, past, &result);
        }
        else
        {
            capow::RunWave2DGpu(waveOptions, source, past, &result);
        }

        const std::vector<capow::AlpakaPlaneValue> normalized = NormalizeWave2DIntensity(waveOptions, result);
        std::string error;
        const bool ok = capow::WriteBmpFromIntensityPlane(
            normalized.data(), waveOptions.width, waveOptions.height, options.output.c_str(), &error);
        if (!ok)
        {
            LogBatchError(error);
            return 6;
        }
    }
    catch (const std::exception &exception)
    {
        OutputDebugStringA("CA_WAVE_2D batch failed: ");
        OutputDebugStringA(exception.what());
        OutputDebugStringA("\n");
        return 7;
    }

    return 0;
}

int RunHeat1DBatchMode(const capow::BatchOptions &options, CA *focus)
{
    if (options.wrapMode != capow::BATCH_WRAP_WRAP)
    {
        OutputDebugStringA("1D heat batch currently supports wrap mode only\n");
        return 3;
    }

    capow::Heat1DOptions heatOptions;
    heatOptions.width = focus->HorzCount();
    heatOptions.steps = options.steps;
    heatOptions.rule = options.rule == capow::BATCH_RULE_CA_HEATWAVE2 ? capow::HEAT_1D_RULE_FIVE_NEIGHBOR
                                                                      : capow::HEAT_1D_RULE_THREE_NEIGHBOR;

    try
    {
        std::vector<capow::AlpakaPlaneValue> initial;
        capow::Heat1DFields result;
        capow::MakeHeat1DInitial(heatOptions, &initial);
        if (options.backend == capow::BATCH_BACKEND_CPU)
        {
            capow::RunHeat1DHost(heatOptions, initial, &result);
        }
        else
        {
            capow::RunHeat1DGpu(heatOptions, initial, &result);
        }

        const std::vector<capow::AlpakaPlaneValue> normalized = NormalizeHeat1DIntensity(heatOptions, result);
        std::string error;
        const bool ok =
            capow::WriteBmpFromIntensityPlane(normalized.data(), heatOptions.width, 1, options.output.c_str(), &error);
        if (!ok)
        {
            LogBatchError(error);
            return 6;
        }
    }
    catch (const std::exception &exception)
    {
        OutputDebugStringA(capow::BatchRuleName(options.rule));
        OutputDebugStringA(" batch failed: ");
        OutputDebugStringA(exception.what());
        OutputDebugStringA("\n");
        return 7;
    }

    return 0;
}
#endif

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
    if (calife_list == nullptr || masterhwnd == nullptr)
    {
        OutputDebugStringA("batch mode is missing application state\n");
        return 4;
    }

    rseed(options.seed);
    calife_list->Changecount(1);
    calife_list->SetSleep(FALSE);

    CA *focus = calife_list->FocusCA();
    if (focus == nullptr)
    {
        OutputDebugStringA("batch mode is missing focus CA\n");
        return 4;
    }
    if (IsSyntheticBatchRule(options.rule))
    {
#if defined(CAPOW_ENABLE_ALPAKA)
        return RunSyntheticBatchMode(options, focus);
#else
        OutputDebugStringA("synthetic heat batch rule requires Alpaka build\n");
        return 3;
#endif
    }
    if (IsHeat2DBatchRule(options.rule))
    {
#if defined(CAPOW_ENABLE_ALPAKA)
        return RunHeat2DBatchMode(options, focus);
#else
        OutputDebugStringA("CA_HEAT_2D batch rule requires Alpaka build\n");
        return 3;
#endif
    }
    if (IsWave2DBatchRule(options.rule))
    {
#if defined(CAPOW_ENABLE_ALPAKA)
        return RunWave2DBatchMode(options, focus);
#else
        OutputDebugStringA("CA_WAVE_2D batch rule requires Alpaka build\n");
        return 3;
#endif
    }
    if (IsHeat1DBatchRule(options.rule))
    {
#if defined(CAPOW_ENABLE_ALPAKA)
        return RunHeat1DBatchMode(options, focus);
#else
        OutputDebugStringA("1D heat batch rule requires Alpaka build\n");
        return 3;
#endif
    }

    HDC hdc = GetDC(masterhwnd);
    if (hdc == nullptr)
    {
        OutputDebugStringA("batch mode could not get window DC\n");
        return 5;
    }

    calife_list->SetCAType(focus, CaTypeForRule(options.rule), TRUE);
    focus->Setwrapflag(WrapFlagForBatchMode(options.wrapMode));
    calife_list->Locate();
    focus->FourierSeed();
    focus->ResetGenerationCount();

    for (int i = 0; i < options.steps; ++i)
    {
        calife_list->Update_and_Show(hdc);
        calife_list->UpdateGenerationCount();
    }

    const bool ok = WriteBmpFromHdc(hdc, focus->Minx(), focus->Miny(), FocusImageWidth(focus), FocusImageHeight(focus),
        options.output.c_str(), &error);

    ReleaseDC(masterhwnd, hdc);
    if (!ok)
    {
        LogBatchError(error);
        return 6;
    }
    return 0;
}

} // namespace capow
