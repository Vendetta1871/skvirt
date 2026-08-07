# System Settings module (KCM) for skvirt.
# extra-cmake-modules is not installed on this machine, so provide the two
# variables kcoreaddons_add_plugin normally gets from KDECMakeSettings/KDEInstallDirs.
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
if(NOT KDE_INSTALL_PLUGINDIR)
    # Relative, so `cmake --install --prefix` can redirect it.
    set(KDE_INSTALL_PLUGINDIR lib/qt6/plugins)
endif()

# KCM must be a shared module; without KDECMakeSettings (ECM is not installed)
# BUILD_SHARED_LIBS is unset and kcoreaddons_add_plugin would make it static.
set(BUILD_SHARED_LIBS ON)

kcoreaddons_add_plugin(kcm_skvirt
    SOURCES
        kcm_skvirt.cpp
        kcm_skvirt.h
        kcm_skvirt.json
    INSTALL_NAMESPACE "plasma/kcms/systemsettings"
)

# Plasma finds the KCM by file name and derives the plugin id from it; the
# default "lib" prefix would make it "libkcm_skvirt" and break the qrc path.
set_target_properties(kcm_skvirt PROPERTIES PREFIX "")

# Same KConfigXT settings as the daemon, generated for this target.
kconfig_add_kcfg_files(kcm_skvirt GENERATE_MOC ${CMAKE_SOURCE_DIR}/src/skvirt.kcfgc)

# KQuickManagedConfigModule loads the main UI from qrc:/kcm/<pluginId>/main.qml.
set_source_files_properties(ui/main.qml PROPERTIES QT_RESOURCE_ALIAS "main.qml")
qt6_add_resources(kcm_skvirt "ui"
    PREFIX "/kcm/kcm_skvirt"
    FILES
        ui/main.qml
)

target_include_directories(kcm_skvirt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(kcm_skvirt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtilsCore
    KF6::KCMUtilsQuick
)
