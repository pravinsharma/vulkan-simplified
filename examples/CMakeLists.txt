add_executable(pong pong.cpp)
target_link_libraries(pong PRIVATE vks)

add_custom_command(TARGET pong POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../shaders"
        "$<TARGET_FILE_DIR:pong>/shaders"
)

add_executable(turtle turtle.cpp)
target_link_libraries(turtle PRIVATE vks)

add_custom_command(TARGET turtle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../shaders"
        "$<TARGET_FILE_DIR:turtle>/shaders"
)
