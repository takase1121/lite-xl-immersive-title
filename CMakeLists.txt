cmake_minimum_required(VERSION 3.7)

project(monitor
		LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(MONITOR_SOURCE_FILES "monitor.manifest" "monitor.c")
set(MONITOR_LINK_LIBS "dwmapi")

set(MONITOR_ENABLE_ACCENT_REPORT "ON" CACHE STRING "Enables reading accent colors")

if(MONITOR_ENABLE_ACCENT_REPORT)
	add_definitions(-DENABLE_ACCENT_REPORT=1)
	set(MONITOR_SOURCE_FILES "${MONITOR_SOURCE_FILES}" "winrt_ui_settings.cc")
	set(MONITOR_LINK_LIBS "${MONITOR_LINK_LIBS}" "runtimeobject")
endif()

add_executable(monitor "${MONITOR_SOURCE_FILES}")
target_link_libraries(monitor PRIVATE "${MONITOR_LINK_LIBS}")

install(TARGETS monitor DESTINATION .)
install(FILES init.lua DESTINATION .)