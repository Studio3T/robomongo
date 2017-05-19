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

if(SYSTEM_LINUX)
    set(bin_dir             bin)
    set(lib_dir             lib)
    set(resources_dir       share)
    set(license_dir         .)

    set(qt_plugins_dir      ${lib_dir})
    set(qt_conf_dir         ${bin_dir})
    set(qt_conf_plugins     "../lib")
elseif(SYSTEM_MACOSX)
    set(bundle_name         "Robo 3T.app")
    set(contents_path       ${bundle_name}/Contents)

    set(bin_dir             ${contents_path}/MacOS)
    set(lib_dir             ${contents_path}/Frameworks)
    set(resources_dir       ${contents_path}/Resources)
    set(license_dir         ${resources_dir})

    set(qt_plugins_dir      ${contents_path}/PlugIns/Qt)
    set(qt_conf_dir         ${resources_dir})
    set(qt_conf_plugins     "PlugIns/Qt")
elseif(SYSTEM_WINDOWS)
    set(bin_dir             .)
    set(lib_dir             .)
    set(resources_dir       .)
    set(license_dir         .)

    set(qt_plugins_dir      ${lib_dir})
    set(qt_conf_dir         ${bin_dir})
    set(qt_conf_plugins     .)
endif()

# Generate qt.conf file
configure_file(
    "${CMAKE_SOURCE_DIR}/install/qt.conf.in"
    "${CMAKE_BINARY_DIR}/qt.conf")

# Install qt.conf file
install(
    FILES       "${CMAKE_BINARY_DIR}/qt.conf"
    DESTINATION "${qt_conf_dir}")

# Install OpenSSL dynamic lib files
if(SYSTEM_WINDOWS)
    install(
        FILES 
        "${OpenSSL_DIR}/out32dll/ssleay32.dll"
        "${OpenSSL_DIR}/out32dll/libeay32.dll"
        DESTINATION ${bin_dir})
elseif(SYSTEM_MACOSX)
    install(
        FILES 
        "${OpenSSL_DIR}/libssl.1.0.0.dylib"
        "${OpenSSL_DIR}/libcrypto.1.0.0.dylib"
        DESTINATION ${lib_dir}/lib)
elseif(SYSTEM_LINUX)
    install(
        FILES 
        "${OpenSSL_DIR}/libssl.so"
        "${OpenSSL_DIR}/libssl.so.1.0.0"        
        "${OpenSSL_DIR}/libcrypto.so"        
        "${OpenSSL_DIR}/libcrypto.so.1.0.0"
        DESTINATION ${lib_dir})         
endif()

# Install binary
install(
    TARGETS robomongo
    RUNTIME DESTINATION ${bin_dir}
    BUNDLE DESTINATION .)

# Install license, copyright and changelogs files
install(
    FILES
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/COPYRIGHT
        ${CMAKE_SOURCE_DIR}/CHANGELOG
        ${CMAKE_SOURCE_DIR}/DESCRIPTION
    DESTINATION ${license_dir})

# Install common dependencies
install_qt_lib(Core Gui Widgets PrintSupport Network Xml)
install_qt_plugins(QGifPlugin QICOPlugin)
install_icu_libs()

if(SYSTEM_LINUX)
    install_qt_lib(XcbQpa DBus)
    install_qt_plugins(
        QXcbIntegrationPlugin)
        
    # Install newer versions of libstdc++ to support C++11
    install(
        FILES
            "/usr/lib/x86_64-linux-gnu/libstdc++.so.6"
            "/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.22"
        DESTINATION ${lib_dir})
elseif(SYSTEM_MACOSX)
    install_qt_lib(MacExtras DBus)
    install_qt_plugins(
        QCocoaIntegrationPlugin
        QMinimalIntegrationPlugin
        QOffscreenIntegrationPlugin)

    # Install icon
    install(
        FILES       "${CMAKE_SOURCE_DIR}/install/macosx/robomongo.icns"
        DESTINATION "${resources_dir}")
elseif(SYSTEM_WINDOWS)
    install_qt_plugins(
        QWindowsIntegrationPlugin
        QMinimalIntegrationPlugin
        QOffscreenIntegrationPlugin)

    # Install runtime libraries:
    # msvcp120.dll
    # msvcr120.dll
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    include(InstallRequiredSystemLibraries)
endif()
