#pragma once

#include <vector>
#include "move_only_any.hpp"

namespace dipp::details
{
    using service_info = std::vector<move_only_any>;
}