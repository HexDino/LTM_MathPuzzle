cmake_minimum_required(VERSION 3.5)

project(MathPuzzleClient VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

# Source files
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    networkmanager.cpp
    networkmanager.h
    gamestatemachine.cpp
    gamestatemachine.h
    loginscreen.cpp
    loginscreen.h
    lobbyscreen.cpp
    lobbyscreen.h
    roomscreen.cpp
    roomscreen.h
    gamescreen.cpp
    gamescreen.h
    resultscreen.cpp
    resultscreen.h
)

# Create executable
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MathPuzzleClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(MathPuzzleClient
        ${PROJECT_SOURCES}
    )
endif()

# Link Qt libraries
target_link_libraries(MathPuzzleClient PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# Platform-specific settings
if(WIN32)
    set_target_properties(MathPuzzleClient PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_finalize_executable(MathPuzzleClient)
endif()

# Installation
install(TARGETS MathPuzzleClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


