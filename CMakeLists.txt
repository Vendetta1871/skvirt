cmake_minimum_required(VERSION 3.20)
project(skvirt LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml DBus)
find_package(LayerShellQt REQUIRED)

qt6_add_executable(skvirt
    src/main.cpp
    src/KeyboardWindow.cpp
    src/KeyboardController.cpp
    src/UinputKeyboard.cpp
    src/InputMonitor.cpp
    src/KWinVk.cpp
    src/FcitxIm.cpp
)

target_include_directories(skvirt PRIVATE src)

qt6_add_resources(skvirt "qml"
    PREFIX "/skvirt"
    FILES
        src/qml/main.qml
        src/qml/SkvirtKeyboard.qml
        src/qml/Key.qml
)

target_link_libraries(skvirt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::DBus
    LayerShellQt::Interface
)
