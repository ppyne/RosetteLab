#include "rosettelab/document/document.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace rosettelab::document {

std::string curve_type_name(const CurveType type)
{
    switch (type) {
    case CurveType::PolarRose:
        return "Polar rose";
    case CurveType::Hypotrochoid:
        return "Hypotrochoid";
    case CurveType::Epitrochoid:
        return "Epitrochoid";
    case CurveType::Lissajous:
        return "Lissajous";
    case CurveType::Harmonograph:
        return "Harmonograph";
    case CurveType::Spirograph:
        return "Spirograph";
    case CurveType::Count:
        break;
    }
    throw std::invalid_argument("Unknown curve type");
}

CurveLayer& Document::add_polar_rose(
    const curves::PolarRoseParameters& parameters,
    std::optional<std::string> name)
{
    if (!name.has_value() || name->empty()) {
        name = next_default_name(CurveType::PolarRose);
    }

    layers_.push_back({
        next_id_++,
        std::move(*name),
        CurveType::PolarRose,
        parameters,
        true,
        false,
    });
    return layers_.back();
}

bool Document::remove_layer(const LayerId id)
{
    const auto iterator = std::find_if(layers_.begin(), layers_.end(),
        [id](const CurveLayer& layer) { return layer.id == id; });
    if (iterator == layers_.end()) {
        return false;
    }
    layers_.erase(iterator);
    return true;
}

bool Document::move_layer(const std::size_t from, const std::size_t to)
{
    if (from >= layers_.size() || to >= layers_.size()) {
        return false;
    }
    if (from == to) {
        return true;
    }

    auto layer = std::move(layers_[from]);
    layers_.erase(layers_.begin() + static_cast<std::ptrdiff_t>(from));
    layers_.insert(layers_.begin() + static_cast<std::ptrdiff_t>(to), std::move(layer));
    return true;
}

bool Document::rename_layer(const LayerId id, std::string name)
{
    if (name.empty()) {
        return false;
    }
    auto* layer = find_layer(id);
    if (layer == nullptr) {
        return false;
    }
    layer->name = std::move(name);
    return true;
}

bool Document::set_layer_visible(const LayerId id, const bool visible)
{
    auto* layer = find_layer(id);
    if (layer == nullptr) {
        return false;
    }
    layer->visible = visible;
    return true;
}

bool Document::set_layer_locked(const LayerId id, const bool locked)
{
    auto* layer = find_layer(id);
    if (layer == nullptr) {
        return false;
    }
    layer->locked = locked;
    return true;
}

CurveLayer* Document::find_layer(const LayerId id)
{
    const auto iterator = std::find_if(layers_.begin(), layers_.end(),
        [id](const CurveLayer& layer) { return layer.id == id; });
    return iterator == layers_.end() ? nullptr : &*iterator;
}

const CurveLayer* Document::find_layer(const LayerId id) const
{
    const auto iterator = std::find_if(layers_.begin(), layers_.end(),
        [id](const CurveLayer& layer) { return layer.id == id; });
    return iterator == layers_.end() ? nullptr : &*iterator;
}

std::string Document::next_default_name(const CurveType type)
{
    const auto index = static_cast<std::size_t>(type);
    if (index >= name_counters_.size()) {
        throw std::invalid_argument("Unknown curve type");
    }
    return curve_type_name(type) + " " + std::to_string(++name_counters_[index]);
}

} // namespace rosettelab::document

