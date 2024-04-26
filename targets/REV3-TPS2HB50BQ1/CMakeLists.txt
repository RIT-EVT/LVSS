include(${EVT_CORE_DIR}/cmake/evt-core_build.cmake)

project(REV3-TPS2HB50BQ1)
cmake_minimum_required(VERSION 3.15)

make_exe(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC ${BOARD_LIB_NAME})