cmake_minimum_required(VERSION 3.16)
project(atelier_sim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LVGL REQUIRED lvgl)

include_directories(
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/../include
    ${SDL2_INCLUDE_DIRS}
    ${LVGL_INCLUDE_DIRS}
)

file(GLOB LVGL_SOURCES ${LVGL_LIBRARIES}/*.c 2>/dev/null)

add_executable(atelier_sim
    main_sim.cpp
    ../src/ui.cpp
    ${LVGL_SOURCES}
)

target_link_libraries(atelier_sim ${SDL2_LIBRARIES} ${LVGL_LIBRARIES} m)
