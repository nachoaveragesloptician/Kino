cmake_minimum_required(VERSION 3.16)
project(Kino)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Network OpenGLWidgets)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)


add_executable(Kino WIN32
    # Core
    src/core/main.cpp
    src/core/mainwindow.cpp
    
    # UI Pages
    src/ui/pages/fileview.cpp
    src/ui/pages/drivespage.cpp
    src/ui/pages/browsepage.cpp
    src/ui/pages/playerpage.cpp
    src/ui/pages/settingspage.cpp
    
    # UI Components
    src/ui/components/sidebar.cpp
    src/ui/components/toast.cpp
    src/ui/components/mediadelegate.cpp
    src/ui/components/mpvwidget.cpp
    
    # Services
    src/services/scanworker.cpp
    src/services/tmdbclient.cpp
    
    # Resources
    resources/resources.qrc
)

target_include_directories(Kino PRIVATE 
    src/core
    src/ui/pages
    src/ui/components
    src/services
    src/utils
)

# Link Libraries
target_link_libraries(Kino PRIVATE 
    Qt6::Widgets 
    Qt6::Network 
    Qt6::OpenGLWidgets 
    PkgConfig::MPV         
)
