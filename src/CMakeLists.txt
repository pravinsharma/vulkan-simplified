target_sources(vks PRIVATE
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
    vks/render_graph.cpp
    vks/post_process.cpp
)

add_subdirectory(vks/backend)
add_subdirectory(vks/renderer)
add_subdirectory(vks/shader_compiler)
add_subdirectory(vks/assets)
