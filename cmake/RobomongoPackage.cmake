# Notes for maintainers.
#
# 1. Make sure that we use only lowercase letters in file name of outputed
#    package for all platforms and package types.
#

# Get last commit hash
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_hash
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Timestamp (not used for now)
string(TIMESTAMP timestamp "%Y-%m-%d")

# Package name (as it should appear in UI)
set(CPACK_PACKAGE_NAME robo3t)

# Version of the package
set(CPACK_PACKAGE_VERSION_MAJOR     ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR     ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH     ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION           ${PROJECT_VERSION})

# Where to put generated package
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/package)

# Disables the component-based install process for installers
# that support it (NSIS, for instance)
set(CPACK_MONOLITHIC_INSTALL ON)

# Strip debug information on platforms that support it
set(CPACK_STRIP_FILES ON)

# Additional information
set(CPACK_PACKAGE_VENDOR "3T Software Labs Ltd")
set(CPACK_PACKAGE_CONTACT robo3t@3t.io)
set(CPACK_PACKAGE_DESCRIPTION "Shell-centric cross-platform MongoDB management tool.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Robo 3T is a shell-centric cross-platform MongoDB management tool.")

# Use lowercase for system name and package file name
string(TOLOWER ${CMAKE_SYSTEM_NAME} system_name)
string(TOLOWER ${CPACK_PACKAGE_NAME} package_file_name)
string(TOLOWER ${CPACK_PACKAGE_VERSION} package_file_version)

# We use function from TargetArch.cmake module
# Returns string with target architecture value
# Output for common architectures is: i386 or x86_64
target_architecture(target_arch)

# Package file name
set(CPACK_PACKAGE_FILE_NAME ${package_file_name}-${package_file_version}-${system_name}-${target_arch}-${git_hash})

if(SYSTEM_LINUX)
    set(CPACK_GENERATOR TGZ)


elseif(SYSTEM_MACOSX)
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_DMG_DS_STORE ${CMAKE_SOURCE_DIR}/install/macosx/DMG_DS_Store)

elseif(SYSTEM_WINDOWS)
    set(files_dir "${CMAKE_SOURCE_DIR}/install/windows")
    set(exe_name "robo3t.exe")

    set(CPACK_GENERATOR NSIS ZIP)
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/DESCRIPTION")

    # License file will be shown in the installation wizard
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

    # Default installation directory (not full path), just the path after
    # default "c:\Program Files"
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${ROBOMONGO_DISPLAY_FULL_NAME}")

    # A path to the executable inside installed package that contains icon
    # This icon is used in Windows "Add or remove programs" control panel
    set(CPACK_NSIS_INSTALLED_ICON_NAME "\\\\${exe_name}")

    # Create shortcuts on desktop and start-up menu
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "
        CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${ROBOMONGO_DISPLAY_FULL_NAME}.lnk' '$INSTDIR\\\\${exe_name}'
        CreateShortCut '$DESKTOP\\\\${ROBOMONGO_DISPLAY_FULL_NAME}.lnk' '$INSTDIR\\\\${exe_name}'
    ")

    # Cleanup during uninstallation. This cleanup is required because shortcuts
    # will not be deleted automatically.
    # Without line '!insertmacro...' shortcut in start menu will not be removed
    # (i.e. it doesn't work when we referece '$STARTMENU_FOLDER' that is used
    # when creating shortcuts). This solution was found somewhere in internet.
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "
        !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
        Delete '$DESKTOP\\\\${ROBOMONGO_DISPLAY_FULL_NAME}.lnk'
        Delete '$SMPROGRAMS\\\\$MUI_TEMP\\\\${ROBOMONGO_DISPLAY_FULL_NAME}.lnk'
    ")

    # The title displayed at the top of the installer
    set(CPACK_NSIS_PACKAGE_NAME "${ROBOMONGO_DISPLAY_FULL_NAME}")

    # Installer images (for sidebar and topbar)
    set(CPACK_PACKAGE_ICON "${files_dir}\\\\nsis-topbar.bmp")
    set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "!define MUI_WELCOMEFINISHPAGE_BITMAP \\\"${files_dir}\\\\nsis-sidebar.bmp\\\"")

    # URL to a web site providing more information about application
    # This url was noticed at least in "Add or remove programs" control
    # panel.
    set(CPACK_NSIS_URL_INFO_ABOUT "www.robomongo.org")

    # Specify an executable to add an option to run on the finish
    # page of the NSIS installer. Without "..\\\\" it dowsn't work for NSIS 2.5
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "..\\\\${exe_name}")

    # Trying to set for installer and uninstaller, but this is
    # doesn't work and default icons are still used...
    set(CPACK_NSIS_MUI_ICON "${files_dir}\\\\robomongo.ico")
    set(CPACK_NSIS_MUI_UNIICON "${files_dir}\\\\robomongo.ico")
endif()

include(CPack)
