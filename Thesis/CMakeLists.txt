file(GLOB_RECURSE THESIS_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE THESIS_HEADERS CONFIGURE_DEPENDS include/*.h include/*.hpp)

add_executable(Thesis ${THESIS_SOURCES} ${THESIS_HEADERS} "include/shader.h" "src/shader.cpp")

target_include_directories(Thesis PRIVATE include)

target_link_libraries(Thesis PRIVATE
    glfw
    glad_gl_43_core
    glm::glm
    imgui_lib
    happly_lib
)

set_target_properties(Thesis PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Copy runtime assets to the build output directory after each build
add_custom_command(TARGET Thesis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:Thesis>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:Thesis>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/media
        $<TARGET_FILE_DIR:Thesis>/media
    COMMENT "Copying assets to build directory"
)
