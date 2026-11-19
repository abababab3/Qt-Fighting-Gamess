include(D:/Qtttttt/Qt-fighting-games/build/Desktop_Qt_6_5_3_MinGW_64_bit_Debug/.qt/QtDeploySupport.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/Qt-fighting-games-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_ALL_MODULES_FOUND_VIA_FIND_PACKAGE "ZlibPrivate;EntryPointPrivate;Core;Gui;Widgets")

qt6_deploy_runtime_dependencies(
    EXECUTABLE D:/Qtttttt/Qt-fighting-games/build/Desktop_Qt_6_5_3_MinGW_64_bit_Debug/Qt-fighting-games.exe
    GENERATE_QT_CONF
)
