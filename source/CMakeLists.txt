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
