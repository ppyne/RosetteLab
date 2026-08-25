#pragma once

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

struct LayerAppearance {
    RgbaColor stroke{0.0, 0.0, 0.0, 1.0};
    double stroke_width{0.6};
    bool fill_enabled{false};
    RgbaColor fill{1.0, 1.0, 1.0, 1.0};
    FillRule fill_rule{FillRule::NonZero};
    double opacity{1.0};
    BlendMode blend_mode{BlendMode::Normal};

    friend constexpr bool operator==(const LayerAppearance&, const LayerAppearance&) = default;
};

} // namespace rosettelab::document

