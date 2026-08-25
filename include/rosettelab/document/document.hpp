#pragma once

#include "rosettelab/curves/polar_rose.hpp"
#include "rosettelab/document/appearance.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace rosettelab::document {

enum class CurveType : std::size_t {
    PolarRose,
    Hypotrochoid,
    Epitrochoid,
    Lissajous,
    Harmonograph,
    Spirograph,
    Count,
};

[[nodiscard]] std::string curve_type_name(CurveType type);

using LayerId = std::uint64_t;
using CurveParameters = std::variant<curves::PolarRoseParameters>;

struct CurveLayer {
    LayerId id{};
    std::string name;
    CurveType type{CurveType::PolarRose};
    CurveParameters parameters{curves::PolarRoseParameters{}};
    bool visible{true};
    bool locked{false};
    LayerAppearance appearance{};
};

struct DocumentSettings {
    double page_width{210.0};
    double page_height{210.0};
    std::string unit{"mm"};
    RgbaColor background{1.0, 1.0, 1.0, 1.0};

    friend constexpr bool operator==(const DocumentSettings&, const DocumentSettings&) = default;
};

class Document {
public:
    [[nodiscard]] CurveLayer& add_polar_rose(
        const curves::PolarRoseParameters& parameters = {},
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
