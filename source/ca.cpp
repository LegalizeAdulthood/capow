/*******************************************************************************
    FILE:               ca.cpp
    PROJECT:            CAMCOS CAPOW!
    ENVIRONMENT:        MS Visual C++ 5.0/MS Windows 95/NT


    FILE DESCRIPTION:   This file contains CA class function defintions.

*******************************************************************************/
//====================INCLUDES===============
#include "ca.hpp"
#if defined(CAPOW_ENABLE_ALPAKA)
#include "AlpakaBackend.hpp"
#include "AlpakaDigitalLive.hpp"
#include "AlpakaHeat2DLive.hpp"
#include "AlpakaWave1DLive.hpp"
#include "Capow.hpp"
#include "CapowGL.hpp"
#include <cstdint>
#include <memory>
#include <string>
#endif
#include "CapowRules.hpp"
#include "Random.h"
#include "Tweakca.hpp"
#include "Userpara.hpp"
#include "WavePlaneImage.hpp"
#include "resource.h"
#include <math.h>
// #include <vector.h> already in ca.hpp

//====================EXTERNAL DATA===============
extern BOOL toolbarON;
extern BOOL statusON;
extern BOOL zoomviewflag;
extern HWND masterhwnd;
extern int statusBarHeight;
extern int toolBarHeight;
#if defined(CAPOW_ENABLE_ALPAKA)
extern CapowGL *capowgl;

static capow::Heat2DBoundaryMode Heat2DBoundaryModeForWrapFlag(int wrapFlag)
{
    switch (wrapFlag)
    {
    case WF_WRAP:
        return capow::HEAT_2D_BOUNDARY_WRAP;
    case WF_FREE:
        return capow::HEAT_2D_BOUNDARY_FREE;
    case WF_ABSORB:
        return capow::HEAT_2D_BOUNDARY_ABSORB;
    case WF_ZERO:
        return capow::HEAT_2D_BOUNDARY_ZERO;
    case WF_FIXED:
        return capow::HEAT_2D_BOUNDARY_FIXED;
    }
    return capow::HEAT_2D_BOUNDARY_WRAP;
}

static bool IsAlpakaHeat2DWrapFlagSupported(int wrapFlag)
{
    return wrapFlag == WF_WRAP || wrapFlag == WF_FREE || wrapFlag == WF_ABSORB || wrapFlag == WF_ZERO ||
        wrapFlag == WF_FIXED;
}

static bool IsAlpakaWave1DType(int type)
{
    return type == CA_OSCILLATOR || type == CA_DIVERSE_OSCILLATOR || type == ALT_CA_OSCILLATOR_WAVE ||
        type == ALT_CA_DIVERSE_OSCILLATOR_WAVE || type == CA_ULAM_WAVE || type == ALT_CA_ULAM_WAVE ||
        type == CA_AUTO_ULAM_WAVE || type == CA_CUBIC_ULAM_WAVE;
}

static capow::Wave1DRule AlpakaWave1DRuleForType(int type)
{
    if (type == CA_DIVERSE_OSCILLATOR)
        return capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR;
    if (type == ALT_CA_OSCILLATOR_WAVE)
        return capow::WAVE_1D_RULE_OSCILLATOR_WAVE;
    if (type == ALT_CA_DIVERSE_OSCILLATOR_WAVE)
        return capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR_WAVE;
    if (type == CA_ULAM_WAVE || type == ALT_CA_ULAM_WAVE)
        return capow::WAVE_1D_RULE_ULAM;
    if (type == CA_AUTO_ULAM_WAVE)
        return capow::WAVE_1D_RULE_AUTO_ULAM;
    if (type == CA_CUBIC_ULAM_WAVE)
        return capow::WAVE_1D_RULE_CUBIC_ULAM;
    return capow::WAVE_1D_RULE_OSCILLATOR;
}

static capow::AlpakaRule AlpakaWave1DAlpakaRuleForType(int type)
{
    if (type == CA_DIVERSE_OSCILLATOR)
        return capow::ALPAKA_RULE_CA_DIVERSE_OSCILLATOR;
    if (type == ALT_CA_OSCILLATOR_WAVE)
        return capow::ALPAKA_RULE_ALT_CA_OSCILLATOR_WAVE;
    if (type == ALT_CA_DIVERSE_OSCILLATOR_WAVE)
        return capow::ALPAKA_RULE_ALT_CA_DIVERSE_OSCILLATOR_WAVE;
    if (type == CA_ULAM_WAVE || type == ALT_CA_ULAM_WAVE)
        return capow::ALPAKA_RULE_CA_ULAM_WAVE;
    if (type == CA_AUTO_ULAM_WAVE)
        return capow::ALPAKA_RULE_CA_AUTO_ULAM_WAVE;
    if (type == CA_CUBIC_ULAM_WAVE)
        return capow::ALPAKA_RULE_CA_CUBIC_ULAM_WAVE;
    return capow::ALPAKA_RULE_CA_OSCILLATOR;
}
#endif

void AddUserParam(CA *owner, LPSTR label, Real value)
{
    TweakRange range(0.0);
    AdditiveTweakParam *param = new AdditiveTweakParam(0.0, value, 0.0, 1000.0, label, FALSE, range);
    owner->userParamAdd.push_back(param);
}

Wavecell::Wavecell() // Constructor
    :
    state(0.0),
    velocity(0.0)
{
    for (int i = 0; i < VARIABLE_COUNT; i++)
        variable[i] = 0.0;
    // Set these in the range 1.0 +- CELL_VARIANCE  and  multiply
    // them times the CA_list's corresponding fields in diverseoscillator.
    // In practice we need to reset this so that the values in
    // the corresponding wave_source_row and wave_target_row cells match
    for (int i = 0; i < CELL_PARAM_COUNT; i++) // 2017 redeclare i as int
        _cell_param[i] = 1.0 + Randomsignreal() * (VARIANCE_MEAN - VARIANCE_VARIANCE);
}

CA::CA(CAlist *mylist) :
    _heat_inc(MIN_HEAT_INC, HEAT_INC_MEAN, HEAT_INC_VARIANCE, MAX_HEAT_INC, HEAT_INC_STR, NO_ADJUST, HEAT_MULT_INC,
        TRUE, GENERAL_MIN_MULTIPLIER),
    _max_intensity(MIN_MAXINTENSITY, MAX_INTENSITY_MEAN, MAX_INTENSITY_VARIANCE, MAX_MAXINTENSITY, MAX_INTENSITY_STR,
        NO_ADJUST,
        TweakRange(
            MAX_INTENSITY_LO, MAX_INTENSITY_LO_TO_MED, MAX_INTENSITY_MED, MAX_INTENSITY_MED_TO_HI, MAX_INTENSITY_HI)),
    _max_velocity(MIN_MAXVELOCITY, MAX_VELOCITY_MEAN, MAX_VELOCITY_VARIANCE, MAX_MAXVELOCITY, MAX_VELOCITY_STR,
        NO_ADJUST,
        TweakRange(MAX_VELOCITY_LO, MAX_VELOCITY_LO_TO_MED, MAX_VELOCITY_MED, MAX_VELOCITY_MED_TO_HI, MAX_VELOCITY_HI)),
    _maxvalpercent(MIN_MAX_VALPERCENT, MAX_VALPERCENT_MEAN, MAX_VALPERCENT_VARIANCE, MAX_MAX_VALPERCENT,
        MAX_VALPERCENT_STR, NO_ADJUST,
        TweakRange(MAX_VALPERCENT_LO, MAX_VALPERCENT_LO_TO_MED, MAX_VALPERCENT_MED, MAX_VALPERCENT_MED_TO_HI,
            MAX_VALPERCENT_HI)),
    _friction_multiplier(MIN_FRICTION, FRICTION_MEAN, FRICTION_VARIANCE, MAX_FRICTION, FRICTION_STR, NO_ADJUST,
        FRICTION_MULT_INC, TRUE, GENERAL_MIN_MULTIPLIER),
    _mass(MIN_MASS, MASS_MEAN, MASS_VARIANCE, MAX_MASS, MASS_STR, NO_ADJUST, MASS_MULT_INC, FALSE),
    _dt(MIN_TIME_STEP, TIME_STEP_MEAN, TIME_STEP_VARIANCE, MAX_TIME_STEP, TIME_STEP_STR, ADJUST_ACCELERATION,
        TIME_STEP_MULT_INC, FALSE),
    _dx(MIN_SPACE_STEP, SPACE_STEP_MEAN, SPACE_STEP_VARIANCE, MAX_SPACE_STEP, SPACE_STEP_STR, ADJUST_ACCELERATION,
        SPACE_STEP_MULT_INC, FALSE),
    _spring_multiplier(MIN_SPRING, SPRING_MEAN, SPRING_VARIANCE, MAX_SPRING, SPRING_STR, NO_ADJUST,
        NONLINEARITY_MULT_INC, TRUE, NONLINEARITY_MIN_MULTIPLIER),
    _nonlinearity1(MIN_NONLINEARITY1, NONLINEARITY1_MEAN, NONLINEARITY1_VARIANCE, MAX_NONLINEARITY1, NONLINEARITY_STR,
        NO_ADJUST, NONLINEARITY_MULT_INC, TRUE, NONLINEARITY_MIN_MULTIPLIER),
    _nonlinearity2(MIN_NONLINEARITY2, NONLINEARITY2_MEAN, NONLINEARITY2_VARIANCE, MAX_NONLINEARITY2, NONLINEARITY_STR,
        NO_ADJUST, NONLINEARITY_MULT_INC, TRUE, NONLINEARITY_MIN_MULTIPLIER),
    _driver_multiplier(MIN_DRIVER_AMP, _max_intensity.Val() / 2, _max_intensity.Val() / 4, MAX_DRIVER_AMP,
        DRIVER_AMP_STR, NO_ADJUST, DRIVER_AMP_MULT_INC, TRUE, DRIVER_MIN_MULTIPLIER),
    _frequency_multiplier(MIN_DRIVER_FREQ, DRIVER_FREQ_MEAN, DRIVER_FREQ_VARIANCE, MAX_DRIVER_FREQ, DRIVER_FREQ_STR,
        ADJUST_ACCELERATION, DRIVER_FREQ_MULT_INC, TRUE, DRIVER_MIN_MULTIPLIER),
    _chunk(MIN_CHUNK, CHUNK_MEAN, CHUNK_VARIANCE, MAX_CHUNK, CHUNK_STR, NO_ADJUST, CHUNK_MULT_INC, TRUE, MIN_POS_CHUNK)

{ // enter unallocated, exit allocated, initialized.
    calist_ptr = mylist;
    /*Set this stuff before Allocate */
    hwnd = calist_ptr->hwnd;
    _generationcount = 0;
    _stretch_lasttime = FALSE; // l.andrews 11/3/01 found uninit. in smooth.cpp
    sourcerowindex = 0;
    targetrowindex = 1;
    pastrowindex = MEMORY - 1;
    wavesourceindex = 0;
    wavetargetindex = 1;
    wavepastindex = 2; // Like -1 relative to the 3 wave row buffers.
#if defined(CAPOW_ENABLE_ALPAKA)
    alpakaWave1DTextureReady = false;
    alpakaHeat2DTextureReady = false;
    alpakaDigital1DTextureReady = false;
#endif
    generatorflag = START_GENERATOR_FLAG;
    /*End of stuff needed before Allocate*/
    Allocate();
    monochromeflag = FALSE;
    band_count = START_BAND_COUNT; // l.andrews 11/2/01 moved from
                                   // below because band_count is used in setcolortable
    type_ca = 0;                   // l.andrews 11/2/01 since it will be used by Gettype()
                                   // before any other initialization
    SetColors();
    /*Set this before you do any table settings */
    lambda = START_LAMBDA;
    nabeoptions = 0;
    cellcount = 0;
    dimension = 0; // l.andrews 11/2/01 another attempt to set dimension
    radius = 0;    // So that Changeradiusandstates does something
    states = 0;
    /*End of stuff needed for table settings */
    /* Put this before you do a Seed */
    _startsmoothsteps = 0; // or Can set in Settype with basic_adjustparam
    _smoothsteps = 0;
    _smoothflag = FALSE; // Whether or not to do automatic smoothing.
    _justloadedcells = FALSE;
    viewmode = START_VIEWMODE; // like IDC_SPLIT_VIEW
    showmode = START_SHOWMODE; // like BOTH_VIEW or ODD_VIEW
    showvelocity = START_SHOWVELOCITY;
    _wavespeed = 1.0; // used by Alt??? wave methods.
    _dx_lock = TRUE;
    time = 0.0;
    _phase = Randomsignreal() * 2.0 * PI;
    Adjust_acceleration_multiplier();
    // sets _dt_over_2 and accleration_multiplier and frequency_factor
    /* End of stuff needed for seed */
    Changeradiusandstates(START_RADIUS, START_STATES);
    /*Do after Allocate, because it calls Lambdalookup, Seed, Resetfreq,
        Setcolortable */
    type_ca = 0;            // Use 0 for a no-good type_ca.
    _castyle = 0;           // Use 0 for a no-good castyle.
    maxx = minx = 0;        // l.andrews 11/2/01 my guess at how to start up - it's kind of silly
    Settype(START_TYPE_CA); // This also seeds if type_ca is 0.
    wrapflag = START_WRAPFLAG;
    blankedgeflag = 0; // This variable is not currently used for anything
    showvelocity = START_SHOWVELOCITY;
    entropy = 0.0;
    target_entropy = START_TARGET_ENTROPY;
    entropyflag = START_ENTROPYFLAG;
    entropy_bonus = START_ENTROPY_BONUS;
    fail_stripe = START_FAILSTRIPE_SCORE;
    score = 0.0;
    // DLL stuff
    _DLLhandle = NULL; // Currently loaded DLL
    _lpfnUSERNABESIZE = NULL;
    _lpfnUSERCASTYLE = NULL;
    _lpfnUSERRULE_1 = NULL;
    _lpfnUSERRULE_3 = NULL;
    _lpfnUSERRULE_5 = NULL;
    _lpfnUSERRULE_9 = NULL;
    _usernabesize = 3;         // means use a three argument 1D update rule.
    _usercastyle = CA_WAVE_2D; // means that if you ever have _user_nabesize
                               // set for 5 args, do as a 2D rule.
    lstrcpyA(_userrulename, "");

    //======== Bang-Nguyen ========
    Fourier_init();

    //=============================
    pAddUserParam = AddUserParam;                          // Set the member function pointer to the global function.
    AddUserParam(this, "Mutation Strength (0 to 1)", 0.2); // Formerly called "Variance"
    userParamAdd[0]->SetRange(0.0, 1.0);
    userParamAdd[0]->SetVal(0.2); // Just to be sure.
}

void CA::Allocate()
{
#ifdef FIXED_640_480
    _max_horz_count = 640;
#else  // not FIXED_640_480
    _max_horz_count = GetSystemMetrics(SM_CXSCREEN);
#endif // FIXED_640_480
    int i;
    generator_ptr = new Generator(this);
    generatorlist.SetPtr(this);
    lookup = (unsigned char *) (new char[MAXNABEOPTIONS]);
    if (!lookup)
    {
        MessageBoxA(
            hwnd, (LPSTR) "Failure in Lookup Allocation!", (LPSTR) "Memory Problems!", MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }
    freqlookup = (unsigned short *) (new int[MAXNABEOPTIONS]);
    if (!freqlookup)
    {
        MessageBoxA(
            hwnd, (LPSTR) "Failure in Freqlookup Allocation!", (LPSTR) "Memory Problems!", MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }

    for (i = 0; i < MEMORY; i++)
    {
        rowbuffer[i] = (unsigned char *) new char[_max_horz_count];
        for (int j = 0; j < _max_horz_count; ++j)
            rowbuffer[i][j] = '\0';
        if (!rowbuffer[i])
        {
            MessageBoxA(hwnd, (LPSTR) "Failure in Rowbuffer Allocation!", (LPSTR) "Memory Problems!",
                MB_OK | MB_ICONEXCLAMATION);
            SendMessage(hwnd, WM_DESTROY, 0, 0L);
            return;
        }
    }
    past_row = rowbuffer[pastrowindex];
    source_row = rowbuffer[sourcerowindex];
    target_row = rowbuffer[targetrowindex];
    for (i = 0; i < 3; i++)
    {
        waverowbuffer[i] = new Wavecell[_max_horz_count];
        if (!waverowbuffer[i])
        {
            MessageBoxA(
                hwnd, (LPSTR) "Failure in Waverow Allocation!", (LPSTR) "Memory Problems!", MB_OK | MB_ICONEXCLAMATION);
            SendMessage(hwnd, WM_DESTROY, 0, 0L);
            return;
        }
    }
    wave_past_row = waverowbuffer[wavepastindex];
    wave_source_row = waverowbuffer[wavesourceindex];
    wave_target_row = waverowbuffer[wavetargetindex];
    // 2D stuff
    for (i = 0; i < 3; i++)
    {
        waveplanebuffer[i] = new Wavecell2[(int) CX_2D * CY_2D];
        if (!waveplanebuffer[i])
        {
            MessageBoxA(hwnd, (LPSTR) "Failure in Waveplane Allocation!", (LPSTR) "Memory Problems!",
                MB_OK | MB_ICONEXCLAMATION);
            SendMessage(hwnd, WM_DESTROY, 0, 0L);
            return;
        }
    }
    wave_past_plane = waveplanebuffer[wavepastindex];
    wave_source_plane = waveplanebuffer[wavesourceindex];
    wave_target_plane = waveplanebuffer[wavetargetindex];

    // Sets the _cell_param fields.
    // Be careful not to do this until wave_source_row and wave_target_row
    // are all set.
    // Now we can initialize variance.
    _variance = MultiplicativeTweakParam(MIN_VARIANCE, VARIANCE_MEAN, VARIANCE_VARIANCE, MAX_VARIANCE, VARIANCE_STR,
        ADJUST_VARIANCE, VARIANCE_MULT_INC, FALSE);
    Smooth_variance();

    colortable = (COLORREF *) (new COLORREF[MAX_COLOR]);
    if (!colortable)
    {
        MessageBoxA(
            hwnd, (LPSTR) "Failure in ColorTable Allocation!", (LPSTR) "Memory Problems!", MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }
    _anchor_color = new COLORREF[MAX_BAND_COUNT + 1];
    if (!_anchor_color)
    {
        MessageBoxA(
            hwnd, (LPSTR) "Failure in AnchorColor Allocation!", (LPSTR) "Memory Problems!", MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }

    COLORREF_target_row = new COLORREF[_max_horz_count];
    if (!COLORREF_target_row)
    {
        MessageBoxA(hwnd, (LPSTR) "Failure in COLORREF_target_row allocation!", (LPSTR) "Memory Problems!",
            MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }

    colorindex_target_row = new unsigned short[_max_horz_count];
    if (!colorindex_target_row)
    {
        MessageBoxA(hwnd, (LPSTR) "Failure in colorindex_target_row Allocation!", (LPSTR) "Memory Problems!",
            MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }
    tp_real_array = new Real[4 * _max_horz_count];
    if (!tp_real_array)
    {
        MessageBoxA(hwnd, (LPSTR) "Failure in tp_real_array Allocation!", (LPSTR) "Memory Problems!",
            MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }
    fourier_a = new Real[MAXTERM];
    if (!fourier_a)
    {
        MessageBoxA(
            hwnd, (LPSTR) "Failure in fourier_a Allocation!", (LPSTR) "Memory Problems!", MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }
    fourier_b = new Real[MAXTERM];
    // to contain the Fourier coefficients
    if (!fourier_b)
    {
        MessageBoxA(
            hwnd, (LPSTR) "Failure in fourier_b Allocation!", (LPSTR) "Memory Problems!", MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }
    fourier_approx = new Real[2 * _max_horz_count];
    // approximate values by Fourier series
    if (!fourier_approx)
    {
        MessageBoxA(hwnd, (LPSTR) "Failure in fourier_approx Allocation!", (LPSTR) "Memory Problems!",
            MB_OK | MB_ICONEXCLAMATION);
        SendMessage(hwnd, WM_DESTROY, 0, 0L);
        return;
    }
}

CA::~CA()
{
    int i;

    delete generator_ptr;
    delete[] lookup;
    delete[] freqlookup;
    for (i = 0; i < MEMORY; i++)
        delete[] rowbuffer[i];
    for (i = 0; i < 3; i++)
        delete[] waverowbuffer[i];
    for (i = 0; i < 3; i++)
        delete[] waveplanebuffer[i];
    delete[] colortable;
    delete[] colorindex_target_row;
    delete[] COLORREF_target_row;
    delete[] _anchor_color;
    delete[] tp_real_array;
    delete[] fourier_a;
    delete[] fourier_b;
    delete[] fourier_approx;
    removeUserParam(this);

    if (_DLLhandle)
        FreeLibrary(_DLLhandle);
}

void CA::Lambdalookup()
{ // We use this to randomize both digital and analog CAs.
    // Should switch in general, though the very first time you call this
    // you probably want to do both.
    for (unsigned short i = 0; i < nabeoptions; i++)
    {
        if (Randomreal() < lambda)
            lookup[i] = (unsigned char) (1 + Random((unsigned short) (states - 1)));
        else
            lookup[i] = 0;
    }
    //---CA_wave
    RandomizeTweakParam(&_dt, TIME_STEP_MEAN, TIME_STEP_VARIANCE);
    RandomizeTweakParam(&_dx, SPACE_STEP_MEAN, SPACE_STEP_VARIANCE);
    RandomizeTweakParam(&_mass, MASS_MEAN, MASS_VARIANCE);
    RandomizeTweakParam(&_friction_multiplier, FRICTION_MEAN, FRICTION_VARIANCE);
    RandomizeTweakParam(&_spring_multiplier, SPRING_MEAN, SPRING_VARIANCE);
    RandomizeTweakParam(&_frequency_multiplier, DRIVER_FREQ_MEAN, DRIVER_FREQ_VARIANCE);
    RandomizeTweakParam(&_max_intensity, MAX_INTENSITY_MEAN, MAX_INTENSITY_VARIANCE);
    RandomizeTweakParam(&_driver_multiplier, _max_intensity.Val() / 2, _max_intensity.Val() / 4);
    RandomizeTweakParam(&_max_velocity, MAX_VELOCITY_MEAN, MAX_VELOCITY_VARIANCE);
    Resetfreq();
    Computeactual_lambda();
    if (calist_ptr->FocusCA() == this)
        update_flag = 1; // need to update dialog boxs
}

void CA::Resetfreq()
{
    cellcount = 0;
    for (unsigned short i = 0; i < nabeoptions; i++)
        freqlookup[i] = 0;
}
void CA::SyncRows()
{
    /* This is to correct the fact that we have 16 rows and only
        row_buffer[0] gets seeded with all 640 cells. So if the size
        changes and we aren't at row_buffer[0], we need to copy what we
        currently have back to row_buffer[0]. Therefore row_buffer[0]
        acts as a kind of memory when we change the size around. */

    // First, see if we are at row_buffer[0] in the first place
    if (source_row != rowbuffer[0])
    {
        // If not, copy what we have to row_buffer[0]
        for (short i = 0; i < horz_count; i++)
        {
            rowbuffer[0][i] = source_row[i];
            rowbuffer[MEMORY - 1][i] = past_row[i];
        }
        // Now reset everything to the top
        source_row = rowbuffer[0];
        sourcerowindex = 0;
        target_row = rowbuffer[1];
        targetrowindex = 1;
        past_row = rowbuffer[MEMORY - 1];
        pastrowindex = MEMORY - 1;
    }
    // See if we are at waverowbuffer[0]
    if (wave_source_row != waverowbuffer[0])
    {
        // If not, copy what we have to waverowbuffer[0]
        for (short i = 0; i < horz_count; i++)
        {
            waverowbuffer[0][i].intensity = wave_source_row[i].intensity;
            waverowbuffer[0][i].velocity = wave_source_row[i].velocity;
            waverowbuffer[2][i].intensity = wave_past_row[i].intensity;
        }
        // Now reset everything to the top
        wave_source_row = waverowbuffer[0];
        wavesourceindex = 0;
        wave_target_row = waverowbuffer[1];
        wavetargetindex = 1;
        wave_past_row = waverowbuffer[2];
        wavepastindex = 2;
    }
    if (viewmode == IDC_SPLIT_VIEW)
        row_number = splity - (calist_ptr->_blt_lines) + 1;
    if (viewmode == IDC_SCROLL_VIEW)
        row_number = maxy - (calist_ptr->_blt_lines) + 1;
}

/* fourier-flag of focus CA is 0, but fourier-flag  of others may not. Check
 fourier-flag so that we don't have to recompute SFT */
void CA::Locate(int tile, HWND hwnd, int CA_count_per_edge)
{
    int i, j;
    RECT windowrect;

    GetClientRect(hwnd, &windowrect);
#ifdef FORCENARROW
    windowrect.right = FORCEXSIZE; // FORCEXSIZE is defined in CA.HPP.
                                   //  Used 64 for most of the border pix.  32 for
                                   //"reversible circuit 2x CA" for illo for chapter 4 of my
                                   // LIFEBOX book.  Used 64 for FLURB.
#endif                             // FORCENARROW
#ifndef FIT_STATUS_BAR
    Locate(tile, windowrect.right, windowrect.bottom, CA_count_per_edge);
#else  // do new FIT_STATUS_BAR way
    tile_number = tile;
    i = tile % CA_count_per_edge;
    j = tile / CA_count_per_edge;
    windowrect.bottom -= 1 + ((statusON) ? statusBarHeight : 0);
    windowrect.bottom -= (toolbarON) ? toolBarHeight : 0;
    windowrect.right--;
    if (CA_count_per_edge != 1)
    {
        windowrect.bottom -= 2; // subtract the perimeter gap, 1 unit for both edges
        windowrect.right -= 2;  // subtract the perimeter gap, 1 unit for both edges
    }
    miny = (int) ((float) j * (windowrect.bottom + BORDER) / CA_count_per_edge);
    maxy = (int) ((float) (j + 1) * (windowrect.bottom + BORDER) / CA_count_per_edge) - BORDER;
    miny += (toolbarON) ? toolBarHeight : 0;
    maxy += (toolbarON) ? toolBarHeight : 0;
    if (CA_count_per_edge != 1)
    {
        miny += 1; // to adjust for the perimeter gap
        maxy += 1; // to adjust for the perimeter gap
    }
    minx = (int) ((float) i * (windowrect.right + BORDER) / CA_count_per_edge);
    maxx = (int) ((float) (i + 1) * (windowrect.right + BORDER) / CA_count_per_edge) - BORDER;
    if (CA_count_per_edge != 1)
    {
        minx += 1; // to adjust for the perimeter gap;
        maxx += 1; // to adjust for the perimeter gap;
    }
    splity = SPLIT_SCROLL_PROPORTION * (maxy - miny); // Like cheight.
    splity =
        (splity / (calist_ptr->_blt_lines)) * (calist_ptr->_blt_lines); // Make a multiple of (calist_ptr->_blt_lines)
    splity = miny + splity - 1;

    horz_count_2D = horz_count = maxx - minx + 1;
    vert_count_2D = vert_count = maxy - miny + 1;
    split_vert_count = maxy - splity;

    if (horz_count_2D > CX_2D)
        horz_count_2D = CX_2D;
    if (vert_count_2D > CY_2D)
        vert_count_2D = CY_2D;
    maxx_2D = minx + horz_count_2D;
    maxy_2D = miny + vert_count_2D;

    switch (viewmode)
    {
    case IDC_DOWN_VIEW:
        row_number = miny;
        break;

    case IDC_SCROLL_VIEW:
        row_number = maxy - (calist_ptr->_blt_lines) + 1;
        break;

    case IDC_SPLIT_VIEW:
        row_number = splity - (calist_ptr->_blt_lines) + 1;
        break;

    default:
        row_number = miny;
        break;
    }
    Setwrapflag(wrapflag); /*Calling this makes you fix the edges
        (set to zero if wrapflag FALSSE and blankedgeflag TRUE),
        and  do a Reset_smoothsteps() or a Smoothedge?D call.*/
    /* we need to reset the counter here so that it will be in sync
        with the advancement of rownumber */
    SyncRows();
#endif // FIT_STATUS_BAR
}

void CA::Locate(int itile_number, int dmaxx, int dmaxy, int CA_count_per_edge)
{
    int dwidth, dheight, cwidth, cheight; // display and cell window sizes
    int xpos, ypos;

    /* move all rows back to default start position, but retain all the current
        information in them, so if we are increasing the size, we have the
        rest of the seeded rowbuffer[0] to look at initially */

    tile_number = itile_number;
    dwidth = dmaxx - 2; // why???
    dheight = dmaxy - 2;
    cwidth = (int) (((float) dwidth - (BORDER * (CA_count_per_edge - 1))) / CA_count_per_edge);
    cheight = (int) (((float) dheight - (BORDER * (CA_count_per_edge - 1))) / CA_count_per_edge);

    // Make it a multiple of (calist_ptr->_blt_lines)
    cheight = (cheight / (calist_ptr->_blt_lines)) * (calist_ptr->_blt_lines);

    xpos = tile_number % CA_count_per_edge;
    ypos = tile_number / CA_count_per_edge;

    minx = xpos * (cwidth + BORDER);
    miny = ypos * (cheight + BORDER) + ((toolbarON) ? toolBarHeight : 0);
    maxx = minx + cwidth;
    maxy = miny + cheight;
    splity = SPLIT_SCROLL_PROPORTION * (maxy - miny); // Like cheight.
    splity =
        (splity / (calist_ptr->_blt_lines)) * (calist_ptr->_blt_lines); // Make a multiple of (calist_ptr->_blt_lines)
    splity = miny + splity - 1;
    horz_count = maxx - minx + 1;
    vert_count = maxy - miny + 1;
    split_vert_count = maxy - splity + 1;
    horz_count_2D = horz_count;
    vert_count_2D = vert_count;
    if (horz_count_2D > CX_2D)
        horz_count_2D = CX_2D;
    if (vert_count_2D > CY_2D)
        vert_count_2D = CY_2D;
    maxx_2D = minx + horz_count_2D - 1;
    maxy_2D = miny + vert_count_2D - 1;
    //---------- Fourier ---------
    Reset_tp_all();

    switch (viewmode)
    {
    case IDC_DOWN_VIEW:
        row_number = miny;
        break;

    case IDC_SCROLL_VIEW:
        row_number = maxy - (calist_ptr->_blt_lines) + 1;
        break;

    case IDC_SPLIT_VIEW:
        row_number = splity - (calist_ptr->_blt_lines) + 1;
        break;

    default:
        row_number = miny;
        break;
    }
    Setwrapflag(wrapflag); /*Calling this makes you fix the edges
        (set to zero if wrapflag FALSSE and blankedgeflag TRUE),
        and  do a Reset_smoothsteps() or a Smoothedge?D call.*/
    /* we need to reset the counter here so that it will be in sync
        with the advancement of rownumber */
    SyncRows();
}

void CA::StandardUpdate(HDC hdc)
{
#if defined(CAPOW_ENABLE_ALPAKA)
    if (TryAlpakaDigitalUpdate(hdc))
        return;
#endif
    int i, leftindex;
    unsigned short nabe = 0;

    if (wrapflag == WF_WRAP)
        for (i = horz_count - radius; i < horz_count; i++)
        {
            nabe |= source_row[i];
            nabe <<= statebits;
        }
    if (wrapflag == WF_FREE) // Acts as if multile copies of first cell to left
        for (i = 0; i < radius; i++)
        {
            nabe |= source_row[0];
            nabe <<= statebits;
        }
    // if wrapflag == WF_ZERO or WF_FIXED, you leave zeroes in the nabe.
    for (i = 0; i < radius; i++)
    {
        nabe |= source_row[i];
        nabe <<= statebits;
    }
    nabe |= source_row[radius];
    target_row[0] = lookup[nabe];
    if (entropyflag)
    {
        cellcount++;
        freqlookup[nabe]++;
    }
    for (i = 1; i < horz_count - radius; i++)
    {
        nabe <<= statebits;
        nabe &= mask;
        nabe |= source_row[i + radius];
        target_row[i] = lookup[nabe];
        if (entropyflag)
        {
            cellcount++;
            freqlookup[nabe]++;
        }
    }
    if (wrapflag == WF_WRAP)
    {
        leftindex = 0;
        for (i = horz_count - radius; i < horz_count; ++i)
        {
            nabe <<= statebits;
            nabe &= mask;
            nabe |= source_row[leftindex];
            target_row[i] = lookup[nabe];
            if (entropyflag)
            {
                cellcount++;
                freqlookup[nabe]++;
            }
            leftindex++;
        }
    }
    else if (wrapflag == WF_ZERO || wrapflag == WF_FIXED) // not wrapflag
        for (i = horz_count - radius; i < horz_count; ++i)
        {
            nabe <<= statebits;
            nabe &= mask;
            target_row[i] = lookup[nabe];
            if (entropyflag)
            {
                cellcount++;
                freqlookup[nabe]++;
            }
        } // end wrapflag if else.
    else // WF_FREE case acts as if multipe copies of last cell to right
        for (i = horz_count - radius; i < horz_count; ++i)
        {
            nabe <<= statebits;
            nabe &= mask;
            nabe |= source_row[horz_count - 1];
            target_row[i] = lookup[nabe];
            if (entropyflag)
            {
                cellcount++;
                freqlookup[nabe]++;
            }
        }

    if (generatorflag)
        generator_ptr->Step();
    generatorlist.Step();
    /* We let the generator, if it's on, write to the target row.  This
    way you see the generator value, and this value is used as source
    after the rows are rolled just below.*/
    for (i = 0; i < horz_count; i++)
    {
        colorindex_target_row[i] = target_row[i]; // Just put some good value
        // into the colorindex_target_row in case you suddenly switch to being
        // a wave.  Standard doesn't USE colorindex_target_row, though, I don't think.
        //  This doesn't seem to fix the bug by the way, so is a waste of time.
        COLORREF_target_row[i] = colortable[target_row[i]];
    }
    Show(hdc);
}

void CA::EnsureHistoryImage()
{
    const int imageWidth = maxx - minx + 1;
    const int imageHeight = maxy - miny + 1;
    if (historyImage.Width() != imageWidth || historyImage.Height() != imageHeight)
        historyImage.Resize(imageWidth, imageHeight);
}

void CA::ClearHistoryImage()
{
    EnsureHistoryImage();
    historyImage.Clear(capow::ImageBuffer::Pixel(0xFF000000U));
}

void CA::ClearDisplayImages()
{
    const capow::ImageBuffer::Pixel black = capow::ImageBuffer::Pixel(0xFF000000U);

    if (wavePlaneImage.Data() != nullptr)
        wavePlaneImage.Clear(black);

    if (viewmode == IDC_2D_VIEW)
    {
        if (historyImage.Data() != nullptr)
            historyImage.Clear(black);
    }
    else
    {
        ClearHistoryImage();
    }
}

void CA::PutHistoryPixel(int x, int y, COLORREF color)
{
    historyImage.PutPixel(x - minx, y - miny, capow::ImagePixelFromColorRef(color));
}

void CA::FillHistoryRect(int left, int top, int right, int bottom, COLORREF color)
{
    historyImage.FillRect(
        left - minx, top - miny, right - left + 1, bottom - top + 1, capow::ImagePixelFromColorRef(color));
}

void CA::ScrollHistoryRect(int left, int top, int right, int bottom, int deltaX, int deltaY)
{
    historyImage.ScrollRect(left - minx, top - miny, right - left + 1, bottom - top + 1, deltaX, deltaY);
}

void CA::Show(HDC)
{
    int i;
    const COLORREF black = RGB(0, 0, 0);
    // Convert target_row values to COLORREF values.  For the Standard
    //(digital) CAs, the values will be unsigned char, for the Wave
    //(analog) CAs, the values will be long int.

    // Store the target row values into the bitmap.
    switch (viewmode)
    {
    case IDC_DOWN_VIEW:
        EnsureHistoryImage();
        for (i = 0; i < horz_count; ++i)
            PutHistoryPixel(minx + i, row_number, COLORREF_target_row[i]);
        row_number++;
        if (row_number > maxy)
            row_number = miny;
        break;
    case IDC_SCROLL_VIEW:
        EnsureHistoryImage();
        // Do the bump first, if needed, so the image looks good.
        if (row_number == maxy - (calist_ptr->_blt_lines) + 1)
            ScrollHistoryRect(minx, miny, maxx, maxy, 0, -(calist_ptr->_blt_lines));
        for (i = 0; i < horz_count; ++i)
            PutHistoryPixel(minx + i, row_number, COLORREF_target_row[i]);
        row_number++; // Starts at maxy - (calist_ptr->_blt_lines) + 1
        if (row_number > maxy)
            row_number = maxy - (calist_ptr->_blt_lines) + 1;
        break;
    case IDC_WIRE_VIEW:
        break;
    case IDC_GRAPH_VIEW:
        EnsureHistoryImage();
        FillHistoryRect(minx, miny, maxx, maxy, black);
        break;

    case IDC_POINT_GRAPH: //===== 3/15/96 - Bang-Nguyen =====
        UpdatePointGraph();
        break;

    case IDC_SPLIT_VIEW:
        EnsureHistoryImage();
        // Put scroll part in top half.
        // COPY the IDC_SCROLL_VIEW with splity for maxy.
        if (row_number == splity - (calist_ptr->_blt_lines) + 1)
            ScrollHistoryRect(minx, miny, maxx, splity, 0, -(calist_ptr->_blt_lines));
        for (i = 0; i < horz_count; ++i)
            PutHistoryPixel(minx + i, row_number, COLORREF_target_row[i]);
        row_number++; // Starts at splity - (calist_ptr->_blt_lines) + 1
        if (row_number > splity)
            row_number = splity - (calist_ptr->_blt_lines) + 1;
        // Put graph part in bottom half.  Put splity for miny.
        // Put vert_count/2 for vert_count.

        FillHistoryRect(minx, splity + 1, maxx, maxy, black);
        break;
    }
    // Roll forward the source and target buffers.
    if (++sourcerowindex >= MEMORY)
        sourcerowindex = 0;
    if (++targetrowindex >= MEMORY)
        targetrowindex = 0;
    source_row = rowbuffer[sourcerowindex]; // same as target_row, actually.
    target_row = rowbuffer[targetrowindex];
    if (cellcount > (calist_ptr->breedcycle * horz_count))
        /*could overflow one of the freqlookup entries in the next row */

        Entropy();
    if (sourcerowindex == MEMORY - 1)
        Avoidstripes();
}

void CA::ReversibleUpdate(HDC hdc)
{
#if defined(CAPOW_ENABLE_ALPAKA)
    if (TryAlpakaDigitalUpdate(hdc))
        return;
#endif
    int i, leftindex;
    unsigned short nabe = 0;
    unsigned char statesmask;

    statesmask = (unsigned char) (states - 1); // assume states is a power of two
    if (wrapflag == WF_WRAP)
        for (i = horz_count - radius; i < horz_count; i++)
        {
            nabe |= source_row[i];
            nabe <<= statebits;
        }
    if (wrapflag == WF_FREE) // Acts as if multile copies of first cell to left
        for (i = 0; i < radius; i++)
        {
            nabe |= source_row[0];
            nabe <<= statebits;
        }
    // if wrapflag == WF_ZERO or WF_FIXED, you leave zeroes in the nabe.
    for (i = 0; i < radius; i++)
    {
        nabe |= source_row[i];
        nabe <<= statebits;
    }
    nabe |= source_row[radius];
    target_row[0] = (unsigned char) ((lookup[nabe] + states - past_row[0]) & statesmask);
    /* This is for reversibility.  The idea is that Newstate =
        Computedstate - Paststate MOD states.  To stay away from
        negatives, we add states - Paststate.  To do a fast MOD,
        we and with statesmask (which is states-1).  Note there
        could be a problem here if states is over 128.*/
    if (entropyflag)
    {
        cellcount++;
        freqlookup[nabe]++;
    }
    for (i = 1; i < horz_count - radius; i++)
    {
        nabe <<= statebits;
        nabe &= mask;
        nabe |= source_row[i + radius];
        target_row[i] = (unsigned char) ((lookup[nabe] + states - past_row[i]) & statesmask);
        if (entropyflag)
        {
            cellcount++;
            freqlookup[nabe]++;
        }
    }
    if (wrapflag == WF_WRAP)
    {
        leftindex = 0;
        for (i = horz_count - radius; i < horz_count; ++i)
        {
            nabe <<= statebits;
            nabe &= mask;
            nabe |= source_row[leftindex];
            target_row[i] = (unsigned char) ((lookup[nabe] + (states - past_row[i])) & statesmask);
            if (entropyflag)
            {
                cellcount++;
                freqlookup[nabe]++;
            }
            leftindex++;
        }
    }
    else if (wrapflag == WF_ZERO || wrapflag == WF_FIXED) // not wrapflag
        for (i = horz_count - radius; i < horz_count; ++i)
        {
            nabe <<= statebits;
            nabe &= mask;
            target_row[i] = (unsigned char) ((lookup[nabe] + (states - past_row[i])) & statesmask);
            if (entropyflag)
            {
                cellcount++;
                freqlookup[nabe]++;
            }
        }
    else // WF_FREE case acts as if multipe copies of last cell to right
        for (i = horz_count - radius; i < horz_count; ++i)
        {
            nabe <<= statebits;
            nabe &= mask;
            nabe |= source_row[horz_count - 1];
            target_row[i] = (unsigned char) ((lookup[nabe] + (states - past_row[i])) & statesmask);
            if (entropyflag)
            {
                cellcount++;
                freqlookup[nabe]++;
            }
        }

    if (++pastrowindex >= MEMORY) // for reversible_flag
        pastrowindex = 0;
    past_row = rowbuffer[pastrowindex];

    if (generatorflag)
        generator_ptr->Step();
    generatorlist.Step();
    /* We let the generator, if it's on, write to the target row.  This
    way you see the generator value, and this value is used as source
    after the rows are rolled just below.*/
    for (i = 0; i < horz_count; i++)
    {
        colorindex_target_row[i] = target_row[i]; // Just put some good value
        // into the colorindex_target_row in case you suddenly switch to being
        // a wave.  Standard doesn't USE colorindex_target_row, though, I don't think.
        COLORREF_target_row[i] = colortable[target_row[i]];
    }

    Show(hdc);
}

void CA::WaveUpdate(HDC hdc)
{
#if defined(CAPOW_ENABLE_ALPAKA)
    if (TryAlpakaWave1DUpdate(hdc))
        return;
#endif

    if (!_smoothsteps) // Normal situation
    {
        for (short i = 1; i < horz_count - 1; i++)
            (this->*UpdateCell_3)(i - 1, i, i + 1);
        // Do the same thing at the ends if wrapflag, else set to 0.0.
        if (wrapflag == WF_WRAP)
        {
            // left end update
            (this->*UpdateCell_3)(horz_count - 1, 0, 1);
            // right end update
            (this->*UpdateCell_3)(horz_count - 2, horz_count - 1, 0);
        }
        else if (wrapflag == WF_FREE)
        // Act as if there is an identical cell to left or right
        {
            // left end update
            (this->*UpdateCell_3)(0, 0, 1);
            // right end update
            (this->*UpdateCell_3)(horz_count - 2, horz_count - 1, horz_count - 1);
        }
        else if (wrapflag == WF_ABSORB) // mike 11-18-97
        {
            // target value is the source value of its inner neighbor
            wave_target_row[0].intensity = wave_source_row[1].intensity;
            wave_target_row[horz_count - 1].intensity = wave_source_row[horz_count - 2].intensity;
        }
        // else WF_ZERO or WF_FIXED or WF_ABSORB means you don't update it.
    }
    else //_smoothsteps not 0.
        SmoothAverageStretch1D();
    WaveUpdateStep(hdc);
}

void CA::WaveUpdate_5(HDC hdc)
{
    for (short i = 2; i < horz_count - 2; i++)
        (this->*UpdateCell_5)(i - 2, i - 1, i, i + 1, i + 2);
    // Do the same thing at the ends if wrapflag, else set to 0.0.
    if (wrapflag == WF_WRAP)
    {
        // left end update
        (this->*UpdateCell_5)(horz_count - 2, horz_count - 1, 0, 1, 2);
        (this->*UpdateCell_5)(horz_count - 1, 0, 1, 2, 3);
        // right end update
        (this->*UpdateCell_5)(horz_count - 3, horz_count - 2, horz_count - 1, 0, 1);
        (this->*UpdateCell_5)(horz_count - 4, horz_count - 3, horz_count - 2, horz_count - 1, 0);
    }
    else if (wrapflag == WF_FREE)
    {
        // left end update
        (this->*UpdateCell_5)(0, 0, 0, 1, 2);
        (this->*UpdateCell_5)(0, 0, 1, 2, 3);
        // right end update
        (this->*UpdateCell_5)(horz_count - 3, horz_count - 2, horz_count - 1, horz_count - 1, horz_count - 1);
        (this->*UpdateCell_5)(horz_count - 4, horz_count - 3, horz_count - 2, horz_count - 1, horz_count - 1);
    }
    else if (wrapflag == WF_ABSORB) // mike 11-18-97
    {
        // the target value will be the source value of its inner neighbor.
        wave_target_row[1].intensity = wave_source_row[2].intensity;
        wave_target_row[0].intensity = wave_source_row[1].intensity;

        wave_target_row[horz_count - 2].intensity = wave_source_row[horz_count - 3].intensity;
        wave_target_row[horz_count - 1].intensity = wave_source_row[horz_count - 2].intensity;
    }

    else // WF_ZERO and WF_FIXED
    {
        // left end, wrap off
        wave_past_row[0].intensity = wave_target_row[0].intensity = 0.0;
        wave_target_row[0].velocity = 0.0;
        wave_past_row[1].intensity = wave_target_row[1].intensity = 0.0;
        wave_target_row[1].velocity = 0.0;
        // right end, wrap off
        wave_past_row[horz_count - 1].intensity = wave_target_row[horz_count - 1].intensity = 0.0;
        wave_target_row[horz_count - 1].velocity = 0.0;
        wave_past_row[horz_count - 2].intensity = wave_target_row[horz_count - 2].intensity = 0.0;
        wave_target_row[horz_count - 2].velocity = 0.0;
        // Pass RR same as R: zero.
    }
    WaveUpdateStep(hdc);
}

void CA::WaveUpdateStep(HDC hdc)
{
    int i;

    if (generatorflag)
        generator_ptr->Step();
    generatorlist.Step();
    /* We let the generator, if it's on, write to the wave target row.  This
    way you see the generator value, and this value is used as source
    after the rows are swapped just below.*/

    /* Now chunk all intensities and velocities
        and write all the rows to the bitmap */
#define SKIP_ROWS
#ifdef SKIP_ROWS
    if (!((showmode == EVEN_SHOW && row_number & 1) || (showmode == ODD_SHOW && !(row_number & 1))))
    // Only change the colorindex_target_row every on the even time steps
    // if you have clicked EVEN show mode
#endif

        for (i = 0; i < horz_count; i++)
        {
            /* chunking introduces coarseness to the grain.  Big chunk means
            use a lower numerical accuracy for the states.  Big chunk means fewer
            effective states for intensity and velocity, means will act more like
            a discrete CA*/
            if (_chunk.Val() > MIN_POS_CHUNK)
            {
                wave_target_row[i].intensity = _chunk.Val() * ((long) (wave_target_row[i].intensity / _chunk.Val()));
                wave_target_row[i].velocity = _chunk.Val() * ((long) (wave_target_row[i].velocity / _chunk.Val()));
            }

            /* map the (intensity to max_intensity) range to (0 to 256) color range
            and store the result in colorindex_target_row */
            // Note that if, say, A ranges between -maxA and maxA,
            //  then (maxA + A) / 2A ranges between 0 and 1.
            // 2017 I worked on this code and the similar code in WaveUpdateStep2D, which is used by most  User rules.
            if (!(showvelocity))
            {
                colorindex_target_row[i] =
                    (unsigned short) (((MAX_COLOR - 1) * (wave_target_row[i].intensity + _max_intensity.Val())) /
                        (2.0 * _max_intensity.Val()));
            }
            else
                colorindex_target_row[i] =
                    (unsigned short) (((MAX_COLOR - 1) *
                                          (AMPLIFY_VEL_COLOR * wave_target_row[i].velocity + _max_velocity.Val())) /
                        (2.0 * _max_velocity.Val()));
            // 2017 When I choose View | Show Velocity, the velocity is so close to 0 that I can't see much
            // Possibly I should multiply the velocity on the right by AMPLIFY_VEL_COLOR from ca.hpp, which might be,
            // say, 80.0

            POSITIVECLAMP(colorindex_target_row[i], (unsigned short) (MAX_COLOR - 1));

            if (entropyflag)
            {
                cellcount++;
                freqlookup[(unsigned char) (colorindex_target_row[i])]++;
            }
        }
    switch (showmode)
    {
    case BOTH_SHOW:
        break;
    case ODD_SHOW:
        for (i = 0; i < horz_count - 1; i += 2)
            colorindex_target_row[i] = colorindex_target_row[i + 1];
        if (horz_count & 1) // last index is even
            colorindex_target_row[horz_count - 1] = colorindex_target_row[horz_count - 2];
        break;
    case EVEN_SHOW:
        for (i = 1; i < horz_count; i += 2)
            colorindex_target_row[i] = colorindex_target_row[i - 1];
        break;
    }

    if (++wavesourceindex >= 3)
        wavesourceindex = 0;
    if (++wavetargetindex >= 3)
        wavetargetindex = 0;
    if (++wavepastindex >= 3)
        wavepastindex = 0;
    wave_source_row = waverowbuffer[wavesourceindex];
    wave_target_row = waverowbuffer[wavetargetindex];
    wave_past_row = waverowbuffer[wavepastindex];

    // Now step the time.
    time += _dt.Val();
    if (time > TIMEWRAP)
        time -= TIMEWRAP;
    for (i = 0; i < horz_count; i++)
        COLORREF_target_row[i] = colortable[colorindex_target_row[i]];

    Show(hdc);
}

// HEAT RULES=================== ----------

void CA::HeatInt1(int l, int c, int r)
{
    /* In the past, I've believed the heat equation says uxx=0, but it's
    more accurate to say it says ut = uxx.
        First, crude, idea to try and make uxx = 0 .  With one neighbor, this
    means (-2C + L + R) = 0, or C = (L+R)/2. This gives gross instability.
    C = L+C+R / 3 works better.
        If we bring in the ut=uxx formulation, we have something like
    C += (dt/dx*dx)(L -2C + R) as a first attempt.  This makes terrible
    results, see Masatake Mori, THE FINITE ELMENT METHOD AND ITS APPLICATIONS,
    PP.105, 106 for explanation. (If all are zero and C is plus, then C
    jumps to negative!)
        We can get a different formulation, writing lam for (dt/dx*dx).
    (nC - C)/dt = (nL -2 nC +nR)/dxdx
    nC = C + lam(nL - 2nC + nR)
    (1 + 2*lam)nC = lam nL + C + lam nR
    replace nL by L and nR by R (!!!!) and get
    nC = (lam L + C + lam R)/(1 + 2*lam).
    This is a nice formula because it incorporates dt and dx, and it is like
    a weighted averaging scheme.
        In use, it is more stable if you pick a large _dx, the idea
    being that lam, A.K.A. acceleration_parameter, shouldn't be very large.
        To drive the heating, I add dt+heat_inc to u each step as well.
        Rather than clamping, we wrap.
    */

    const capow::Heat1DResult<Real> result = capow::ComputeHeat1D<Real>(wave_source_row[l].intensity,
        wave_source_row[c].intensity, wave_source_row[r].intensity, _dt_over_dx_2, _heat_inc.Val(),
        _max_intensity.Val(), _max_velocity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity; // Calculate just for graphing.
}

void CA::HeatInt2(int ll, int l, int c, int r, int rr)
{
/*  We just use a crude average.
    For the "correct" way we would use the five neighbor second difference,
uxx =   (-LL + 16 L -30 C + 16 R - RR)/(12 dx*dx)
    We can get a different formulation, writing lam for (dt/12*dx*dx).
(nC - C)/dt = (-nLL + 16 nL -30 nC + 16 nR - nRR)/12*dxdx
nC = C + lam(-nLL + 16 nL -30 nC + 16 nR - nRR)
(1 + 30*lam)nC = C + lam (-nLL + 16nL  +  16nR - nRR )
replace nL by L and nR by R  nLL by LL and nRR by RR(!!!!) and get
nC = (C + lam(-LL + 16 L + 16 R - RR))/(1 + 30*lam).
    But this isn't stable.
*/
#define SIMPLE_HEAT2
#ifdef SIMPLE_HEAT2
    const capow::Heat1DResult<Real> result = capow::ComputeHeat1D5<Real>(wave_source_row[ll].intensity,
        wave_source_row[l].intensity, wave_source_row[c].intensity, wave_source_row[r].intensity,
        wave_source_row[rr].intensity, _heat_inc.Val(), _max_intensity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity; // Calculate just for graphing.

    // Assume heat_inc is positive so only wrap at top.  You
    // may possibly get an intensity < -max_intensity, but this
    // will be clamped back to -max_intensity in WRAP in WaveUpdate.
#else  // not SIMPLE_HEAT2
    wave_target_row[c].intensity = (wave_source_row[c].intensity +
                                       acceleration_multiplier *
                                           (-wave_source_row[ll].intensity + 16.0 * wave_source_row[l].intensity +
                                               16.0 * wave_source_row[r].intensity - wave_source_row[rr].intensity)) /
            (1 + 30.0 * acceleration_multiplier) +
        dt * heat_inc;
    wave_target_row[c].velocity =
        (wave_target_row[c].intensity - wave_source_row[c].intensity) / _dt.Val(); // Calculate just for graphing.
    CLAMP(wave_target_row[c].velocity, -_max_velocity.Val(), _max_velocity.Val());
    WRAP((wave_target_row[c].intensity), -_max_intensity.Val(), _max_intensity.Val());
#endif // SIMPLE_HEAT2
}

//--------Correct Wave Equation

void CA::AltWaveVelInt1(int l, int c, int r)
{
    /* This has the canonical for uNew = (2*u - uPast) + 2*Wave*(uNabeAvg - u),
    with Wave = _wavespeed_2_times_dt_2_over_dx_2.  2*Wave has to be below 2 for
    stability, so this rule is stable as long as dt < dx.
    */
    const capow::Wave1DResult<Real> result = capow::ComputeWave1D<Real>(wave_source_row[l].intensity,
        wave_source_row[c].intensity, wave_source_row[r].intensity, wave_past_row[c].intensity,
        _wavespeed_2_times_dt_2_over_dx_2, _max_intensity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity; // Calculate just for graphing.
}

void CA::WaveVelInt2(int ll, int l, int c, int r, int rr)
{
    /*  Here we are going to do the same thing as WaveVelInt1, except
    we look at five neighbors.  The second differences formula for
    uxx = (-LL + 16 L -30 C + 16 R - RR)/(12 dx*dx).
  */
    const capow::Wave1DResult<Real> result =
        capow::ComputeWave1D5<Real>(wave_source_row[ll].intensity, wave_source_row[l].intensity,
            wave_source_row[c].intensity, wave_source_row[r].intensity, wave_source_row[rr].intensity,
            wave_source_row[c].velocity, _dt_over_12_times_dx_2, _max_intensity.Val(), _max_velocity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity; // Calculate just for graphing.
}

//============= Oscillator Rules====================================

#pragma argsused // Because we don't actually use l,c, or r
void CA::Oscillator(int l, int c, int r)
{
    const auto driverValue = _driver_multiplier.Val() * cos(_phase + frequency_factor * time);
    const auto result = capow::ComputeOscillator1D(wave_source_row[c].intensity, wave_source_row[c].velocity,
        _dt_over_mass, _friction_multiplier.Val(), _spring_multiplier.Val(), driverValue, _max_intensity.Val(),
        _max_velocity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity;
}

#pragma argsused // Because we don't actually use l,c, or r
void CA::DiverseOscillator(int l, int c, int r)
{
    const auto driverValue = _driver_multiplier.Val() * cos(_phase + frequency_factor * time);
    const auto result = capow::ComputeDiverseOscillator1D(wave_source_row[c].intensity, wave_source_row[c].velocity,
        _dt_over_mass, _friction_multiplier.Val(), _spring_multiplier.Val(), driverValue,
        wave_target_row[c].friction_tweak, wave_target_row[c].spring_tweak, wave_target_row[c].mass_tweak,
        _max_intensity.Val(), _max_velocity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity;
}

void CA::AltOscillatorWave(int l, int c, int r)
{
    /* This comes out of the Wave Equation for newu in terms of u.
    For if we say V = (U - pastU)/dt, and
    newV = V + dt wavespeed^2 * (L-2U+R)/dx^2 + dt/mass (osc_accel), and
    newU = U _ dt*newV, then it all comes out.  If you leave out
    osc_accel, this is equivalent to
    newU - U + dt*( (U-PastU)/dt + dt wavespeed^2 * (L-2U+R)/dx^2),
    which in turn becomes the Wave Equation schema.*/

    const auto driverValue = _driver_multiplier.Val() * cos(_phase + frequency_factor * time);
    const auto result = capow::ComputeOscillatorWave1D(wave_source_row[l].intensity, wave_source_row[c].intensity,
        wave_source_row[r].intensity, wave_source_row[c].velocity, _dt_over_mass, _friction_multiplier.Val(),
        _spring_multiplier.Val(), driverValue, _dt_over_dx_2, _max_intensity.Val(), _max_velocity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity;
}

void CA::AltDiverseOscillatorWave(int l, int c, int r)
{
    const auto driverValue = _driver_multiplier.Val() * cos(_phase + frequency_factor * time);
    const auto result =
        capow::ComputeDiverseOscillatorWave1D(wave_source_row[l].intensity, wave_source_row[c].intensity,
            wave_source_row[r].intensity, wave_source_row[c].velocity, _dt_over_mass, _friction_multiplier.Val(),
            _spring_multiplier.Val(), driverValue, wave_target_row[c].friction_tweak, wave_target_row[c].spring_tweak,
            wave_target_row[c].mass_tweak, _dt_over_dx_2, _max_intensity.Val(), _max_velocity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity;
}

//================Nonlinear Waves==================

void CA::AltUlamWave(int l, int c, int r)
{ // Quadratic nonlinear wave
    const auto result = capow::ComputeUlamWave1D(wave_source_row[l].intensity, wave_source_row[c].intensity,
        wave_source_row[r].intensity, wave_past_row[c].intensity, _wavespeed_2_times_dt_2_over_dx_2,
        _nonlinearity1.Val(), _max_intensity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity;
}

#define nonlinearity_tweak _cell_param[0]
#define max_nonlinearity _nonlinearity2.Val() // used spring_multiplier before
#define NONLINEARITY_GROW_FACTOR 1.01
#define MIN_NONLINEARITY_TWEAK 0.001
void CA::StableUlamWave(int l, int c, int r)
{
    const auto result = capow::ComputeStableUlamWave1D(wave_source_row[l].intensity, wave_source_row[c].intensity,
        wave_source_row[r].intensity, wave_source_row[c].velocity, wave_source_row[c].nonlinearity_tweak, _dt_over_dx_2,
        _dt.Val(), max_nonlinearity, _max_intensity.Val(), _max_velocity.Val());
    wave_target_row[c].velocity = result.velocity;
    if (result.zeroNeighbors)
        // Go linear, and try and save the neighbor cells as well.
        wave_target_row[c].nonlinearity_tweak = wave_target_row[r].nonlinearity_tweak =
            wave_target_row[l].nonlinearity_tweak = 0;
    wave_target_row[c].intensity = result.nextIntensity;
    wave_source_row[c].nonlinearity_tweak = result.nextTweak;
    wave_target_row[c].nonlinearity_tweak = result.nextTweak;
}

#define OSTROV_CUBE
#ifdef OSTROV_CUBE
/* The cubic ulam is quite stable provided that you keep the
max_intensity pretty low, so that the cubes don't run away.*/
void CA::CubicUlamWave(int l, int c, int r) // CubicUlamWave(int l, int c, int r)
{
    const auto result = capow::ComputeCubicUlamWave1D(wave_source_row[l].intensity, wave_source_row[c].intensity,
        wave_source_row[r].intensity, wave_past_row[c].intensity, _wavespeed_2_times_dt_2_over_dx_2,
        _nonlinearity2.Val(), _max_intensity.Val(), _dt.Val());
    wave_target_row[c].intensity = result.nextIntensity;
    wave_target_row[c].velocity = result.velocity;
}
#else  // not OSTROV_CUBIC
void CA::CubicUlamWave(int l, int c, int r) // CubicUlamWave(int l, int c, int r)
{
    Real cldiff = wave_source_row[c].intensity - wave_source_row[l].intensity;
    Real rcdiff = wave_source_row[r].intensity - wave_source_row[c].intensity;
    wave_target_row[c].intensity = -wave_past_row[c].intensity + 2.0 * wave_source_row[c].intensity +
        _wavespeed_2_times_dt_2_over_dx_2 *
            (rcdiff - cldiff + _nonlinearity2.Val() * (rcdiff * rcdiff * rcdiff - cldiff * cldiff * cldiff));
    CLAMP(wave_target_row[c].intensity, -_max_intensity.Val(), _max_intensity.Val());
    wave_target_row[c].velocity = (wave_target_row[c].intensity - wave_source_row[c].intensity) / _dt.Val();
}
#endif // OSTROV_CUBIC
//===========================2D Wave stuff
void CA::Wave2D(int c, int e, int n, int w, int s)
{
    /* This is the form uNew = (2*u - uPast) + 2*Wave*(uNabeAvg - u).  Here
        Wave = 0.5 * _wavespeed_2_times_dt_2_over_dx_2.  2*Wave has to be below 1
        for stability, so this rule is stable as long as dt < sqrt(2.0) * dx. */
    /* I do FOUR_SUM/4.0 - C; if I use the more logical FOUR_SUM - 4.0*C, then
    I need an extra 1/4.0 here for stability.  So it's easer to put it inside. */
    const capow::Wave2DResult<Real> result =
        capow::ComputeWave2D<Real>(wave_source_plane[c].intensity, wave_source_plane[e].intensity,
            wave_source_plane[n].intensity, wave_source_plane[w].intensity, wave_source_plane[s].intensity,
            wave_past_plane[c].intensity, _wavespeed_2_times_dt_2_over_dx_2, _max_intensity.Val(), _dt.Val());
    wave_target_plane[c].intensity = result.nextIntensity;
    // 2017 extra line to save velocity in variable[1]
    wave_target_plane[c].variable[1] = result.velocity; // Calculate velocity and put in variable[1]
}

void CA::Heat2D(int c, int e, int n, int w, int s)
{
    const capow::Heat2DResult<Real> result = capow::ComputeHeat2D<Real>(wave_source_plane[c].intensity,
        wave_source_plane[e].intensity, wave_source_plane[n].intensity, wave_source_plane[w].intensity,
        wave_source_plane[s].intensity, _heat_inc.Val(), _max_intensity.Val(), _dt.Val());
    wave_target_plane[c].intensity = result.nextIntensity;
    // 2017 extra line to save velocity in variable[1]
    wave_target_plane[c].variable[1] = result.velocity; // Calculate velocity and put in variable[1]
}

void CA::WaveUpdateStep2D(HDC hdc) // You don't need the hdc argument!
{
    if (generatorflag)
        generator_ptr->Step(); // write to target_plane
    generatorlist.Step();

    capow::RenderWavePlaneToImageBuffer(&wavePlaneImage, wave_target_plane, horz_count_2D, vert_count_2D, CX_2D,
        colortable, _max_intensity.Val(), showvelocity != 0);

    RotateWavePlanes2D();
}

void CA::RotateWavePlanes2D()
{
    if (++wavesourceindex >= 3)
        wavesourceindex = 0;
    if (++wavetargetindex >= 3)
        wavetargetindex = 0;
    if (++wavepastindex >= 3)
        wavepastindex = 0;
    wave_source_plane = waveplanebuffer[wavesourceindex];
    wave_target_plane = waveplanebuffer[wavetargetindex];
    wave_past_plane = waveplanebuffer[wavepastindex];
}

#if defined(CAPOW_ENABLE_ALPAKA)
void CA::CopyAlpakaWave1DToCpu()
{
    if (!alpakaWave1DLive || !alpakaWave1DLive->IsActive())
        return;

    std::vector<capow::AlpakaPlaneValue> intensityValues(static_cast<std::size_t>(horz_count));
    std::vector<capow::AlpakaPlaneValue> velocityValues(static_cast<std::size_t>(horz_count));
    std::vector<capow::AlpakaPlaneValue> pastValues(static_cast<std::size_t>(horz_count));
    std::vector<capow::AlpakaPlaneValue> nonlinearityValues(static_cast<std::size_t>(horz_count));
    std::string errorText;
    const bool ok = alpakaWave1DLive->DownloadCurrentAndPast(intensityValues.data(), 1, velocityValues.data(), 1,
        pastValues.data(), 1, nonlinearityValues.data(), 1, &errorText);
    if (!ok)
    {
        OutputDebugStringA("1D GPU download failed: ");
        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
        return;
    }

    for (int x = 0; x < horz_count; ++x)
    {
        const std::size_t indexValue = static_cast<std::size_t>(x);
        wave_source_row[x].intensity = intensityValues[indexValue];
        wave_source_row[x].velocity = velocityValues[indexValue];
        wave_target_row[x] = wave_source_row[x];
        wave_past_row[x] = wave_source_row[x];
        wave_past_row[x].intensity = pastValues[indexValue];
        wave_source_row[x]._cell_param[0] = nonlinearityValues[indexValue];
        wave_target_row[x]._cell_param[0] = nonlinearityValues[indexValue];
        wave_past_row[x]._cell_param[0] = nonlinearityValues[indexValue];
    }
}

void CA::CopyAlpakaHeat2DToCpu()
{
    if (!alpakaHeat2DLive || !alpakaHeat2DLive->IsActive())
        return;

    const std::size_t packedCount = static_cast<std::size_t>(horz_count_2D) * static_cast<std::size_t>(vert_count_2D);
    std::vector<capow::AlpakaPlaneValue> intensityValues(packedCount);
    std::vector<capow::AlpakaPlaneValue> velocityValues(packedCount);
    std::vector<capow::AlpakaPlaneValue> pastValues;
    const bool isWave2D = type_ca == CA_WAVE_2D;
    if (isWave2D)
        pastValues.resize(packedCount);
    std::string errorText;
    const bool ok = isWave2D
        ? alpakaHeat2DLive->DownloadCurrentAndPast(
              intensityValues.data(), 1, velocityValues.data(), 1, pastValues.data(), 1, &errorText)
        : alpakaHeat2DLive->DownloadCurrent(intensityValues.data(), 1, velocityValues.data(), 1, &errorText);
    if (!ok)
    {
        OutputDebugStringA("2D GPU download failed: ");
        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
        return;
    }

    for (int y = 0; y < vert_count_2D; ++y)
    {
        for (int x = 0; x < horz_count_2D; ++x)
        {
            const std::size_t packedIndex =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(horz_count_2D) + static_cast<std::size_t>(x);
            const int c = index(x, y);
            wave_source_plane[c].intensity = intensityValues[packedIndex];
            wave_source_plane[c].variable[1] = velocityValues[packedIndex];
            wave_target_plane[c] = wave_source_plane[c];
            wave_past_plane[c] = wave_source_plane[c];
            if (isWave2D)
                wave_past_plane[c].intensity = pastValues[packedIndex];
        }
    }

    PaintHeat2DPlaneToBitmap(wave_source_plane);
}

void CA::CopyAlpakaDigital1DToCpu()
{
    if (!alpakaDigital1DLive || !alpakaDigital1DLive->IsActive())
        return;

    std::vector<capow::AlpakaDigitalValue> stateValues(static_cast<std::size_t>(horz_count));
    std::string errorText;
    const bool ok = alpakaDigital1DLive->DownloadCurrent(stateValues.data(), &errorText);
    if (!ok)
    {
        OutputDebugStringA("digital GPU download failed: ");
        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
        return;
    }

    for (int x = 0; x < horz_count; ++x)
    {
        const capow::AlpakaDigitalValue value = stateValues[static_cast<std::size_t>(x)];
        source_row[x] = value;
        target_row[x] = value;
        colorindex_target_row[x] = value;
        COLORREF_target_row[x] = colortable[value];
    }

    if (type_ca == CA_REVERSIBLE)
    {
        errorText.clear();
        const bool pastOk = alpakaDigital1DLive->DownloadPast(past_row, &errorText);
        if (!pastOk)
        {
            OutputDebugStringA("digital GPU past download failed: ");
            OutputDebugStringA(errorText.c_str());
            OutputDebugStringA("\n");
        }
    }
}

bool CA::CopyAlpakaDigital1DToTargetRow()
{
    if (!alpakaDigital1DLive || !alpakaDigital1DLive->IsActive())
        return true;

    std::vector<capow::AlpakaDigitalValue> stateValues(static_cast<std::size_t>(horz_count));
    std::string errorText;
    const bool ok = alpakaDigital1DLive->DownloadCurrent(stateValues.data(), &errorText);
    if (!ok)
    {
        OutputDebugStringA("digital GPU download failed: ");
        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
        return false;
    }

    for (int x = 0; x < horz_count; ++x)
    {
        const capow::AlpakaDigitalValue value = stateValues[static_cast<std::size_t>(x)];
        target_row[x] = value;
        colorindex_target_row[x] = value;
        COLORREF_target_row[x] = colortable[value];
    }
    return true;
}

void CA::AccumulateAlpakaStandardEntropy()
{
    if (!entropyflag)
        return;

    unsigned short nabe = 0;
    for (int i = horz_count - radius; i < horz_count; i++)
    {
        nabe |= source_row[i];
        nabe <<= statebits;
    }
    for (int i = 0; i < radius; i++)
    {
        nabe |= source_row[i];
        nabe <<= statebits;
    }
    nabe |= source_row[radius];
    cellcount++;
    freqlookup[nabe]++;

    for (int i = 1; i < horz_count - radius; i++)
    {
        nabe <<= statebits;
        nabe &= mask;
        nabe |= source_row[i + radius];
        cellcount++;
        freqlookup[nabe]++;
    }

    int leftindex = 0;
    for (int i = horz_count - radius; i < horz_count; ++i)
    {
        nabe <<= statebits;
        nabe &= mask;
        nabe |= source_row[leftindex];
        cellcount++;
        freqlookup[nabe]++;
        leftindex++;
    }
}

void CA::MarkAlpakaHeat2DDirty()
{
    CopyAlpakaWave1DToCpu();
    CopyAlpakaHeat2DToCpu();
    CopyAlpakaDigital1DToCpu();
    if (alpakaWave1DLive)
        alpakaWave1DLive->Deactivate();
    if (alpakaHeat2DLive)
        alpakaHeat2DLive->Deactivate();
    if (alpakaDigital1DLive)
        alpakaDigital1DLive->Deactivate();
    alpakaWave1DTextureReady = false;
    alpakaHeat2DTextureReady = false;
    alpakaDigital1DTextureReady = false;
}

bool CA::CanUseAlpakaLiveGpu(void)
{
    if (type_ca == CA_USER)
        return false;

    capow::AlpakaManager &backendManager = capow::GetAlpakaManager();
    if (type_ca == CA_STANDARD || type_ca == CA_REVERSIBLE)
    {
        const capow::AlpakaRule rule =
            type_ca == CA_REVERSIBLE ? capow::ALPAKA_RULE_CA_REVERSIBLE : capow::ALPAKA_RULE_CA_STANDARD;
        const bool supportedView = viewmode == IDC_DOWN_VIEW || viewmode == IDC_SCROLL_VIEW;
        return backendManager.CanRunGpu(rule) && supportedView && wrapflag == WF_WRAP && generatorlist.Count() == 0;
    }

    if (IsAlpakaWave1DType(type_ca))
    {
        const capow::AlpakaRule rule = AlpakaWave1DAlpakaRuleForType(type_ca);
        const bool supportedView = viewmode == IDC_DOWN_VIEW || viewmode == IDC_SCROLL_VIEW;
        return backendManager.CanRunGpu(rule) && supportedView && wrapflag == WF_WRAP && showmode == BOTH_SHOW &&
            _chunk.Val() <= MIN_POS_CHUNK && _smoothsteps == 0 && !generatorflag && generatorlist.Count() == 0;
    }

    const bool isHeat2D = type_ca == CA_HEAT_2D;
    const bool isWave2D = type_ca == CA_WAVE_2D;
    if (!isHeat2D && !isWave2D)
        return false;

    const capow::AlpakaRule rule = isWave2D ? capow::ALPAKA_RULE_CA_WAVE_2D : capow::ALPAKA_RULE_CA_HEAT_2D;
    const bool supportedWrap = isHeat2D ? IsAlpakaHeat2DWrapFlagSupported(wrapflag) : wrapflag == WF_WRAP;
    return backendManager.CanRunGpu(rule) && viewmode == IDC_2D_VIEW && supportedWrap && _smoothsteps == 0 &&
        !generatorflag && generatorlist.Count() == 0;
}

bool CA::TryAlpakaDigitalUpdate(HDC hdc)
{
    const bool reversible = type_ca == CA_REVERSIBLE;
    const bool standard = type_ca == CA_STANDARD;
    if (!standard && !reversible)
    {
        if (alpakaDigital1DLive && alpakaDigital1DLive->IsActive())
            MarkAlpakaHeat2DDirty();
        return false;
    }

    capow::AlpakaManager &backendManager = capow::GetAlpakaManager();
    const capow::AlpakaRule rule = reversible ? capow::ALPAKA_RULE_CA_REVERSIBLE : capow::ALPAKA_RULE_CA_STANDARD;
    const bool liveGpuType = capowgl != nullptr && capowgl->Type() == LIVE_GPU;
    const bool supportedView = viewmode == IDC_DOWN_VIEW || viewmode == IDC_SCROLL_VIEW;
    const bool liveGpuCandidate = backendManager.GetBackend() == capow::ALPAKA_BACKEND_GPU &&
        backendManager.CanRunGpu(rule) && liveGpuType && supportedView && wrapflag == WF_WRAP &&
        generatorlist.Count() == 0;
    if (!liveGpuCandidate)
    {
        if (alpakaDigital1DLive && alpakaDigital1DLive->IsActive())
            MarkAlpakaHeat2DDirty();
        if (liveGpuType)
            ForceAlpakaCpuBackend();
        return false;
    }

    if (!alpakaDigital1DLive)
        alpakaDigital1DLive = std::make_unique<capow::Digital1DLiveState>();

    capow::Digital1DLiveOptions options;
    options.width = horz_count;
    options.historyWidth = maxx - minx + 1;
    options.historyHeight = maxy - miny + 1;
    options.row = row_number - miny;
    options.bltLines = calist_ptr->_blt_lines;
    options.view = viewmode == IDC_DOWN_VIEW ? capow::DIGITAL_1D_LIVE_VIEW_DOWN : capow::DIGITAL_1D_LIVE_VIEW_SCROLL;
    options.rule = reversible ? capow::DIGITAL_1D_LIVE_RULE_REVERSIBLE : capow::DIGITAL_1D_LIVE_RULE_STANDARD;
    options.radius = radius;
    options.stateBits = statebits;
    options.stateCount = states;
    options.lookupCount = nabeoptions;
    options.colorCount = MAX_COLOR;

    const capow::AlpakaDigitalValue *sourceData = nullptr;
    const capow::AlpakaDigitalValue *pastData = nullptr;
    const capow::AlpakaDigitalValue *lookupData = nullptr;
    if (alpakaDigital1DLive->NeedsSource(options))
    {
        sourceData = source_row;
        pastData = reversible ? past_row : nullptr;
        lookupData = lookup;
    }

    if (!capowgl->MakeCurrent(hdc))
    {
        if (alpakaDigital1DLive && alpakaDigital1DLive->IsActive())
            MarkAlpakaHeat2DDirty();
        if (liveGpuType)
            ForceAlpakaCpuBackend();
        return false;
    }

    std::string errorText;
    const bool ok = alpakaDigital1DLive->RunFrame(
        options, sourceData, pastData, lookupData, reinterpret_cast<const std::uint32_t *>(colortable), &errorText);
    capowgl->ReleaseCurrent();
    if (!ok)
    {
        OutputDebugStringA(reversible ? "CA_REVERSIBLE GPU update failed: " : "CA_STANDARD GPU update failed: ");
        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
        MarkAlpakaHeat2DDirty();
        ForceAlpakaCpuBackend();
        return false;
    }

    const bool standardStripeCheck = standard &&
        (calist_ptr->stripeseedflag || calist_ptr->stripekillflag || (calist_ptr->breedflag && fail_stripe));
    const bool needsCpuRowState = entropyflag || standardStripeCheck;
    if (needsCpuRowState)
    {
        AccumulateAlpakaStandardEntropy();
        if (!CopyAlpakaDigital1DToTargetRow())
        {
            MarkAlpakaHeat2DDirty();
            ForceAlpakaCpuBackend();
            return true;
        }
    }

    alpakaDigital1DTextureReady = true;
    if (reversible)
    {
        if (++pastrowindex >= MEMORY)
            pastrowindex = 0;
        past_row = rowbuffer[pastrowindex];
    }
    if (viewmode == IDC_DOWN_VIEW)
    {
        row_number++;
        if (row_number > maxy)
            row_number = miny;
    }
    else
    {
        row_number++;
        if (row_number > maxy)
            row_number = maxy - (calist_ptr->_blt_lines) + 1;
    }
    if (++sourcerowindex >= MEMORY)
        sourcerowindex = 0;
    if (++targetrowindex >= MEMORY)
        targetrowindex = 0;
    source_row = rowbuffer[sourcerowindex];
    target_row = rowbuffer[targetrowindex];
    if (cellcount > (calist_ptr->breedcycle * horz_count))
        Entropy();
    if (standard && sourcerowindex == MEMORY - 1)
    {
        if (alpakaDigital1DLive)
            alpakaDigital1DLive->Deactivate();
        Avoidstripes();
    }
    return true;
}

bool CA::TryAlpakaWave1DUpdate(HDC hdc)
{
    if (!IsAlpakaWave1DType(type_ca))
    {
        if (alpakaWave1DLive && alpakaWave1DLive->IsActive())
            MarkAlpakaHeat2DDirty();
        return false;
    }

    capow::AlpakaManager &backendManager = capow::GetAlpakaManager();
    const capow::AlpakaRule rule = AlpakaWave1DAlpakaRuleForType(type_ca);
    const bool liveGpuType = capowgl != nullptr && capowgl->Type() == LIVE_GPU;
    const bool supportedView = viewmode == IDC_DOWN_VIEW || viewmode == IDC_SCROLL_VIEW;
    const bool liveGpuCandidate = backendManager.GetBackend() == capow::ALPAKA_BACKEND_GPU &&
        backendManager.CanRunGpu(rule) && liveGpuType && supportedView && wrapflag == WF_WRAP &&
        showmode == BOTH_SHOW && _chunk.Val() <= MIN_POS_CHUNK && !generatorflag && generatorlist.Count() == 0;
    const bool canUseGpu = liveGpuCandidate && _smoothsteps == 0;
    if (!canUseGpu)
    {
        if (alpakaWave1DLive && alpakaWave1DLive->IsActive())
            MarkAlpakaHeat2DDirty();
        if (liveGpuType && !liveGpuCandidate)
            ForceAlpakaCpuBackend();
        return false;
    }

    if (!alpakaWave1DLive)
        alpakaWave1DLive = std::make_unique<capow::Wave1DLiveState>();

    capow::Wave1DLiveOptions options;
    options.width = horz_count;
    options.historyWidth = maxx - minx + 1;
    options.historyHeight = maxy - miny + 1;
    options.row = row_number - miny;
    options.bltLines = calist_ptr->_blt_lines;
    options.view = viewmode == IDC_DOWN_VIEW ? capow::WAVE_1D_LIVE_VIEW_DOWN : capow::WAVE_1D_LIVE_VIEW_SCROLL;
    options.rule = AlpakaWave1DRuleForType(type_ca);
    options.waveSpeed2TimeStep2OverDx2 = _wavespeed_2_times_dt_2_over_dx_2;
    options.dtOverDx2 = _dt_over_dx_2;
    options.maxIntensity = _max_intensity.Val();
    options.maxVelocity = _max_velocity.Val();
    options.timeStep = _dt.Val();
    options.dtOverMass = _dt_over_mass;
    options.frictionMultiplier = _friction_multiplier.Val();
    options.springMultiplier = _spring_multiplier.Val();
    options.driverValue = _driver_multiplier.Val() * cos(_phase + frequency_factor * time);
    options.nonlinearity1 = _nonlinearity1.Val();
    options.nonlinearity2 = _nonlinearity2.Val();
    options.velocityColorScale = AMPLIFY_VEL_COLOR;
    options.colorCount = MAX_COLOR;
    options.showVelocity = showvelocity != 0;

    std::vector<capow::AlpakaPlaneValue> sourceIntensity;
    std::vector<capow::AlpakaPlaneValue> pastIntensity;
    std::vector<capow::AlpakaPlaneValue> sourceVelocity;
    std::vector<capow::AlpakaPlaneValue> frictionTweaks;
    std::vector<capow::AlpakaPlaneValue> springTweaks;
    std::vector<capow::AlpakaPlaneValue> massTweaks;
    std::vector<capow::AlpakaPlaneValue> nonlinearityTweaks;
    const capow::AlpakaPlaneValue *sourceData = nullptr;
    const capow::AlpakaPlaneValue *pastData = nullptr;
    const capow::AlpakaPlaneValue *velocityData = nullptr;
    const capow::AlpakaPlaneValue *frictionData = nullptr;
    const capow::AlpakaPlaneValue *springData = nullptr;
    const capow::AlpakaPlaneValue *massData = nullptr;
    const capow::AlpakaPlaneValue *nonlinearityData = nullptr;
    if (alpakaWave1DLive->NeedsSource(options))
    {
        sourceIntensity.resize(static_cast<std::size_t>(horz_count));
        pastIntensity.resize(static_cast<std::size_t>(horz_count));
        sourceVelocity.resize(static_cast<std::size_t>(horz_count));
        frictionTweaks.resize(static_cast<std::size_t>(horz_count));
        springTweaks.resize(static_cast<std::size_t>(horz_count));
        massTweaks.resize(static_cast<std::size_t>(horz_count));
        nonlinearityTweaks.resize(static_cast<std::size_t>(horz_count));
        for (int x = 0; x < horz_count; ++x)
        {
            const std::size_t indexValue = static_cast<std::size_t>(x);
            sourceIntensity[indexValue] = wave_source_row[x].intensity;
            pastIntensity[indexValue] = wave_past_row[x].intensity;
            sourceVelocity[indexValue] = wave_source_row[x].velocity;
            frictionTweaks[indexValue] = wave_target_row[x].friction_tweak;
            springTweaks[indexValue] = wave_target_row[x].spring_tweak;
            massTweaks[indexValue] = wave_target_row[x].mass_tweak;
            nonlinearityTweaks[indexValue] = wave_source_row[x]._cell_param[0];
        }
        sourceData = sourceIntensity.data();
        pastData = pastIntensity.data();
        velocityData = sourceVelocity.data();
        frictionData = frictionTweaks.data();
        springData = springTweaks.data();
        massData = massTweaks.data();
        nonlinearityData = nonlinearityTweaks.data();
    }

    if (!capowgl->MakeCurrent(hdc))
    {
        if (alpakaWave1DLive && alpakaWave1DLive->IsActive())
            MarkAlpakaHeat2DDirty();
        if (liveGpuType)
            ForceAlpakaCpuBackend();
        return false;
    }

    std::string errorText;
    const bool ok = alpakaWave1DLive->RunFrame(options, sourceData, 1, pastData, 1, velocityData, 1, frictionData,
        springData, massData, nonlinearityData, reinterpret_cast<const std::uint32_t *>(colortable), &errorText);
    capowgl->ReleaseCurrent();
    if (!ok)
    {
        OutputDebugStringA("CA_WAVE_1D GPU update failed: ");
        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
        MarkAlpakaHeat2DDirty();
        ForceAlpakaCpuBackend();
        return false;
    }

    alpakaWave1DTextureReady = true;
    if (viewmode == IDC_DOWN_VIEW)
    {
        row_number++;
        if (row_number > maxy)
            row_number = miny;
    }
    else
    {
        row_number++;
        if (row_number > maxy)
            row_number = maxy - (calist_ptr->_blt_lines) + 1;
    }
    time += _dt.Val();
    if (time > TIMEWRAP)
        time -= TIMEWRAP;
    return true;
}

void CA::PaintHeat2DPlaneToBitmap(const Wavecell2 *plane)
{
    if (plane == nullptr)
        return;

    capow::RenderWavePlaneToImageBuffer(&wavePlaneImage, plane, horz_count_2D, vert_count_2D, CX_2D, colortable,
        _max_intensity.Val(), showvelocity != 0);
}

bool CA::TryAlpakaHeat2DUpdate(HDC hdc)
{
    const bool isHeat2D = type_ca == CA_HEAT_2D;
    const bool isWave2D = type_ca == CA_WAVE_2D;
    if (!isHeat2D && !isWave2D)
    {
        MarkAlpakaHeat2DDirty();
        return false;
    }

    capow::AlpakaManager &backendManager = capow::GetAlpakaManager();
    const capow::AlpakaRule rule = isWave2D ? capow::ALPAKA_RULE_CA_WAVE_2D : capow::ALPAKA_RULE_CA_HEAT_2D;
    const bool liveGpuType = capowgl != nullptr && capowgl->Type() == LIVE_GPU;
    const bool supportedWrap = isHeat2D ? IsAlpakaHeat2DWrapFlagSupported(wrapflag) : wrapflag == WF_WRAP;
    const bool canUseGpu = backendManager.GetBackend() == capow::ALPAKA_BACKEND_GPU && backendManager.CanRunGpu(rule) &&
        viewmode == IDC_2D_VIEW && supportedWrap && _smoothsteps == 0 && !generatorflag && generatorlist.Count() == 0 &&
        liveGpuType;
    if (!canUseGpu)
    {
        MarkAlpakaHeat2DDirty();
        if (liveGpuType)
            ForceAlpakaCpuBackend();
        return false;
    }

    if (!alpakaHeat2DLive)
        alpakaHeat2DLive = std::make_unique<capow::Heat2DLiveState>();

    capow::Heat2DLiveOptions options;
    options.width = horz_count_2D;
    options.height = vert_count_2D;
    options.rule = isWave2D ? capow::LIVE_2D_RULE_WAVE : capow::LIVE_2D_RULE_HEAT;
    options.heatIncrement = _heat_inc.Val();
    options.waveSpeed2TimeStep2OverDx2 = _wavespeed_2_times_dt_2_over_dx_2;
    options.maxIntensity = _max_intensity.Val();
    options.timeStep = _dt.Val();
    options.velocityColorScale = AMPLIFY_VEL_COLOR_2D;
    options.colorCount = MAX_COLOR;
    options.showVelocity = showvelocity != 0;
    options.boundaryMode = Heat2DBoundaryModeForWrapFlag(wrapflag);

    const capow::AlpakaPlaneValue *sourceData = nullptr;
    const capow::AlpakaPlaneValue *pastData = nullptr;
    std::vector<capow::AlpakaPlaneValue> sourcePlane;
    std::vector<capow::AlpakaPlaneValue> pastPlane;
    if (alpakaHeat2DLive->NeedsSource(options))
    {
        sourcePlane.resize(static_cast<std::size_t>(horz_count_2D) * static_cast<std::size_t>(vert_count_2D));
        if (isWave2D)
            pastPlane.resize(sourcePlane.size());
        for (int y = 0; y < vert_count_2D; ++y)
        {
            for (int x = 0; x < horz_count_2D; ++x)
            {
                const std::size_t packedIndex =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(horz_count_2D) + static_cast<std::size_t>(x);
                const int c = index(x, y);
                sourcePlane[packedIndex] = wave_source_plane[c].intensity;
                if (isWave2D)
                    pastPlane[packedIndex] = wave_past_plane[c].intensity;
            }
        }
        sourceData = sourcePlane.data();
        if (isWave2D)
            pastData = pastPlane.data();
    }

    if (!capowgl->MakeCurrent(hdc))
    {
        MarkAlpakaHeat2DDirty();
        if (liveGpuType)
            ForceAlpakaCpuBackend();
        return false;
    }

    std::string errorText;
    const bool ok = alpakaHeat2DLive->RunFrame(
        options, sourceData, 1, pastData, 1, reinterpret_cast<const std::uint32_t *>(colortable), &errorText);
    capowgl->ReleaseCurrent();
    if (!ok)
    {
        OutputDebugStringA("CA_HEAT_2D GPU update failed: ");
        OutputDebugStringA(errorText.c_str());
        OutputDebugStringA("\n");
        MarkAlpakaHeat2DDirty();
        ForceAlpakaCpuBackend();
        return false;
    }

    alpakaHeat2DTextureReady = true;
    RotateWavePlanes2D();
    return true;
}

bool CA::DrawAlpakaHeat2DTexture(HDC hdc, int left, int top, int width, int height)
{
    if (capow::GetAlpakaManager().GetBackend() != capow::ALPAKA_BACKEND_GPU)
        return false;
    if (capowgl == nullptr || capowgl->Type() != LIVE_GPU)
        return false;
    if (!alpakaHeat2DTextureReady || !alpakaHeat2DLive)
        return false;

    const unsigned int texture = alpakaHeat2DLive->GetTexture();
    if (texture == 0U)
        return false;

    return capowgl->DrawTexture2D(hdc, texture, left, top, width, height);
}

bool CA::DrawAlpakaWave1DTexture(HDC hdc, int left, int top, int width, int height)
{
    if (capow::GetAlpakaManager().GetBackend() != capow::ALPAKA_BACKEND_GPU)
        return false;
    if (capowgl == nullptr || capowgl->Type() != LIVE_GPU)
        return false;
    if (!alpakaWave1DTextureReady || !alpakaWave1DLive)
        return false;

    const unsigned int texture = alpakaWave1DLive->GetTexture();
    if (texture == 0U)
        return false;

    return capowgl->DrawTexture2D(hdc, texture, left, top, width, height);
}

bool CA::DrawAlpakaDigital1DTexture(HDC hdc, int left, int top, int width, int height)
{
    if (capow::GetAlpakaManager().GetBackend() != capow::ALPAKA_BACKEND_GPU)
        return false;
    if (capowgl == nullptr || capowgl->Type() != LIVE_GPU)
        return false;
    if (!alpakaDigital1DTextureReady || !alpakaDigital1DLive)
        return false;

    const unsigned int texture = alpakaDigital1DLive->GetTexture();
    if (texture == 0U)
        return false;

    return capowgl->DrawTexture2D(hdc, texture, left, top, width, height);
}
#endif

void CA::WaveUpdate2D(HDC hdc)
{
    int c, e, n, w, s;
    int x, y;

#if defined(CAPOW_ENABLE_ALPAKA)
    if (TryAlpakaHeat2DUpdate(hdc))
        return;
#endif

    if (!_smoothsteps)
    {
        for (y = 1; y < vert_count_2D - 1; y++)
        {
            c = index(1, y);
            e = index(2, y);
            n = index(1, y - 1);
            w = index(0, y);
            s = index(1, y + 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c++;
                e++;
                n++;
                w++;
                s++;
            }
        }
        if (wrapflag == WF_WRAP)
        {
            // upper left corner update
            (this->*UpdateCell_5)(
                index(0, 0), index(1, 0), index(0, vert_count_2D - 1), index(horz_count_2D - 1, 0), index(0, 1));
            // upper right corner update
            (this->*UpdateCell_5)(index(horz_count_2D - 1, 0), index(0, 0), index(horz_count_2D - 1, vert_count_2D - 1),
                index(horz_count_2D - 2, 0), index(horz_count_2D - 1, 1));
            // bottom left corner update
            (this->*UpdateCell_5)(index(0, vert_count_2D - 1), // c
                index(1, vert_count_2D - 1),                   // e
                index(0, vert_count_2D - 2),                   // n
                index(horz_count_2D - 1, vert_count_2D - 1),   // w
                index(0, 0));                                  // s
            // bottom right corner update
            (this->*UpdateCell_5)(index(horz_count_2D - 1, vert_count_2D - 1), index(0, vert_count_2D - 1), // e
                index(horz_count_2D - 1, vert_count_2D - 2),                                                // n
                index(horz_count_2D - 2, vert_count_2D - 1),                                                // w
                index(horz_count_2D - 1, 0));                                                               // s
            // top edge
            c = index(1, 0);
            e = index(2, 0);
            n = index(1, vert_count_2D - 1);
            w = index(0, 0);
            s = index(1, 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c++;
                e++;
                n++;
                w++;
                s++;
            }
            // bottom edge
            c = index(1, vert_count_2D - 1);
            e = index(2, vert_count_2D - 1);
            n = index(1, vert_count_2D - 2);
            w = index(0, vert_count_2D - 1);
            s = index(1, 0);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c++;
                e++;
                n++;
                w++;
                s++;
            }
            // left edge
            c = index(0, 1);
            e = index(1, 1);
            n = index(0, 0);
            w = index(horz_count_2D - 1, 1);
            s = index(0, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c += CX_2D; // Step down one row in maximun square.
                e += CX_2D; // Look in definition of index() in ca.hpp
                n += CX_2D; // to see that incrementing y adds CX_2D.
                w += CX_2D;
                s += CX_2D;
            }

            // right edge
            c = index(horz_count_2D - 1, 1);
            e = index(0, 1);
            n = index(horz_count_2D - 1, 0);
            w = index(horz_count_2D - 2, 1);
            s = index(horz_count_2D - 1, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c += CX_2D;
                e += CX_2D;
                n += CX_2D;
                w += CX_2D;
                s += CX_2D;
            }
        }
        else if (wrapflag == WF_FREE)
        {
            // upper left corner update
            (this->*UpdateCell_5)(index(0, 0), index(1, 0), index(0, 0), index(0, 0), index(0, 1));
            // upper right corner update
            (this->*UpdateCell_5)(index(horz_count_2D - 1, 0), index(horz_count_2D - 1, 0), index(horz_count_2D - 1, 0),
                index(horz_count_2D - 2, 0), index(horz_count_2D - 1, 1));
            // bottom left corner update
            (this->*UpdateCell_5)(index(0, vert_count_2D - 1), // c
                index(1, vert_count_2D - 1),                   // e
                index(0, vert_count_2D - 2),                   // n
                index(0, vert_count_2D - 1),                   // w
                index(0, vert_count_2D - 1));                  // s
            // bottom right corner update
            (this->*UpdateCell_5)(index(horz_count_2D - 1, vert_count_2D - 1),
                index(horz_count_2D - 1, vert_count_2D - 1), index(horz_count_2D - 1, vert_count_2D - 2), // n
                index(horz_count_2D - 2, vert_count_2D - 1),                                              // w
                index(horz_count_2D - 1, vert_count_2D - 1));
            // top edge
            c = index(1, 0);
            e = index(2, 0);
            n = index(1, 0);
            w = index(0, 0);
            s = index(1, 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c++;
                e++;
                n++;
                w++;
                s++;
            }
            // bottom edge
            c = index(1, vert_count_2D - 1);
            e = index(2, vert_count_2D - 1);
            n = index(1, vert_count_2D - 2);
            w = index(0, vert_count_2D - 1);
            s = index(1, vert_count_2D - 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c++;
                e++;
                n++;
                w++;
                s++;
            }
            // left edge
            c = index(0, 1);
            e = index(1, 1);
            n = index(0, 0);
            w = index(0, 1);
            s = index(0, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c += CX_2D; // Step down one row in maximun square.
                e += CX_2D; // Look in definition of index() in ca.hpp
                n += CX_2D; // to see that incrementing y adds CX_2D.
                w += CX_2D;
                s += CX_2D;
            }

            // right edge
            c = index(horz_count_2D - 1, 1);
            e = index(horz_count_2D - 1, 1);
            n = index(horz_count_2D - 1, 0);
            w = index(horz_count_2D - 2, 1);
            s = index(horz_count_2D - 1, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_5)(c, e, n, w, s);
                c += CX_2D;
                e += CX_2D;
                n += CX_2D;
                w += CX_2D;
                s += CX_2D;
            }
        }
        else if (wrapflag == WF_ABSORB) // mike 11-18-97
        {
            // the target values of the edges will be the source values of the
            // inner neighbors.
            int pitch;

            // handle the top and bottom edges
            // pitch is the difference in indexes between cells along opposite edges
            pitch = horz_count_2D * (vert_count_2D - 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                // top
                wave_target_plane[x].intensity = wave_source_plane[x + horz_count_2D].intensity;
                // bottom
                wave_target_plane[x + pitch].intensity = wave_source_plane[x + pitch - horz_count_2D].intensity;
            }

            // handle left and right edges
            pitch = horz_count_2D - 1;
            for (y = horz_count_2D; y < horz_count_2D * (vert_count_2D - 1); y += horz_count_2D)
            {
                // left
                wave_target_plane[y].intensity = wave_source_plane[y + 1].intensity;
                // right
                wave_target_plane[y + pitch].intensity = wave_source_plane[y + pitch - 1].intensity;
            }

            // handle corners
            // top left
            wave_target_plane[0].intensity = wave_source_plane[horz_count_2D + 1].intensity;
            // top right
            wave_target_plane[horz_count_2D - 1].intensity = wave_source_plane[2 * horz_count_2D - 1].intensity;
            // bottom_left
            wave_target_plane[horz_count_2D * (vert_count_2D - 1)].intensity =
                wave_source_plane[horz_count_2D * (vert_count_2D - 2) + 1].intensity;
            // bottom right
            wave_target_plane[horz_count_2D * vert_count_2D - 1].intensity =
                wave_source_plane[horz_count_2D * (vert_count_2D - 1) - 2].intensity;
        }
    }
    else //_smoothsteps case
        SmoothAverageStretch2D();
    WaveUpdateStep2D(hdc);
}
void CA::WaveUpdate2D_9(HDC hdc)
{
    int c, e, ne, n, nw, w, sw, s, se;
    int x, y;

    if (!_smoothsteps)
    {
        // Need to copy and change the WaveUpdate2D code
        for (y = 1; y < vert_count_2D - 1; y++)
        {
            c = index(1, y);
            e = index(2, y);
            ne = index(2, y - 1);
            n = index(1, y - 1);
            nw = index(0, y - 1);
            w = index(0, y);
            sw = index(0, y + 1), s = index(1, y + 1), se = index(2, y + 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c++;
                e++;
                ne++, n++;
                nw++;
                w++, sw++;
                s++;
                se++;
            }
        }
        if (wrapflag == WF_WRAP)
        {
            // upper left corner update
            (this->*UpdateCell_9)(index(0, 0),               // c
                index(1, 0),                                 // e
                index(1, vert_count_2D - 1),                 // ne
                index(0, vert_count_2D - 1),                 // n
                index(horz_count_2D - 1, vert_count_2D - 1), // nw
                index(horz_count_2D - 1, 0),                 // w
                index(horz_count_2D - 1, 1),                 // sw
                index(0, 1),                                 // s
                index(1, 1));                                // se
            // upper right corner update
            (this->*UpdateCell_9)(index(horz_count_2D - 1, 0), // c
                index(0, 0),                                   // e
                index(0, vert_count_2D - 1),                   // ne
                index(horz_count_2D - 1, vert_count_2D - 1),   // n
                index(horz_count_2D - 2, vert_count_2D - 1),   // nw
                index(horz_count_2D - 2, 0),                   // w
                index(horz_count_2D - 2, 1),                   // sw
                index(horz_count_2D - 1, 1),                   // s
                index(0, 1));                                  // se
            // bottom left corner update
            (this->*UpdateCell_9)(index(0, vert_count_2D - 1), // c
                index(1, vert_count_2D - 1),                   // e
                index(1, vert_count_2D - 2),                   // ne
                index(0, vert_count_2D - 2),                   // n
                index(horz_count_2D - 1, vert_count_2D - 2),   // nw
                index(horz_count_2D - 1, vert_count_2D - 1),   // w
                index(horz_count_2D - 1, 0),                   // sw
                index(0, 0),                                   // s
                index(1, 0));                                  // se
            // bottom right corner update
            (this->*UpdateCell_9)(index(horz_count_2D - 1, vert_count_2D - 1), index(0, vert_count_2D - 1), // e
                index(0, vert_count_2D - 2),                                                                // ne
                index(horz_count_2D - 1, vert_count_2D - 2),                                                // n
                index(horz_count_2D - 2, vert_count_2D - 2),                                                // nw
                index(horz_count_2D - 2, vert_count_2D - 1),                                                // w
                index(horz_count_2D - 2, 0),                                                                // sw
                index(horz_count_2D - 1, 0),                                                                // s
                index(0, 0));                                                                               // se
            // top edge
            c = index(1, 0);
            e = index(2, 0);
            ne = index(2, vert_count_2D - 1);
            n = index(1, vert_count_2D - 1);
            nw = index(0, vert_count_2D - 1);
            w = index(0, 0);
            sw = index(0, 1);
            s = index(1, 1);
            se = index(2, 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c++;
                e++;
                ne++, n++;
                nw++;
                w++, sw++;
                s++;
                se++;
            }
            // bottom edge
            c = index(1, vert_count_2D - 1);
            e = index(2, vert_count_2D - 1);
            ne = index(2, vert_count_2D - 2);
            n = index(1, vert_count_2D - 2);
            nw = index(0, vert_count_2D - 2);
            w = index(0, vert_count_2D - 1);
            sw = index(0, 0);
            s = index(1, 0);
            se = index(2, 0);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c++;
                e++;
                ne++, n++;
                nw++;
                w++, sw++;
                s++;
                se++;
            }
            // left edge
            c = index(0, 1);
            e = index(1, 1);
            ne = index(1, 0);
            n = index(0, 0);
            nw = index(horz_count_2D - 1, 0);
            w = index(horz_count_2D - 1, 1);
            sw = index(horz_count_2D - 1, 2);
            s = index(0, 2);
            se = index(1, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c += CX_2D; // Step down one row in maximun square.
                e += CX_2D; // Look in definition of index() in ca.hpp
                n += CX_2D; // to see that incrementing y adds CX_2D.
                w += CX_2D;
                s += CX_2D;
                ne += CX_2D;
                nw += CX_2D;
                se += CX_2D;
                sw += CX_2D;
            }

            // right edge
            c = index(horz_count_2D - 1, 1);
            e = index(0, 1);
            ne = index(0, 0);
            n = index(horz_count_2D - 1, 0);
            nw = index(horz_count_2D - 2, 0);
            w = index(horz_count_2D - 2, 1);
            sw = index(horz_count_2D - 2, 2);
            s = index(horz_count_2D - 1, 2);
            se = index(0, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c += CX_2D;
                e += CX_2D;
                n += CX_2D;
                w += CX_2D;
                s += CX_2D;
                ne += CX_2D;
                nw += CX_2D;
                se += CX_2D;
                sw += CX_2D;
            }
        }
        else if (wrapflag == WF_FREE)
        {                                      // upper left corner update
            (this->*UpdateCell_9)(index(0, 0), // c
                index(1, 0),                   // e
                index(0, 0),                   // ne
                index(0, 0),                   // n
                index(0, 0),                   // nw
                index(0, 0),                   // w
                index(0, 1),                   // sw
                index(0, 1),                   // s
                index(1, 1));                  // se
            // upper right corner update
            (this->*UpdateCell_9)(index(horz_count_2D - 1, 0), // c
                index(horz_count_2D - 1, 0),                   // e
                index(horz_count_2D - 1, 0),                   // ne
                index(horz_count_2D - 1, 0),                   // n
                index(horz_count_2D - 2, 0),                   // nw
                index(horz_count_2D - 2, 0),                   // w
                index(horz_count_2D - 2, 1),                   // sw
                index(horz_count_2D - 1, 1),                   // s
                index(horz_count_2D - 1, 1));                  // se
            // bottom left corner update
            (this->*UpdateCell_9)(index(0, vert_count_2D - 1), // c
                index(1, vert_count_2D - 1),                   // e
                index(1, vert_count_2D - 2),                   // ne
                index(0, vert_count_2D - 2),                   // n
                index(0, vert_count_2D - 2),                   // nw
                index(0, vert_count_2D - 1),                   // w
                index(0, vert_count_2D - 1),                   // sw
                index(0, vert_count_2D - 1),                   // s
                index(1, vert_count_2D - 1));                  // se
            // bottom right corner update
            (this->*UpdateCell_9)(index(horz_count_2D - 1, vert_count_2D - 1),
                index(horz_count_2D - 1, vert_count_2D - 1),  // e
                index(horz_count_2D - 1, vert_count_2D - 2),  // ne
                index(horz_count_2D - 1, vert_count_2D - 2),  // n
                index(horz_count_2D - 2, vert_count_2D - 2),  // nw
                index(horz_count_2D - 2, vert_count_2D - 1),  // w
                index(horz_count_2D - 2, vert_count_2D - 1),  // sw
                index(horz_count_2D - 1, vert_count_2D - 1),  // s
                index(horz_count_2D - 1, vert_count_2D - 1)); // se
            // top edge
            c = index(1, 0);
            e = index(2, 0);
            ne = index(2, 0);
            n = index(1, 0);
            nw = index(0, 0);
            w = index(0, 0);
            sw = index(0, 1);
            s = index(1, 1);
            se = index(2, 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c++;
                e++;
                ne++, n++;
                nw++;
                w++, sw++;
                s++;
                se++;
            }
            // bottom edge
            c = index(1, vert_count_2D - 1);
            e = index(2, vert_count_2D - 1);
            ne = index(2, vert_count_2D - 2);
            n = index(1, vert_count_2D - 2);
            nw = index(0, vert_count_2D - 2);
            w = index(0, vert_count_2D - 1);
            sw = index(0, vert_count_2D - 1);
            s = index(1, vert_count_2D - 1);
            se = index(2, vert_count_2D - 1);
            for (x = 1; x < horz_count_2D - 1; x++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c++;
                e++;
                ne++, n++;
                nw++;
                w++, sw++;
                s++;
                se++;
            }
            // left edge
            c = index(0, 1);
            e = index(1, 1);
            ne = index(1, 0);
            n = index(0, 0);
            nw = index(0, 0);
            w = index(0, 1);
            sw = index(0, 2);
            s = index(0, 2);
            se = index(1, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c += CX_2D; // Step down one row in maximun square.
                e += CX_2D; // Look in definition of index() in ca.hpp
                n += CX_2D; // to see that incrementing y adds CX_2D.
                w += CX_2D;
                s += CX_2D;
                ne += CX_2D;
                nw += CX_2D;
                se += CX_2D;
                sw += CX_2D;
            }

            // right edge
            c = index(horz_count_2D - 1, 1);
            e = index(horz_count_2D - 1, 1);
            ne = index(horz_count_2D - 1, 0);
            n = index(horz_count_2D - 1, 0);
            nw = index(horz_count_2D - 2, 0);
            w = index(horz_count_2D - 2, 1);
            sw = index(horz_count_2D - 2, 2);
            s = index(horz_count_2D - 1, 2);
            se = index(horz_count_2D - 1, 2);
            for (y = 1; y < vert_count_2D - 1; y++)
            {
                (this->*UpdateCell_9)(c, e, ne, n, nw, w, sw, s, se);
                c += CX_2D;
                e += CX_2D;
                n += CX_2D;
                w += CX_2D;
                s += CX_2D;
                ne += CX_2D;
                nw += CX_2D;
                se += CX_2D;
                sw += CX_2D;
            }
        }
    }
    else //_smoothsteps case
        SmoothAverageStretch2D();
    WaveUpdateStep2D(hdc);
}
//======================Network Update========================
void CA::NetworkUpdate(HDC hdc)
{
}

void CA::GetCAStyleName(char CA_Style_Name[])
{
    strcpy(CA_Style_Name, _castylename);
}

void CA::SetCAStyleName(char CA_Style_Name[])
{
    strcpy(_castylename, CA_Style_Name);
}

void CA::GetUserRuleName(char UserRuleName[])
{
    char *endp, temp[256] = {'\0'};

    strcpy(UserRuleName, temp); // initializes buffer to null

    if (_userrulename == NULL) // is there a userrulename?
        return;

    strcpy(temp, _userrulename); //  copy full path name to temp buffer

    if (!(strchr(temp, '\\'))) // if there are no '\' than there is no legal path return null
        UserRuleName[0] = '\0';
    else
    {
        while (endp = strchr(temp, '\\')) // otherwise find the last one
            strcpy(temp, ++endp);
        strcpy(UserRuleName, temp); // copy everything to the right of the last '\' into variable
        UserRuleName[strlen(temp)] = '\0';
    }
    return;
}
