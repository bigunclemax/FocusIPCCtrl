cmake_minimum_required(VERSION 3.10)

project(FocusIPCCtrl)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#add_compile_definitions(DEBUG_BUFFERS DEBUG_BUFFERS_HEX) #uncomment this if need debug CAN frames
add_subdirectory(stn1170-tools)
add_subdirectory(staging)

include_directories(${CMAKE_BINARY_DIR})
add_executable(${CMAKE_PROJECT_NAME} main.cpp formcar.cpp dialog.cpp controllers/els27/ControllerElx.cpp)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ./stn1170-tools/serial/include)
target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Widgets stnlib serial)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_compile_options(-Werror=vla)
	target_link_libraries(${CMAKE_PROJECT_NAME} pthread)
endif()
qt5_use_modules(${CMAKE_PROJECT_NAME} Core)