function(qt_conf_resource output)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources/qtconf.qrc
        ${CMAKE_CURRENT_BINARY_DIR}/qtconf.qrc
        COPYONLY)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources/qt.conf.in
        ${CMAKE_CURRENT_BINARY_DIR}/qt.conf)

    qt5_add_resources(rcc_output ${CMAKE_CURRENT_BINARY_DIR}/qtconf.qrc)
    set(${output} ${rcc_output} PARENT_SCOPE)

endfunction()