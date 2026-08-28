#pragma once

#include "rosettelab/document/document.hpp"

#include <string>

namespace rosettelab::pdf {

enum class ColorModel {
    Rgb,
    Cmyk,
};

struct ExportOptions {
    ColorModel color_model{ColorModel::Rgb};
};

// Produces a PDF 1.7 document whose curve geometry and layer compositing remain
// vector-based. Each RosetteLab layer is emitted as an isolated transparency
// group so its opacity and blend mode apply to the completed copy composition.
[[nodiscard]] std::string serialize_vector_pdf(
    const document::Document& document,
    const ExportOptions& options = {});

} // namespace rosettelab::pdf
