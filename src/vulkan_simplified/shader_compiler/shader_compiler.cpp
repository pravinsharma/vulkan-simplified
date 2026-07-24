#include "vulkan_simplified/shader_compiler/shader_compiler.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <shaderc/shaderc.h>

namespace vks {

namespace {

struct CachedShader {
    std::filesystem::file_time_type mtime;
    std::vector<uint32_t> spirv;
};

static std::unordered_map<std::string, CachedShader> g_cache;
static std::once_flag g_shaderc_init;
static shaderc_compiler_t g_compiler = nullptr;

void ensure_shaderc_initialized() {
    std::call_once(g_shaderc_init, []() {
        g_compiler = shaderc_compiler_initialize();
        if (!g_compiler) throw std::runtime_error("shaderc_compiler_initialize failed");
    });
}

shaderc_shader_kind detect_stage(const std::string& path) {
    auto lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    auto pos = lower.rfind('.');
    if (pos == std::string::npos) return shaderc_glsl_fragment_shader;
    auto ext = lower.substr(pos + 1);
    if (ext == "vert" || ext == "vs" || ext == "vertex") return shaderc_glsl_vertex_shader;
    if (ext == "frag" || ext == "fs" || ext == "fragment") return shaderc_glsl_fragment_shader;
    if (ext == "comp" || ext == "cs") return shaderc_glsl_compute_shader;
    if (ext == "geom") return shaderc_glsl_geometry_shader;
    if (ext == "tesc") return shaderc_glsl_tess_control_shader;
    if (ext == "tese") return shaderc_glsl_tess_evaluation_shader;
    return shaderc_glsl_fragment_shader;
}

std::vector<uint32_t> compile_glsl(const std::string& path, const std::string& source) {
    auto kind = detect_stage(path);

    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_source_language(options, shaderc_source_language_glsl);
    shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    shaderc_compile_options_set_target_spirv(options, shaderc_spirv_version_1_5);
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_zero);

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        g_compiler, source.data(), source.size(), kind, path.c_str(), "main", options);

    if (!result) {
        throw std::runtime_error("shaderc_compile_into_spv failed for: " + path);
    }

    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        std::string msg = shaderc_result_get_error_message(result);
        shaderc_result_release(result);
        shaderc_compile_options_release(options);
        throw std::runtime_error("Shader compilation failed: " + path + " - " + msg);
    }

    auto size = shaderc_result_get_length(result);
    std::vector<uint32_t> spirv(size / 4);
    memcpy(spirv.data(), shaderc_result_get_bytes(result), size);

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    return spirv;
}

}

std::vector<uint32_t> compileShader(const std::string& path) {
    ensure_shaderc_initialized();

    std::error_code ec;
    auto file_time = std::filesystem::last_write_time(path, ec);
    if (ec) throw std::runtime_error("Failed to stat shader: " + path);

    {
        auto it = g_cache.find(path);
        if (it != g_cache.end() && it->second.mtime == file_time) {
            return it->second.spirv;
        }
    }

    std::ifstream file(path);
    if (!file) throw std::runtime_error("Failed to open shader: " + path);
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto spirv = compile_glsl(path, source);

    CachedShader cached;
    cached.mtime = file_time;
    cached.spirv = std::move(spirv);
    g_cache[path] = std::move(cached);

    return g_cache[path].spirv;
}

}
