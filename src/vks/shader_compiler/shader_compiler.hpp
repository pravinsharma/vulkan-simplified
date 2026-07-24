#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace vks {

std::vector<uint32_t> compileShader(const std::string& path);

}
