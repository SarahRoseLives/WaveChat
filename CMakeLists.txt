cmake_minimum_required(VERSION 3.16)
project(WaveChat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

set(SOURCES
    src/main.cpp
    src/chat/Message.cpp
    src/chat/ChatController.cpp
    src/tnc/KissFrame.cpp
    src/tnc/KissTnc.cpp
    src/tnc/PortEnumerator.cpp
    src/ui/MainWindow.cpp
    src/ui/ChannelList.cpp
    src/ui/MessageWidget.cpp
    src/ui/ChatView.cpp
    src/ui/InputBar.cpp
    src/ui/SettingsDialog.cpp
    src/ui/ActiveUsersPanel.cpp
)

set(HEADERS
    src/chat/Message.h
    src/chat/ChatController.h
    src/tnc/KissFrame.h
    src/tnc/ITnc.h
    src/tnc/KissTnc.h
    src/tnc/PortEnumerator.h
    src/ui/MainWindow.h
    src/ui/ChannelList.h
    src/ui/MessageWidget.h
    src/ui/ChatView.h
    src/ui/InputBar.h
    src/ui/SettingsDialog.h
    src/ui/ActiveUsersPanel.h
)

set(RESOURCES
    resources/resources.qrc
)

qt_add_executable(WaveChat ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(WaveChat PRIVATE src)
target_link_libraries(WaveChat PRIVATE Qt6::Widgets Qt6::SerialPort)

set_target_properties(WaveChat PROPERTIES WIN32_EXECUTABLE TRUE)
