cmake_minimum_required(VERSION 3.19)
project(Qt-fighting-games LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(Qt-fighting-games
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    fighter.h fighter.cpp
    gameui.h gameui.cpp
    playerlogic.h playerlogic.cpp
)

target_link_libraries(Qt-fighting-games
    PRIVATE
        Qt::Core
        Qt::Widgets
)

qt_add_resources(Qt-fighting-games "app_resources"
    PREFIX "/"
    FILES
        res/p1_ldle.png res/p1_walk.png res/p1_jump.png
        res/p1_attack1.png res/p1_attack2.png res/p1_hurt.png
        res/p2_ldle.png res/p2_walk.png res/p2_jump.png
        res/p2_attack1.png res/p2_attack2.png res/p2_hurt.png
)

install(TARGETS Qt-fighting-games
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Qt-fighting-games
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})