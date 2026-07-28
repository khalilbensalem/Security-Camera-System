cmake_minimum_required(VERSION 3.22.3)
project(prog)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiodcxx)

add_executable(prog src/main.cpp src/TcpServer.cpp src/Camera.cpp src/GpioButton.cpp)
target_include_directories(prog PRIVATE inc ${OpenCV_INCLUDE_DIRS} ${GPIOD_INCLUDE_DIRS})
target_link_libraries(prog PRIVATE ${OpenCV_LIBS} ${GPIOD_LIBRARIES} pthread)