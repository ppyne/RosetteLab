#pragma once

#include "rosettelab/document/appearance.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace rosettelab::palette {

[[nodiscard]] std::string serialize_gpl(
    const std::vector<document::RgbaColor>& colors,
    std::string_view name = "RosetteLab palette");

[[nodiscard]] std::vector<document::RgbaColor> parse_gpl(std::string_view data);

} // namespace rosettelab::palette
