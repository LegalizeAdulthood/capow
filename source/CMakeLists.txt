add_library(CapowCore STATIC
    Autorand.cpp
    Bitmap.hpp Bitmap.cpp
    ca.hpp ca.cpp
    Calist.cpp
    camore.cpp
    CapowGL.hpp CapowGL.cpp
    Carule.hpp
    Dllmem.cpp
    Fourier.cpp
    Loadsav7.cpp
    Loadsave.cpp
    Palette.cpp
    Random.h Random.cpp
    resource.h
    Seed.cpp
    Smooth.cpp
    Touch.cpp
    Tweak.hpp Tweak.cpp
    Tweakca.hpp
    Types.h
)
target_folder(CapowCore "Libraries")

add_library(CapowGui STATIC
    About.cpp
    Analog.cpp
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

add_library(Carule SHARED
    Carule.cpp
    Caruleuse.cpp
    Palette.cpp
    Random.cpp
    Tweak.cpp
    Carule.hpp
)

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
    capow.ico
    CAScreen.rc
)
target_link_libraries(CAScreen PUBLIC Glu32.lib opengl32.lib ComCtl32.lib Lz32.lib Winmm.lib Scrnsave.lib)
set_target_properties(CAScreen PROPERTIES SUFFIX ".scr")
