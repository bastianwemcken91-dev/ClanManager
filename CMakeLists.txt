cmake_minimum_required(VERSION 3.16)
project(ClanManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets Test REQUIRED)

if(APPLE)
  # Remove hard-coded AGL framework from several Qt imported target link flags when not available
  # (Qt sometimes adds deprecated AGL; on arm64 macs it may be missing)
  set(_qt_targets Qt6::Widgets Qt6::Gui Qt6::Core)
  foreach(_t ${_qt_targets})
    if(TARGET ${_t})
      get_target_property(_libs ${_t} INTERFACE_LINK_LIBRARIES)
      if(_libs)
        # Rebuild list without any entry that mentions AGL or AGL.framework
        set(_filtered "")
        foreach(_entry ${_libs})
          string(FIND "${_entry}" "AGL" _hasAGL)
          if(_hasAGL EQUAL -1)
            list(APPEND _filtered "${_entry}")
          endif()
        endforeach()
        set_target_properties(${_t} PROPERTIES INTERFACE_LINK_LIBRARIES "${_filtered}")
      endif()
    endif()
  endforeach()
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES
  ${SRC_DIR}/*.cpp
)

# Remove debug/test/alternative main files
list(FILTER SOURCES EXCLUDE REGEX "main_debug\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "main_fixed\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "main_minimal_test\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "main_test\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "main\\.cpp\\.FIXED$")

# Ensure headers with Q_OBJECT are seen by automoc
list(APPEND SOURCES ${INC_DIR}/MainWindow.h)
list(APPEND SOURCES ${INC_DIR}/LineupDialog.h)

add_executable(ClanManager ${SOURCES})

target_include_directories(ClanManager PRIVATE ${INC_DIR})
target_link_libraries(ClanManager PRIVATE Qt6::Widgets)
enable_testing()

file(GLOB TEST_SOURCES tests/test_*.cpp)
if(TEST_SOURCES)
  set(SOURCES_NO_MAIN "${SOURCES}")
  list(REMOVE_ITEM SOURCES_NO_MAIN ${SRC_DIR}/main.cpp)
  foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC} ${SOURCES_NO_MAIN})
    target_include_directories(${TEST_NAME} PRIVATE ${INC_DIR})
    target_link_libraries(${TEST_NAME} PRIVATE Qt6::Widgets Qt6::Test)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  endforeach()
endif()

# Platform-specific settings
if(WIN32)
  # Windows: Create GUI application (no console window)
  set_target_properties(ClanManager PROPERTIES
    WIN32_EXECUTABLE TRUE
  )
  # Set application icon for Windows
  # target_sources(ClanManager PRIVATE ${CMAKE_SOURCE_DIR}/resources/app.rc)
elseif(APPLE)
  set_target_properties(ClanManager PROPERTIES MACOSX_BUNDLE TRUE)
endif()

# Optional AddressSanitizer for debug runs
option(ENABLE_ASAN "Enable AddressSanitizer (Debug/Local)" OFF)
if(ENABLE_ASAN)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    message(STATUS "AddressSanitizer enabled")
    target_compile_options(ClanManager PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(ClanManager PRIVATE -fsanitize=address)
  endif()
endif()

if(APPLE)
  # Ensure the built bundle can resolve Qt frameworks installed by Homebrew
  # so macdeployqt can locate and embed them. Set an install RPATH pointing
  # to the Homebrew Qt lib folder used on Apple Silicon (/opt/homebrew).
  set_target_properties(ClanManager PROPERTIES
    INSTALL_RPATH "/opt/homebrew/opt/qt/lib"
    BUILD_WITH_INSTALL_RPATH TRUE)
  # Also set CMAKE_INSTALL_RPATH for install step compatibility
  set(CMAKE_INSTALL_RPATH "/opt/homebrew/opt/qt/lib")
endif()
