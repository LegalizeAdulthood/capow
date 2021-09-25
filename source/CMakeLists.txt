cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(capow)

find_package(OpenGL REQUIRED)

add_executable(capow WIN32 
    ABOUT.BMP
    actionbar.bmp
    CAPOW.ICO
    seedbitm.bmp
    toolbar.bmp
    BOLT.CUR
    COPY.CUR
    cursor1.cur
    cursor2.cur
    generator.cur
    TOUCH.CUR
    Capow32.rc
    About.cpp
    Analog.cpp
    Autorand.cpp
    Bitmap.cpp
    ca.cpp
    Calist.cpp
    camore.cpp
    Capow.cpp
    CapowGL.cpp
    Cell.cpp
    Color.cpp
    configure.cpp
    Cycle.cpp
    Digital.cpp
    Dllmem.cpp
    Electric.cpp
    Exp.cpp
    FileSave.cpp
    Fourier.cpp
    Gendlg.cpp
    Label.cpp
    Loadsav7.cpp
    Loadsave.cpp
    Opengldlg.cpp
    Palette.cpp
    Random.cpp
    Seed.cpp
    Smooth.cpp
    status.cpp
    toolbar.cpp
    Touch.cpp
    Tweak.cpp
    Userpara.cpp
    View.cpp
    World.cpp
    Bitmap.hpp
    ca.hpp
    CapowGL.hpp
    Carule.hpp
    Comcthlp.h
    GUI.hpp
    Random.h
    resource.h
    status.hpp
    TWEAK.HPP
    Tweakca.hpp
    Types.h
    USERPARA.HPP
)
target_link_libraries(capow PUBLIC OpenGL::GLU OpenGL::GL winmm.lib lz32.lib Comctl32.lib)

add_library(rule STATIC
        Caruleuse.cpp
        Palette.cpp
        Random.cpp
        Tweak.cpp
)

function(add_ca_rule lib_name dll_name def_name)
    add_library(${lib_name} SHARED
        Carule.cpp
        Carule.hpp
    )
    set_target_properties(${lib_name} PROPERTIES OUTPUT_NAME ${dll_name})
    target_compile_definitions(${lib_name} PRIVATE ${def_name})
    target_link_libraries(${lib_name} PUBLIC rule)
endfunction()

add_ca_rule(WAVE Wave CARULE_WAVE)
add_ca_rule(WAVE_SIMPLE "Wave Unstable" CARULE_WAVE_SIMPLE)
add_ca_rule(WAVE_AVG_1 "Wave Half" CARULE_WAVE_AVG_1)
add_ca_rule(WAVE_AVG_2 "Wave Third" CARULE_WAVE_AVG_2)
add_ca_rule(WAVE_WRAP "Wave Unstable Wrap" CARULE_WAVE_WRAP)
add_ca_rule(OSCILLATOR_WAVE "Oscillator Wave" CARULE_OSCILLATOR_WAVE)
add_ca_rule(OSCILLATOR_CHAOTIC "Oscillator Chaotic" CARULE_OSCILLATOR_CHAOTIC)
add_ca_rule(OSCILLATOR_WAVE_CHAOTIC "Oscillator Wave Chaotic" CARULE_OSCILLATOR_WAVE_CHAOTIC)
add_ca_rule(REACTION_DIFFUSION "Reaction Diffusion" CARULE_REACTION_DIFFUSION)
add_ca_rule(REACTION_WAVE "Reaction Wave" CARULE_REACTION_WAVE)
add_ca_rule(WAVE_QUADRATIC "Quadratic Wave" CARULE_WAVE_QUADRATIC)
add_ca_rule(WAVE_CUBIC "General Cubic Wave" CARULE_WAVE_CUBIC)
add_ca_rule(2D_WAVE_QUADRATIC "2D Quadratic Wave" CARULE_2D_WAVE_QUADRATIC)
add_ca_rule(2D_WAVE_CUBIC "2D Cubic Wave" CARULE_2D_WAVE_CUBIC)
add_ca_rule(2D_HEAT_9 "2D Heat 9-Neighbor" CARULE_2D_HEAT_9)
add_ca_rule(2D_HEAT_5 "2D Heat 5-Neighbor" CARULE_2D_HEAT_5)
add_ca_rule(2D_LIFE "2D Life" CARULE_2D_LIFE)
add_ca_rule(2D_HODGE "2D Hodge" CARULE_2D_HODGE)
add_ca_rule(2D_HODGE_WAVE "2D Hodge Wave" CARULE_2D_HODGE_WAVE)
add_ca_rule(2D_PAIR "2D Pair" CARULE_2D_PAIR)
add_ca_rule(2D_ACTIVATOR_INHIBITOR "2D Activator Inhibitor 9" CARULE_2D_ACTIVATOR_INHIBITOR)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_MIN "2D Activator Inhibitor Min 9" CARULE_2D_ACTIVATOR_INHIBITOR_MIN)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_5 "2D Activator Inhibitor 5" CARULE_2D_ACTIVATOR_INHIBITOR_5)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_SATURATION "2D Activator Inhibitor Saturation 9" CARULE_2D_ACTIVATOR_INHIBITOR_SATURATION)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_SATURATION_5 "2D Activator Inhibitor Saturation 5" CARULE_2D_ACTIVATOR_INHIBITOR_SATURATION_5)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_WAVE_WAVE "2D Activator Inhibitor Wave" CARULE_2D_ACTIVATOR_INHIBITOR_WAVE_WAVE)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_WAVE_DIFFUSE "2D Activator Wave Inhibitor Diffuse" CARULE_2D_ACTIVATOR_INHIBITOR_WAVE_DIFFUSE)
add_ca_rule(2D_AIS_WAVE_DIFFUSE "2D AIS Wave Diffuse" CARULE_2D_AIS_WAVE_DIFFUSE)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_DIFFUSE_WAVE "2D Activator Diffuse Inhibitor Wave" CARULE_2D_ACTIVATOR_INHIBITOR_DIFFUSE_WAVE)
add_ca_rule(2D_OSCILLATOR_CHAOTIC "2D Oscillator Chaotic" CARULE_2D_OSCILLATOR_CHAOTIC)
add_ca_rule(2D_OSCILLATOR_WAVE_CHAOTIC "2D Oscillator Wave Chaotic" CARULE_2D_OSCILLATOR_WAVE_CHAOTIC)
add_ca_rule(2D_OSCILLATOR "2D Oscillator" CARULE_2D_OSCILLATOR)
add_ca_rule(2D_OSCILLATOR_WAVE "2D Oscillator Wave" CARULE_2D_OSCILLATOR_WAVE)
add_ca_rule(2D_BOILING_WAVE_5 "2D Boiling Wave 5" CARULE_2D_BOILING_WAVE_5)
add_ca_rule(2D_BOILING_WAVE_9 "2D Boiling Wave" CARULE_2D_BOILING_WAVE_9)
add_ca_rule(2D_BOILING_WAVE_9_SMOOTH "2D Boiling Wave Smooth" CARULE_2D_BOILING_WAVE_9_SMOOTH)
add_ca_rule(1D_INTERPOLATED "1D Interpolated Continuous Rule" CARULE_1D_INTERPOLATED)
add_ca_rule(ASYMMETRIC_HEAT "1D Asymmetric Heat" CARULE_ASYMMETRIC_HEAT)
add_ca_rule(TWOREGIME_HEAT "1D Two-Regime Heat" CARULE_TWOREGIME_HEAT)
add_ca_rule(2D_LOGISTIC "2D Logistic Diffusion 5" CARULE_2D_LOGISTIC)
add_ca_rule(2D_LOGISTIC_ABRAHAM "2D Logistic Diffusion Abraham 5" CARULE_2D_LOGISTIC_ABRAHAM)
add_ca_rule(2D_DOUBLE_LOGISTIC "2D Logistic Double" CARULE_2D_DOUBLE_LOGISTIC)
add_ca_rule(2D_DOUBLE_LOGISTIC_SMOOTH "2D Logistic Double Smooth" CARULE_2D_DOUBLE_LOGISTIC_SMOOTH)
add_ca_rule(2D_WINFREE_LOGISTIC "2D Logistic Double Smooth" CARULE_2D_WINFREE_LOGISTIC)
add_ca_rule(2D_ACTIVATOR_INHIBITOR_BRAIN "2D Brain AI Zhabo" CARULE_2D_ACTIVATOR_INHIBITOR_BRAIN)
add_ca_rule(2D_HODGE_BRAIN "2D Brain Hodge" CARULE_2D_HODGE_BRAIN)
add_ca_rule(2D_SANDPILE_5 "2D Sandpile 5" CARULE_2D_SANDPILE_5)
add_ca_rule(2D_SANDPILE_9 "2D Sandpile 9" CARULE_2D_SANDPILE_9)
add_ca_rule(2D_WAVE_SANDPILE_5 "2D Sandpile Wave 5" CARULE_2D_WAVE_SANDPILE_5)
add_ca_rule(2D_CITYFORMATION "2D City Formation 9" CARULE_2D_CITYFORMATION)
add_ca_rule(2D_CITYFORMATION_5 "2D City Formation 5" CARULE_2D_CITYFORMATION_5)
add_ca_rule(2D_CITYFORMATION_5_BADVERSION "2D City Formation Delay 5" CARULE_2D_CITYFORMATION_5_BADVERSION)
add_ca_rule(2D_FORESTFIRE "2D Forest Fire 5" CARULE_2D_FORESTFIRE)
add_ca_rule(2D_WINFREE_ZHABO_NEW "2D Winfree Zhabo New" CARULE_2D_WINFREE_ZHABO_NEW)
