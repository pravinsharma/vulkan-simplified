cmake_minimum_required(VERSION 3.24)
project(VulkanSimplified VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(SDL3 REQUIRED CONFIG)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)

add_library(vulkan_simplified STATIC)
target_include_directories(vulkan_simplified PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(vulkan_simplified PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(vulkan_simplified PUBLIC SDL3::SDL3 Vulkan::Vulkan glm::glm unofficial::shaderc::shaderc)

enable_testing()

add_subdirectory(src)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/CMakeLists.txt")
    add_subdirectory(docs)
endif()

set_target_properties(vulkan_simplified PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# --- Lint / Format / Docs custom targets (no-ops unless overridden) ---

add_custom_target(vks_lint
    COMMAND ${CMAKE_COMMAND} -E echo "No linter configured"
)

add_custom_target(vks_format
    COMMAND ${CMAKE_COMMAND} -E echo "No formatter configured"
)

add_custom_target(vks_docs
    COMMAND ${CMAKE_COMMAND} -E echo "Documentation is in docs/"
)

# --- Installation ---

include(GNUInstallDirs)

install(TARGETS vulkan_simplified
    EXPORT vulkan-simplified-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/vulkan_simplified DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT vulkan-simplified-targets
    FILE vulkan-simplified-targets.cmake
    NAMESPACE vks::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vulkan-simplified
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vulkan_simplified-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vulkan_simplified-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vulkan-simplified
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/vulkan_simplified-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vulkan_simplified-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/vulkan_simplified-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vulkan-simplified
)
