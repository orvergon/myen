cmake_minimum_required(VERSION 3.17)
project(myen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GLFW_FOLDER lib/glfw)
set(GLFW_INCLUDES lib/glfw/include)
set(GLFW_LIB_FOLDER lib/glfw/build/src)
find_library(GLFW3 glfw3 HINTS ${GLFW_LIB_FOLDER} REQUIRED)

set(IMGUI_FOLDER lib/imgui)
set(STB_FOLDER lib/stb)
set(IMGUI_BACKENDS ${IMGUI_FOLDER}/backends)

include_directories(myen
  include/
  include/common/
  include/renderBackend/
  lib/
  ${IMGUI_FOLDER}
  ${IMGUI_BACKENDS}
  ${RP3D_INCLUDES}
  ${STB_FOLDER}
  ${TOBJ_LOADER_FOLDER}
  ${GLFW_INCLUDES}
  ${ASSIMP_INCLUDES})

add_executable(myen app/main.cpp
src/myen.cpp
src/window.cpp
src/renderBackend/renderBackend.cpp
${IMGUI_FOLDER}/imgui.cpp
${IMGUI_FOLDER}/imgui_draw.cpp
${IMGUI_FOLDER}/imgui_demo.cpp
${IMGUI_FOLDER}/imgui_tables.cpp
${IMGUI_FOLDER}/imgui_widgets.cpp
${IMGUI_BACKENDS}/imgui_impl_glfw.cpp
${IMGUI_BACKENDS}/imgui_impl_vulkan.cpp)

target_link_libraries(myen vulkan X11 dl pthread Xi Xrandr ${GLFW3})
