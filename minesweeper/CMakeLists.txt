cmake_minimum_required(VERSION 3.19)
project(minesweeper LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(minesweeper
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    minesweeper_engine.h
    minesweeper_engine.cpp
)

target_link_libraries(minesweeper
    PRIVATE
        Qt::Core
        Qt::Widgets
)

target_compile_definitions(minesweeper PRIVATE APP_ICON_PATH="${CMAKE_CURRENT_SOURCE_DIR}/icons/minesweeperICON.ico")

install(TARGETS minesweeper
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET minesweeper
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
