cmake_minimum_required(VERSION 3.20)

project(gav_gif_avatar_animator LANGUAGES CXX)

# Require Windows 10 APIs
add_definitions(-DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use static runtime for single .exe (/MT)
if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/W4 /EHsc /permissive- /utf-8)
  add_link_options(/SUBSYSTEM:WINDOWS)
endif()

file(GLOB SRC_FILES CONFIGURE_DEPENDS
  src/*.cpp
)

add_executable(gav_gif ${SRC_FILES})

target_include_directories(gav_gif PRIVATE src)

target_link_libraries(gav_gif
  PRIVATE
    ole32
    oleaut32
    uuid
    windowscodecs  # WIC
    mmdevapi       # MMDevice
    avrt           # MMCSS
    winmm          # timeBeginPeriod
    gdi32
    Msimg32       # AlphaBlend for per-pixel alpha
    Comdlg32       # file dialogs
    Comctl32       # common controls (trackbar)
    UxTheme
    Shlwapi        # ini path helpers
)


