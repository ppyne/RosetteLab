#pragma once

#include "rosettelab/document/document.hpp"

#include <string>

namespace rosettelab::svg {

inline constexpr const char* metadata_namespace = "https://rosettelab.app/ns/1";
inline constexpr const char* schema_version = "0.1";

struct SvgDocumentSettings {
    double width{210.0};
    double height{210.0};
    std::string unit{"mm"};
    document::RgbaColor background{1.0, 1.0, 1.0, 1.0};
};

[[nodiscard]] std::string serialize_rosettelab_svg(
    const document::Document& document,
    const SvgDocumentSettings& settings = {});

} // namespace rosettelab::svg

