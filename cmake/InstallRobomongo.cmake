# Note for maintainers
# --------------------
#
# Do not use absolute paths in DESTINATION arguments for install() command.
# Because the same install code will be executed again by CPack. And CPack will
# change internally CMAKE_INSTALL_PREFIX to point to some temporary folder
# for package content.
#
#


# Temporary change
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install"
    CACHE STRING "Install path prefix, prepended onto install directories"
    FORCE)

set(install_dir ${CMAKE_INSTALL_PREFIX})
set(bin_dir bin)
set(lib_dir lib)

INSTALL(
    TARGETS robomongo
    DESTINATION ${bin_dir})

INSTALL(
    FILES
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/COPYRIGHT
        ${CMAKE_SOURCE_DIR}/CHANGELOG
    DESTINATION
        .)

INSTALL(
    PROGRAMS
        ${CMAKE_SOURCE_DIR}/install/linux/robomongo.sh
    DESTINATION
        ${bin_dir})


function(install_qt_lib)
    foreach(module ${ARGV})
        set(module_name Qt5${module})
        set(target_name Qt5::${module})

        # Get full path to library (i.e. /path/to/lib/libQt5Widgets.so.5.5.1)
        get_target_property(target_path ${target_name} LOCATION)

        # Resolve symlinks if any
        get_filename_component(real_target_path ${target_path} REALPATH)

        # Get folder path of library (i.e. /path/to/lib)
        get_filename_component(qt_lib_dir ${real_target_path} DIRECTORY)

        # Not very good solution, but we simply take all files with *Qt5<module>* in names (including symlinks)
        file(GLOB module_libs ${qt_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}*)

        # Install to "lib" folder
        install(FILES ${module_libs}
            DESTINATION ${lib_dir})
    endforeach()
endfunction()

function(install_icu_libs)
    # We are trying to get 'lib' folder of Qt installation.
    # For this we take some known target (Qt5::Core in this case)
    # and taking path to this library.

    # Get full path to known library (i.e. /path/to/lib/libQt5Core.so.5.5.1)
    get_target_property(target_path Qt5::Core LOCATION)

    # Get absolute path to 'lib' folder (which is a parent folder of 'known' library)
    get_filename_component(qt_lib_dir ${target_path} DIRECTORY)

    # Not very good solution, but we simply take all files with *icu* in names (including symlinks)
    file(GLOB icu_libs ${qt_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}icu*)

    # Install to "lib" folder
    install(FILES ${icu_libs}
            DESTINATION ${lib_dir})
endfunction()

# Installs Qt5 plugins.
#
# You can find list of plugin names by examing this variable: Qt5<Module>_PLUGINS
# For example, Qt5Gui_PLUGINS.
#
# Debug plugins, running application in this way:
# QT_DEBUG_PLUGINS=1 ./yourapp
#
# Some examples of plugin names:
# QGifPlugin, QGtk2ThemePlugin, QIbusPlatformInputContextPlugin
#
# Sample:
#    install_qt_plugins(QGifPlugin QGtk2ThemePlugin QIbusPlatformInputContextPlugin)
#
function(install_qt_plugins)
    foreach(name ${ARGV})
        set(target_name Qt5::${name})

        # We are trying to get 'plugins' folder of Qt installation.

        # Get full path to plugin library (i.e. /path/to/plugins/imageformats/libqgif.so)
        get_target_property(target_path ${target_name} LOCATION)

        # Get absolute path to parent folder (i.e. /path/to/plugins/imageformats for Qt5::QGifPlugin)
        get_filename_component(plugin_dir ${target_path} DIRECTORY)

        # Get plugin parent directory (i.e. "imageformats" for Qt5::QGifPlugin)
        get_filename_component(plugin_dir_name ${plugin_dir} NAME)

        # Get plugin file name (i.e. "libqgif.so" for Qt5::QGifPlugin)
        get_filename_component(plugin_file_name ${target_path} NAME)

        # Get absolute path to plugins folder (i.e. /path/to/plugins)
        get_filename_component(plugins_dir ${plugin_dir} DIRECTORY)

        # Install to "lib" folder
        install(
            FILES ${target_path}
            DESTINATION ${lib_dir}/${plugin_dir_name})
    endforeach()
endfunction()

install_qt_lib(Core Gui Widgets PrintSupport)
install_qt_plugins(QGifPlugin QICOPlugin QGtk2ThemePlugin QXcbIntegrationPlugin)
install_icu_libs()

