#要求CMakeバージョン
cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME NIDAQ)

#プロジェクト名
project(${PROJECT_NAME})

#ディレクトリ設定
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

message("SRC_DIR : ${SRC_DIR}")
message("INC_DIR : ${INC_DIR}")
message("LIB_DIR : ${LIB_DIR}")

include_directories(
        ${INC_DIR}
)


add_executable(
        ${PROJECT_NAME}
        ${SRC_DIR}/main.c
)

target_link_libraries(
        ${PROJECT_NAME}
         ${LIB_DIR}/NIDAQmx.lib
)
