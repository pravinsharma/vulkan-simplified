#pragma once

#include <stdexcept>
#include <string>

namespace vks {

class FatalError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}
