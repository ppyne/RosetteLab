#pragma once

#include "rosettelab/document/document.hpp"

#include <string>

namespace rosettelab::svg {

inline constexpr const char* metadata_namespace = "https://rosettelab.app/ns/1";
inline constexpr const char* schema_version = "0.1";

[[nodiscard]] std::string serialize_rosettelab_svg(
    const document::Document& document);

[[nodiscard]] std::string serialize_clean_svg(
    const document::Document& document);

} // namespace rosettelab::svg
