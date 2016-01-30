function(install_qt_lib)
    foreach(module ${ARGV})
        set(target_name Qt5::${module})
        set(module_name Qt5${module})

        # Get full path to some known Qt library (i.e. /path/to/lib/libQt5Widgets.so.5.5.1)
        get_target_property(target_path Qt5::Core LOCATION)

        # Get folder path of library (i.e. /path/to/lib)
        get_filename_component(qt_lib_dir ${target_path} DIRECTORY)

        if (SYSTEM_LINUX)
            # Not very good solution, but we simply take all files with *Qt5<module>* in names (including symlinks)
            file(GLOB module_libs ${qt_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}*)

            # Install to "lib" folder
            install(FILES ${module_libs}
                DESTINATION ${lib_dir})
        endif()

        if(SYSTEM_MACOSX)
            set(module_name Qt${module})

            # On Mac OS we are still located in .framework folder,
            # and we need to go one level up
            get_filename_component(qt_lib_dir ${qt_lib_dir} DIRECTORY)

            install(
                DIRECTORY ${qt_lib_dir}/${module_name}.framework
                DESTINATION ${lib_dir}
                PATTERN "*_debug" EXCLUDE      # Exclude debug libraries
                PATTERN "Headers" EXCLUDE)     # Exclude Headers folders
        endif()

        if (SYSTEM_WINDOWS)
            # Install single DLL to lib directory
            install(FILES ${qt_lib_dir}/${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}
                    DESTINATION ${lib_dir})
        endif()

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

# Installs Qt5 plugins. Uses value of global variable "qt_plugins_dir" as
# absolute path to where install plugins.
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
            DESTINATION ${qt_plugins_dir}/${plugin_dir_name})
    endforeach()
endfunction()

