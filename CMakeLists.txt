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
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

add_executable(FieldMouse WIN32
  src/app_state.cpp
  src/config_window.cpp
  src/input.cpp
  src/main.cpp
  src/resources.qrc
  src/settings.cpp
  src/windows.cpp
  src/app.manifest
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
    $<$<CONFIG:Debug>:/INCREMENTAL>
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
  )
endif()

target_link_libraries(FieldMouse PRIVATE
  Qt6::Widgets
  user32
  shell32
)

set(_qt_windeployqt_hints "")

if(DEFINED ENV{QT_ROOT_DIR} AND NOT "$ENV{QT_ROOT_DIR}" STREQUAL "")
  file(TO_CMAKE_PATH "$ENV{QT_ROOT_DIR}" _qt_env_root_dir)
  list(APPEND _qt_windeployqt_hints
    "${_qt_env_root_dir}/bin"
  )
endif()

if(DEFINED Qt6_DIR AND NOT "${Qt6_DIR}" STREQUAL "")
  get_filename_component(_qt_cmake_dir "${Qt6_DIR}" DIRECTORY)
  get_filename_component(_qt_lib_dir "${_qt_cmake_dir}" DIRECTORY)
  get_filename_component(_qt_root_dir "${_qt_lib_dir}" DIRECTORY)

  list(APPEND _qt_windeployqt_hints
    "${_qt_root_dir}/bin"
    "${_qt_root_dir}/tools/qtbase/bin"
    "${_qt_root_dir}/tools/qt6/bin"
    "${_qt_root_dir}/tools/Qt6/bin"
  )
endif()

list(REMOVE_DUPLICATES _qt_windeployqt_hints)

find_program(_qt_deploy_tool
  NAMES windeployqt windeployqt.exe windeployqt6 windeployqt6.exe
  HINTS ${_qt_windeployqt_hints}
)

add_custom_command(TARGET FieldMouse POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "$<TARGET_FILE_DIR:FieldMouse>/LICENSE"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    "$<TARGET_FILE_DIR:FieldMouse>/THIRD_PARTY_NOTICES.md"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/LGPL_COMPLIANCE.md"
    "$<TARGET_FILE_DIR:FieldMouse>/LGPL_COMPLIANCE.md"
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:FieldMouse>/LICENSES"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/LICENSES/LGPL-3.0.txt"
    "$<TARGET_FILE_DIR:FieldMouse>/LICENSES/LGPL-3.0.txt"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0.txt"
    "$<TARGET_FILE_DIR:FieldMouse>/LICENSES/GPL-3.0.txt"
  VERBATIM
)

if(_qt_deploy_tool)
  add_custom_command(TARGET FieldMouse POST_BUILD
    COMMAND "${_qt_deploy_tool}"
      --dir "$<TARGET_FILE_DIR:FieldMouse>"
      --no-translations
      --no-system-d3d-compiler
      --no-opengl-sw
      "$<TARGET_FILE:FieldMouse>"
    VERBATIM
  )
else()
  message(WARNING "windeployqt was not found. FieldMouse will build, but the output directory will not be a complete Qt runtime bundle.")
endif()

set_target_properties(FieldMouse PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin/Release
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/bin/Release
)
