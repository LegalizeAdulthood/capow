add_library(CAScreen SHARED
    Autorand.cpp
    Bitmap.cpp
    ca.cpp
    Calist.cpp
    camore.cpp
    CapowGL.cpp
    CAScreen.cpp
    Configure.cpp
    Dllmem.cpp
    Fourier.cpp
    Label.cpp
    Loadsav7.cpp
    Loadsave.cpp
    Palette.cpp
    Random.cpp
    Seed.cpp
    Smooth.cpp
    Touch.cpp
    Tweak.cpp
    Userpara.cpp
    CAPOW.ICO
    CAScreen.rc
)
target_link_libraries(CAScreen PUBLIC Glu32.lib opengl32.lib ComCtl32.lib Lz32.lib Winmm.lib Scrnsave.lib)
set_target_properties(CAScreen PROPERTIES SUFFIX ".scr")
