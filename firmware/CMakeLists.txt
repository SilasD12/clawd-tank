# firmware/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# src/ is the main component (PlatformIO layout; also used by idf.py via EXTRA_COMPONENT_DIRS)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(clawd-tank)
