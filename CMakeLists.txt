cmake_minimum_required(VERSION 3.12...3.28)

project(
    vulkan-tutorial
    VERSION 0.1
    DESCRIPTION "Vulkan tutorial"
    LANGUAGES CXX
)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)



set(SOURCES
    src/main.cpp
    src/helloTriangleApplication.cpp
)

add_executable(vulkan-tutorial ${SOURCES})

target_compile_features(vulkan-tutorial PUBLIC cxx_std_23)

target_include_directories(vulkan-tutorial
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(vulkan-tutorial
    PRIVATE glfw
    PRIVATE glm::glm
    PRIVATE Vulkan::Vulkan
)

# Get the validator
set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "shaders/*.frag"
    "shaders/*.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(vulkan-tutorial shaders)

add_custom_command(TARGET vulkan-tutorial POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan-tutorial>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:vulkan-tutorial>/shaders"
        )