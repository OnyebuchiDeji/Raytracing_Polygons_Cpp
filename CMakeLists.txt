cmake_minimum_required(VERSION 3.25) # 3.25, not 3.24 so that FetchContent_Declare's SYSTEM keyword works

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not allowed. Please use: cmake -S . -B build")
endif()

project(Raytracing_Polygons_Cpp LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)


# 1. FETCH & CONFIGURE GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG "3.4" SYSTEM)

# 2. FETCH & CONFIGURE GLAD (v2)
FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG "v2.0.6" SOURCE_SUBDIR cmake SYSTEM)

# 3. FETCH & CONFIGURE GLM (Header-only library)
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG "1.0.1" SYSTEM)

FetchContent_MakeAvailable(glfw glad glm)
glad_add_library(glad_gl_core_46 STATIC REPRODUCIBLE LOADER API gl:core=4.6)

# 4. FETCH & MANUALLY TARGET IMGUI
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC)
set_target_properties(imgui PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${imgui_SOURCE_DIR}")
target_sources(imgui PRIVATE
	${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imgui_draw.cpp
	${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw glad_gl_core_46)

# 5. DEFINE USER EXECUTABLE
add_executable(${PROJECT_NAME} 
	src/main.cpp
	src/program.cpp
	src/renderer.cpp
	src/shader.cpp
	src/model.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
    vendor
)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui glm::glm)