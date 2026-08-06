add_library(CapowCore STATIC
    Autorand.cpp
    BatchOptions.hpp BatchOptions.cpp
    CapowRules.hpp
    ca.hpp ca.cpp
    Calist.cpp
    camore.cpp
    CapowGL.hpp CapowGL.cpp
    Carule.hpp
    Dllmem.cpp
    Fourier.cpp
    ImageBuffer.hpp ImageBuffer.cpp
    Loadsav7.cpp
    Loadsave.cpp
    OpenGlImagePresenter.hpp OpenGlImagePresenter.cpp
    Palette.cpp
    Random.h Random.cpp
    resource.h
    Seed.cpp
    Smooth.cpp
    Touch.cpp
    Tweak.hpp Tweak.cpp
    Tweakca.hpp
    Types.h
    WavePlaneImage.hpp WavePlaneImage.cpp
)
target_folder(CapowCore "Libraries")

add_library(CapowGui STATIC
    About.cpp
    Analog.cpp
    BatchImage.hpp BatchImage.cpp
    BatchRunner.hpp BatchRunner.cpp
    Capow.hpp Capow.cpp
    Cell.cpp
    Color.cpp
    Comcthlp.h
    Configure.cpp
    Cycle.cpp
    Digital.cpp
    Electric.cpp
    Exp.cpp
    FileSave.cpp
    Gendlg.cpp
    GUI.hpp
    Label.cpp
    Opengldlg.cpp
    status.hpp status.cpp
    toolbar.hpp toolbar.cpp
    Userpara.hpp Userpara.cpp
    View.cpp
    World.cpp
)
target_link_libraries(CapowGui PUBLIC CapowCore)
target_folder(CapowGui "Libraries")

if(CAPOW_ENABLE_ALPAKA)
    alpaka_add_library(CapowAlpaka STATIC
        AlpakaBackend.hpp
        AlpakaBackend.cpp
        AlpakaBuffers.hpp
        AlpakaBuffers.cpp
        AlpakaHeat2D.hpp
        AlpakaHeat2D.cpp
        AlpakaHeat2DLive.hpp
        AlpakaHeat2DLive.cpp
        AlpakaSyntheticHeat.hpp
        AlpakaSyntheticHeat.cpp
        AlpakaWave2D.hpp
        AlpakaWave2D.cpp
        CapowRules.hpp
        AlpakaUtilities.hpp
    )
    target_link_libraries(CapowAlpaka PRIVATE alpaka::alpaka)
    target_link_libraries(CapowAlpaka PUBLIC opengl32.lib)
    target_compile_definitions(CapowAlpaka PUBLIC CAPOW_ENABLE_ALPAKA=1)
    target_folder(CapowAlpaka "Libraries")
    target_link_libraries(CapowCore PUBLIC CapowAlpaka)
    target_link_libraries(CapowGui PUBLIC CapowAlpaka)
endif()

add_executable(Capow WIN32
    About.bmp
    actionbar.bmp
    capow.ico
    seedbitm.bmp
    toolbar.bmp
    Bolt.cur
    Copy.cur
    cursor1.cur
    cursor2.cur
    generator.cur
    Touch.cur
    Capow32.rc
    WinMain.cpp
)
target_link_libraries(Capow PUBLIC CapowGui CapowCore Glu32.lib opengl32.lib ComCtl32.lib Lz32.lib Winmm.lib)
if(CAPOW_ENABLE_ALPAKA)
    target_link_libraries(Capow PUBLIC CapowAlpaka alpaka::alpaka)
endif()

add_library(UserRuleCore STATIC
    Caruleuse.cpp
    Palette.cpp
    Random.cpp
    Tweak.cpp
    Carule.hpp
)
target_folder(UserRuleCore "Libraries")

add_library(Carule SHARED Carule.cpp)
target_link_libraries(Carule PRIVATE UserRuleCore)

function(add_carule_rule target rule output_name)
    add_library(${target} SHARED Carule.cpp)
    target_compile_definitions(${target} PRIVATE CARULE_EXTERNAL_RULE ${rule})
    target_link_libraries(${target} PRIVATE UserRuleCore)
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${output_name}")
    target_folder(${target} "User Rules")
endfunction()

add_carule_rule(CaruleWave CARULE_WAVE "Wave")
add_carule_rule(CaruleWaveSimple CARULE_WAVE_SIMPLE "Wave Unstable")
add_carule_rule(CaruleWaveAvg1 CARULE_WAVE_AVG_1 "Wave Half")
add_carule_rule(CaruleWaveAvg2 CARULE_WAVE_AVG_2 "Wave Third")
add_carule_rule(CaruleWaveWrap CARULE_WAVE_WRAP "Wave Unstable Wrap")
add_carule_rule(CaruleOscillatorChaotic CARULE_OSCILLATOR_CHAOTIC "Oscillator Chaotic")
add_carule_rule(CaruleOscillatorWaveChaotic CARULE_OSCILLATOR_WAVE_CHAOTIC "Oscillator Wave Chaotic")
add_carule_rule(CaruleReactionDiffusion CARULE_REACTION_DIFFUSION "Reaction Diffusion")
add_carule_rule(CaruleReactionWave CARULE_REACTION_WAVE "Reaction Wave")
add_carule_rule(CaruleWaveQuadratic CARULE_WAVE_QUADRATIC "Quadratic Wave")
add_carule_rule(CaruleWaveCubic CARULE_WAVE_CUBIC "General Cubic Wave")
add_carule_rule(Carule2DWaveQuadratic CARULE_2D_WAVE_QUADRATIC "2D Quadratic Wave")
add_carule_rule(Carule2DWaveCubic CARULE_2D_WAVE_CUBIC "2D Cubic Wave")
add_carule_rule(Carule2DHeat9 CARULE_2D_HEAT_9 "2D Heat 9-Neighbor")
add_carule_rule(Carule2DHeat5 CARULE_2D_HEAT_5 "2D Heat 5-Neighbor")
add_carule_rule(Carule2DLife CARULE_2D_LIFE "2D Life")
add_carule_rule(Carule2DHodge CARULE_2D_HODGE "2D Hodge")
add_carule_rule(Carule2DHodgeWave CARULE_2D_HODGE_WAVE "2D Hodge Wave")
add_carule_rule(Carule2DPair CARULE_2D_PAIR "2D Pair")
add_carule_rule(Carule2DActivatorInhibitor CARULE_2D_ACTIVATOR_INHIBITOR "2D Activator Inhibitor 9")
add_carule_rule(Carule2DActivatorInhibitorMin CARULE_2D_ACTIVATOR_INHIBITOR_MIN "2D Activator Inhibitor Min 9")
add_carule_rule(Carule2DActivatorInhibitor5 CARULE_2D_ACTIVATOR_INHIBITOR_5 "2D Activator Inhibitor 5")
add_carule_rule(Carule2DActivatorInhibitorSaturation CARULE_2D_ACTIVATOR_INHIBITOR_SATURATION "2D Activator Inhibitor Saturation 9")
add_carule_rule(Carule2DActivatorInhibitorSaturation5 CARULE_2D_ACTIVATOR_INHIBITOR_SATURATION_5 "2D Activator Inhibitor Saturation 5")
add_carule_rule(Carule2DActivatorInhibitorWaveWave CARULE_2D_ACTIVATOR_INHIBITOR_WAVE_WAVE "2D Activator Inhibitor Wave")
add_carule_rule(Carule2DActivatorInhibitorWaveDiffuse CARULE_2D_ACTIVATOR_INHIBITOR_WAVE_DIFFUSE "2D Activator Wave Inhibitor Diffuse")
add_carule_rule(Carule2DAisWaveDiffuse CARULE_2D_AIS_WAVE_DIFFUSE "2D AIS Wave Diffuse")
add_carule_rule(Carule2DActivatorInhibitorDiffuseWave CARULE_2D_ACTIVATOR_INHIBITOR_DIFFUSE_WAVE "2D Activator Diffuse Inhibitor Wave")
add_carule_rule(Carule2DOscillatorChaotic CARULE_2D_OSCILLATOR_CHAOTIC "2D Oscillator Chaotic")
add_carule_rule(Carule2DOscillatorWaveChaotic CARULE_2D_OSCILLATOR_WAVE_CHAOTIC "2D Oscillator Wave Chaotic")
add_carule_rule(Carule2DOscillator CARULE_2D_OSCILLATOR "2D Oscillator")
add_carule_rule(Carule2DOscillatorWave CARULE_2D_OSCILLATOR_WAVE "2D Oscillator Wave")
add_carule_rule(Carule2DBoilingWave5 CARULE_2D_BOILING_WAVE_5 "2D Boiling Wave 5")
add_carule_rule(Carule2DBoilingWave9 CARULE_2D_BOILING_WAVE_9 "2D Boiling Wave")
add_carule_rule(Carule2DBoilingWave9Smooth CARULE_2D_BOILING_WAVE_9_SMOOTH "2D Boiling Wave Smooth")
add_carule_rule(Carule1DInterpolated CARULE_1D_INTERPOLATED "1D Interpolated Continuous Rule")
add_carule_rule(CaruleAsymmetricHeat CARULE_ASYMMETRIC_HEAT "1D Asymmetric Heat")
add_carule_rule(CaruleTwoRegimeHeat CARULE_TWOREGIME_HEAT "1D Two-Regime Heat")
add_carule_rule(Carule2DLogistic CARULE_2D_LOGISTIC "2D Logistic Diffusion 5")
add_carule_rule(Carule2DLogisticAbraham CARULE_2D_LOGISTIC_ABRAHAM "2D Logistic Diffusion Abraham 5")
add_carule_rule(Carule2DDoubleLogistic CARULE_2D_DOUBLE_LOGISTIC "2D Logistic Double")
add_carule_rule(Carule2DDoubleLogisticSmooth CARULE_2D_DOUBLE_LOGISTIC_SMOOTH "2D Logistic Double Smooth")
add_carule_rule(Carule2DWinfreeLogistic CARULE_2D_WINFREE_LOGISTIC "2D Winfree Logistic")
add_carule_rule(Carule2DActivatorInhibitorBrain CARULE_2D_ACTIVATOR_INHIBITOR_BRAIN "2D Brain AI Zhabo")
add_carule_rule(Carule2DHodgeBrain CARULE_2D_HODGE_BRAIN "2D Brain Hodge")
add_carule_rule(Carule2DSandpile5 CARULE_2D_SANDPILE_5 "2D Sandpile 5")
add_carule_rule(Carule2DSandpile9 CARULE_2D_SANDPILE_9 "2D Sandpile 9")
add_carule_rule(Carule2DWaveSandpile5 CARULE_2D_WAVE_SANDPILE_5 "2D Sandpile Wave 5")
add_carule_rule(Carule2DCityFormation CARULE_2D_CITYFORMATION "2D City Formation 9")
add_carule_rule(Carule2DCityFormation5 CARULE_2D_CITYFORMATION_5 "2D City Formation 5")
add_carule_rule(Carule2DCityFormation5BadVersion CARULE_2D_CITYFORMATION_5_BADVERSION "2D City Formation Delay 5")
add_carule_rule(Carule2DForestFire CARULE_2D_FORESTFIRE "2D Forest Fire 5")
add_carule_rule(Carule2DWinfreeZhaboNew CARULE_2D_WINFREE_ZHABO_NEW "2D Winfree Zhabo New")

add_library(CAScreen SHARED
    Autorand.cpp
    ca.cpp
    Calist.cpp
    camore.cpp
    CapowGL.cpp
    CAScreen.cpp
    Configure.cpp
    Dllmem.cpp
    Fourier.cpp
    ImageBuffer.cpp
    Label.cpp
    Loadsav7.cpp
    Loadsave.cpp
    OpenGlImagePresenter.cpp
    Palette.cpp
    Random.cpp
    Seed.cpp
    Smooth.cpp
    Touch.cpp
    Tweak.cpp
    Userpara.cpp
    WavePlaneImage.cpp
    capow.ico
    CAScreen.rc
)
target_link_libraries(CAScreen PUBLIC Glu32.lib opengl32.lib ComCtl32.lib Lz32.lib Winmm.lib Scrnsave.lib)
set_target_properties(CAScreen PROPERTIES SUFFIX ".scr")
