cmake_minimum_required(VERSION 3.25)

project(MControlCenterHelper LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
  Core
  DBus
)

add_executable(mcontrolcenter-helper
  helper.h
  helper.cpp
  readwrite.h
  readwrite.cpp
  wmi.cpp
  auth.h
  auth.cpp
  msi-ec.h
  msi-ec.cpp
)
target_link_libraries(mcontrolcenter-helper PRIVATE
  Qt6::Core
  Qt6::DBus
)

target_compile_options(mcontrolcenter-helper PRIVATE -fstack-protector-strong -D_FORTIFY_SOURCE=3 -fstack-clash-protection)
target_link_options(mcontrolcenter-helper PRIVATE -Wl,-z,relro,-z,now)

install(TARGETS mcontrolcenter-helper RUNTIME DESTINATION libexec)
