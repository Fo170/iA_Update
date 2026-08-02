cmake_minimum_required(VERSION 3.16)
project(iA_Update VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Concurrent)

file(COPY ico/app.ico DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ico)
file(COPY lang/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lang)
file(COPY apps.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(iA_Update
    main.cpp
    AppConfig.hpp
    MainWindow.cpp
    AppItem.cpp
    AppDetector.cpp
    VersionChecker.cpp
    Downloader.cpp
    CommandBuilder.cpp
    Installer.cpp
    LangueManager.cpp
    resources.qrc
)

if(WIN32)
    set_target_properties(iA_Update PROPERTIES WIN32_EXECUTABLE TRUE)
    # Find windres (MinGW resource compiler) - try path, then common Qt locations
    find_program(WINDRES windres)
    if(NOT WINDRES)
        message(FATAL_ERROR "windres not found. Install MinGW-w64 or set WINDRES cmake variable.")
    endif()
    set(RC_OUT "${CMAKE_CURRENT_BINARY_DIR}/app_icon.o")
    if(NOT EXISTS "${RC_OUT}")
        execute_process(
            COMMAND ${WINDRES} -i "${CMAKE_CURRENT_SOURCE_DIR}/app.rc" -o "${RC_OUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
    target_sources(iA_Update PRIVATE ${RC_OUT})
endif()

target_link_libraries(iA_Update PRIVATE Qt6::Widgets Qt6::Network Qt6::Concurrent)
target_include_directories(iA_Update PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
