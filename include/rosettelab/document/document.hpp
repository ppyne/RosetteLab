#pragma once

#include "rosettelab/curves/ellipse.hpp"
#include "rosettelab/curves/droplet_rosette.hpp"
#include "rosettelab/curves/harmonograph.hpp"
#include "rosettelab/curves/lissajous.hpp"
#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/curves/trochoid.hpp"
#include "rosettelab/document/appearance.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace rosettelab::document {

enum class CurveType : std::size_t {
    PolarRose,
    Ellipse,
    Hypotrochoid,
    Epitrochoid,
    Lissajous,
    Harmonograph,
    DropletRosette,
    Count,
};

[[nodiscard]] std::string curve_type_name(CurveType type);

using LayerId = std::uint64_t;
using CurveParameters = std::variant<
    curves::PolarRoseParameters,
    curves::EllipseParameters,
    curves::TrochoidParameters,
    curves::LissajousParameters,
    curves::HarmonographParameters,
    curves::DropletRosetteParameters>;

struct LayerTransform {
    double position_x{0.0};
    double position_y{0.0};
    double scale_x{1.0};
    double scale_y{1.0};
    bool link_scales{true};
    double rotation_degrees{0.0};

    friend constexpr bool operator==(const LayerTransform&, const LayerTransform&) = default;
};

enum class CopyArrangement {
    Superimposed,
    Linear,
    Circular,
};

struct LayerCopies {
    CopyArrangement arrangement{CopyArrangement::Superimposed};
    int count{1};
    double rotation_step_degrees{0.0};
    double scale_step{1.0};
    double offset_x_step{0.0};
    double offset_y_step{0.0};
    double circular_radius{0.0};
    double circular_start_degrees{0.0};
    double circular_angle_step_degrees{0.0};
    bool rotate_with_orbit{true};

    friend constexpr bool operator==(const LayerCopies&, const LayerCopies&) = default;
};

struct CopyPlacement {
    double position_x{};
    double position_y{};
    double rotation_degrees{};
    double scale{1.0};
};

struct CurveLayer {
    LayerId id{};
    std::string name;
    CurveType type{CurveType::PolarRose};
    CurveParameters parameters{curves::PolarRoseParameters{}};
    bool visible{true};
    bool locked{false};
    LayerAppearance appearance{};
    LayerTransform transform{};
    LayerCopies copies{};
    std::string preset_id;
    bool preset_customized{false};

    friend bool operator==(const CurveLayer&, const CurveLayer&) = default;
};

[[nodiscard]] inline CopyPlacement copy_placement(
    const CurveLayer& layer, const int copy_index)
{
    CopyPlacement placement{
        layer.transform.position_x,
        layer.transform.position_y,
        layer.transform.rotation_degrees +
            copy_index * layer.copies.rotation_step_degrees,
        std::pow(layer.copies.scale_step, copy_index),
    };
    if (layer.copies.arrangement == CopyArrangement::Linear) {
        placement.position_x += copy_index * layer.copies.offset_x_step;
        placement.position_y += copy_index * layer.copies.offset_y_step;
    } else if (layer.copies.arrangement == CopyArrangement::Circular) {
        const double angle = layer.copies.circular_start_degrees +
            copy_index * layer.copies.circular_angle_step_degrees;
        constexpr double pi = 3.14159265358979323846;
        const double radians = angle * pi / 180.0;
        placement.position_x += layer.copies.circular_radius * std::cos(radians);
        placement.position_y += layer.copies.circular_radius * std::sin(radians);
        if (layer.copies.rotate_with_orbit) {
            placement.rotation_degrees += angle;
        }
    }
    return placement;
}

struct DocumentSettings {
    double page_width{210.0};
    double page_height{210.0};
    std::string unit{"mm"};
    RgbaColor background{1.0, 1.0, 1.0, 1.0};

    friend constexpr bool operator==(const DocumentSettings&, const DocumentSettings&) = default;
};

class Document {
public:
    friend bool operator==(const Document&, const Document&) = default;

    [[nodiscard]] CurveLayer& add_polar_rose(
        const curves::PolarRoseParameters& parameters = {},
        std::optional<std::string> name = std::nullopt);
    [[nodiscard]] CurveLayer& add_ellipse(
        const curves::EllipseParameters& parameters = {},
        std::optional<std::string> name = std::nullopt);
    [[nodiscard]] CurveLayer& add_lissajous(
        const curves::LissajousParameters& parameters = {},
        std::optional<std::string> name = std::nullopt);
    [[nodiscard]] CurveLayer& add_harmonograph(
        const curves::HarmonographParameters& parameters = {},
        std::optional<std::string> name = std::nullopt);
    [[nodiscard]] CurveLayer& add_droplet_rosette(
        const curves::DropletRosetteParameters& parameters = {},
        std::optional<std::string> name = std::nullopt);
    [[nodiscard]] CurveLayer& add_trochoid(
        CurveType type,
        const curves::TrochoidParameters& parameters = {},
        std::optional<std::string> name = std::nullopt);

    [[nodiscard]] std::string suggested_default_name(CurveType type) const;
    [[nodiscard]] CurveLayer* duplicate_layer(
        LayerId id,
        std::optional<std::string> name = std::nullopt);
    [[nodiscard]] bool import_layer(CurveLayer layer);

    [[nodiscard]] bool remove_layer(LayerId id);
    [[nodiscard]] bool move_layer(std::size_t from, std::size_t to);
    [[nodiscard]] bool rename_layer(LayerId id, std::string name);
    [[nodiscard]] bool set_layer_visible(LayerId id, bool visible);
    [[nodiscard]] bool set_layer_locked(LayerId id, bool locked);

    [[nodiscard]] CurveLayer* find_layer(LayerId id);
    [[nodiscard]] const CurveLayer* find_layer(LayerId id) const;

    [[nodiscard]] std::vector<CurveLayer>& layers() noexcept { return layers_; }
    [[nodiscard]] const std::vector<CurveLayer>& layers() const noexcept { return layers_; }
    [[nodiscard]] DocumentSettings& settings() noexcept { return settings_; }
    [[nodiscard]] const DocumentSettings& settings() const noexcept { return settings_; }

private:
    [[nodiscard]] std::string next_default_name(CurveType type);

    std::vector<CurveLayer> layers_;
    DocumentSettings settings_;
    std::array<std::size_t, static_cast<std::size_t>(CurveType::Count)> name_counters_{};
    LayerId next_id_{1};
};

} // namespace rosettelab::document
