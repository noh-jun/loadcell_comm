cmake_minimum_required(VERSION 3.16)
project(loadcell_comm_485 LANGUAGES CXX)

# ---- C++ 표준 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 경고 옵션(원하면 조정) ----
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---- BUILD_SHARED_LIBS 토글 ----
# -DBUILD_SHARED_LIBS=ON  -> shared(.so)
# -DBUILD_SHARED_LIBS=OFF -> static(.a)
if (NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
endif()

# =========================
# Single Library (Final)
# =========================
add_library(loadcell_485_comm
  loadcell_comm/loadcell_485.cpp
  serial_comm/SerialPort.cpp
  ring_buffer/ByteRingBuffer.cpp
)

# (기존에 쓰던 링커 옵션이 꼭 필요하면 유지, 필요 없으면 삭제 가능)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(loadcell_485_comm
    PRIVATE -Wl,--no-as-needed
  )
endif()

# include 경로 (빌드/설치 분리)
target_include_directories(loadcell_485_comm
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/loadcell_comm>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/serial_comm>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ring_buffer>
    $<INSTALL_INTERFACE:include/loadcell_comm/rs485/loadcell_comm>
    $<INSTALL_INTERFACE:include/loadcell_comm/rs485/serial_comm>
    $<INSTALL_INTERFACE:include/loadcell_comm/rs485/ring_buffer>
)

set_target_properties(loadcell_485_comm PROPERTIES
  VERSION 0.1.0
  SOVERSION 0
)

# =========================
# Install
# =========================
install(TARGETS loadcell_485_comm
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# 헤더 설치 (필요 파일만 명시적으로 설치)
install(FILES
  serial_comm/SerialConfig.h
  loadcell_comm/loadcell_485.hpp
  DESTINATION include/loadcell_comm/rs485/loadcell_comm
)

# 아래가 필요하면 주석 해제 (디렉터리 단위 설치)
# install(DIRECTORY
#   loadcell_comm/
#   DESTINATION include/loadcell_comm/rs485/loadcell_comm
#   FILES_MATCHING
#   PATTERN "*.h"
#   PATTERN "*.hpp"
# )
