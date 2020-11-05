cmake_minimum_required(VERSION 3.10)

project(FocusIPCCtrl)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_compile_options(-Werror=vla)
include_directories(${CMAKE_BINARY_DIR})
add_executable(${CMAKE_PROJECT_NAME} main.cpp formcar.cpp dialog.cpp controllers/els27/QControllerEls27.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Widgets)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries(${CMAKE_PROJECT_NAME} pthread)
endif()
qt5_use_modules(${CMAKE_PROJECT_NAME} Core SerialPort)