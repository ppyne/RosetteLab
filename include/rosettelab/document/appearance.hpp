#pragma once

#include <cstddef>
#include <vector>

namespace rosettelab::document {

struct RgbaColor {
    double red{0.0};
    double green{0.0};
    double blue{0.0};
    double alpha{1.0};

    friend constexpr bool operator==(const RgbaColor&, const RgbaColor&) = default;
};

enum class FillRule {
    NonZero,
    EvenOdd,
};

enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity,
};

enum class PaletteScope {
    Subpaths,
    Copies,
};

enum class PaletteTarget {
    Fill,
    Stroke,
    FillAndStroke,
};

struct CyclicPalette {
    bool enabled{false};
    PaletteScope scope{PaletteScope::Copies};
    PaletteTarget target{PaletteTarget::Fill};
    std::vector<RgbaColor> colors;
    int offset{0};

    friend bool operator==(const CyclicPalette&, const CyclicPalette&) = default;
};

struct LayerAppearance {
    bool stroke_enabled{true};
    RgbaColor stroke{0.0, 0.0, 0.0, 1.0};
    double stroke_width{0.6};
    bool fill_enabled{false};
    RgbaColor fill{1.0, 1.0, 1.0, 1.0};
    FillRule fill_rule{FillRule::EvenOdd};
    double opacity{1.0};
    BlendMode blend_mode{BlendMode::Normal};
    CyclicPalette cyclic_palette{};

    friend constexpr bool operator==(const LayerAppearance&, const LayerAppearance&) = default;
};

[[nodiscard]] inline const RgbaColor* cyclic_palette_color(
    const LayerAppearance& appearance, const std::size_t index)
{
    const auto& palette = appearance.cyclic_palette;
    if (!palette.enabled || palette.colors.empty()) return nullptr;
    const auto count = static_cast<long long>(palette.colors.size());
    const auto shifted = static_cast<long long>(index) + palette.offset;
    const auto wrapped = (shifted % count + count) % count;
    return &palette.colors[static_cast<std::size_t>(wrapped)];
}

[[nodiscard]] inline LayerAppearance appearance_for_palette_index(
    const LayerAppearance& source, const std::size_t index)
{
    LayerAppearance result;
    result.stroke_enabled = source.stroke_enabled;
    result.stroke = source.stroke;
    result.stroke_width = source.stroke_width;
    result.fill_enabled = source.fill_enabled;
    result.fill = source.fill;
    result.fill_rule = source.fill_rule;
    result.opacity = source.opacity;
    result.blend_mode = source.blend_mode;
    if (const auto* color = cyclic_palette_color(source, index)) {
        if (source.cyclic_palette.target != PaletteTarget::Stroke) result.fill = *color;
        if (source.cyclic_palette.target != PaletteTarget::Fill) result.stroke = *color;
    }
    return result;
}

} // namespace rosettelab::document
