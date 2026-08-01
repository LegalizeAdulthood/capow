add_library(CapowCore STATIC
    About.cpp
    Analog.cpp
    Autorand.cpp
    Bitmap.cpp
    ca.cpp
    Calist.cpp
    camore.cpp
    Capow.cpp
    Capow.hpp
    CapowGL.cpp
    Cell.cpp
    Color.cpp
    configure.cpp
    #Controlb.cpp
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
    Tweak.hpp
    Tweakca.hpp
    Types.h
    Userpara.hpp
)
target_link_libraries(CapowCore PUBLIC Glu32.lib opengl32.lib ComCtl32.lib Lz32.lib Winmm.lib)
target_folder(CapowCore "Libraries")

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
target_link_libraries(Capow PUBLIC CapowCore)

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
