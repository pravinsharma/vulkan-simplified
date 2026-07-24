find_package(Catch2 CONFIG QUIET)

if(Catch2_FOUND)
    enable_testing()

    add_executable(vks_v01_core ${CMAKE_CURRENT_SOURCE_DIR}/vks_v01_core.cpp)
    target_link_libraries(vks_v01_core PRIVATE vulkan_simplified Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(vks_v01_core)
endif()
