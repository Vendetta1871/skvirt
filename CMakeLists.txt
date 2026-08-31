cmake_minimum_required(VERSION 3.20)
project(skvirt LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml DBus)
find_package(LayerShellQt REQUIRED)
find_package(KF6Config REQUIRED)
find_package(KF6CoreAddons REQUIRED)
find_package(KF6I18n REQUIRED)
find_package(KF6KCMUtils REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HUNSPELL REQUIRED IMPORTED_TARGET hunspell)
find_package(LibIMECore REQUIRED)
find_package(LibIMEPinyin REQUIRED)

qt6_add_executable(skvirt
    src/main.cpp
    src/KeyboardWindow.cpp
    src/KeyboardController.cpp
    src/UinputKeyboard.cpp
    src/InputMonitor.cpp
    src/KWinVk.cpp
    src/FcitxIm.cpp
    src/LayoutGenerator.cpp
    src/SettingsBridge.cpp
    src/SuggestionEngine.cpp
)

target_include_directories(skvirt PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})

# libime/fcitx5 headers need C++20 (std::span, deduced function templates);
# the rest of the codebase is C++17-clean and compiles fine as C++20.
set_target_properties(skvirt PROPERTIES CXX_STANDARD 20)

kconfig_add_kcfg_files(skvirt GENERATE_MOC src/skvirt.kcfgc)

qt6_add_resources(skvirt "qml"
    PREFIX "/skvirt"
    FILES
        src/qml/main.qml
        src/qml/SkvirtKeyboard.qml
        src/qml/Key.qml
        src/qml/MacTheme.qml
)

target_link_libraries(skvirt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::DBus
    LayerShellQt::Interface
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::I18n
    PkgConfig::HUNSPELL
    LibIME::Core
    LibIME::Pinyin
)

add_subdirectory(kcm)
