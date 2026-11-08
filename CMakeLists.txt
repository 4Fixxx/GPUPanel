cmake_minimum_required(VERSION 3.16)
project(gpu-panel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(gpu-panel main.cpp)
target_link_libraries(gpu-panel PRIVATE Qt6::Widgets)

# --- desktop app install: terminal command "gpupanel", launcher entry, icon ---
include(GNUInstallDirs)
set_target_properties(gpu-panel PROPERTIES OUTPUT_NAME gpupanel)
install(TARGETS gpu-panel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES gpu-panel.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES logo.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME gpu-panel.png)
