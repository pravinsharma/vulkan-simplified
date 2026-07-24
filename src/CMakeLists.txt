target_sources(vulkan_simplified PRIVATE
    app.cpp
    camera.cpp
    debug.cpp
    frame.cpp
    scene_entity.cpp
    material.cpp
    mesh.cpp
    texture.cpp
    error.cpp
    types.cpp
    vulkan_simplified/render_graph.cpp
    vulkan_simplified/post_process.cpp
)

add_subdirectory(vulkan_simplified/backend)
add_subdirectory(vulkan_simplified/renderer)
add_subdirectory(vulkan_simplified/shader_compiler)
add_subdirectory(vulkan_simplified/assets)
