cmake_minimum_required(VERSION 3.24)

project(FieldMouse LANGUAGES CXX RC)

if(NOT WIN32)
  message(FATAL_ERROR "field-mouse only supports Windows.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "field-mouse only supports 64-bit Windows builds.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(FieldMouse WIN32
  src/app_state.cpp
  src/input.cpp
  src/main.cpp
  src/settings.cpp
  src/windows.cpp
  src/field_mouse.rc
)

target_include_directories(FieldMouse PRIVATE
  include
  include/field_mouse
)

target_compile_definitions(FieldMouse PRIVATE
  UNICODE
  _UNICODE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  _WINDOWS
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

if(MSVC)
  target_compile_options(FieldMouse PRIVATE
    /W4
    /permissive-
    /sdl
    /EHsc
  )

  target_link_options(FieldMouse PRIVATE
    /SUBSYSTEM:WINDOWS
    /MANIFEST:EMBED
    /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/app.manifest
    $<$<CONFIG:Debug>:/INCREMENTAL>
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
  )
endif()

target_link_libraries(FieldMouse PRIVATE
  user32
  shell32
)

set_target_properties(FieldMouse PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin/Release
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/bin/Release
)
