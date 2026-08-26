#include "rosettelab/document/document.hpp"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace rosettelab::document {

std::string curve_type_name(const CurveType type)
{
    switch (type) {
    case CurveType::PolarRose:
        return "Polar rose";
    case CurveType::Ellipse:
        return "Ellipse";
    case CurveType::Hypotrochoid:
        return "Hypotrochoid";
    case CurveType::Epitrochoid:
        return "Epitrochoid";
    case CurveType::Lissajous:
        return "Lissajous";
    case CurveType::Harmonograph:
        return "Harmonograph";
    case CurveType::Count:
        break;
    }
    throw std::invalid_argument("Unknown curve type");
}

CurveLayer& Document::add_polar_rose(
    const curves::PolarRoseParameters& parameters,
    std::optional<std::string> name)
{
    const auto default_name = next_default_name(CurveType::PolarRose);
    if (!name.has_value() || name->empty()) {
        name = default_name;
    }

    layers_.push_back({
        next_id_++,
        std::move(*name),
        CurveType::PolarRose,
        parameters,
        true,
        false,
        {},
        {},
        {},
        "",
        false,
    });
    return layers_.back();
}

CurveLayer& Document::add_ellipse(
    const curves::EllipseParameters& parameters,
    std::optional<std::string> name)
{
    const auto default_name = next_default_name(CurveType::Ellipse);
    if (!name.has_value() || name->empty()) {
        name = default_name;
    }

    layers_.push_back({
        next_id_++, std::move(*name), CurveType::Ellipse, parameters, true, false, {}, {}, {}, "", false,
    });
    return layers_.back();
}

CurveLayer& Document::add_trochoid(
    const CurveType type,
    const curves::TrochoidParameters& parameters,
    std::optional<std::string> name)
{
    if (type != CurveType::Hypotrochoid && type != CurveType::Epitrochoid) {
        throw std::invalid_argument("Trochoid layer requires a trochoid curve type");
    }
    const auto default_name = next_default_name(type);
    if (!name.has_value() || name->empty()) {
        name = default_name;
    }
    layers_.push_back({next_id_++, std::move(*name), type, parameters, true, false, {}, {}, {}, "", false});
    return layers_.back();
}

CurveLayer& Document::add_lissajous(
    const curves::LissajousParameters& parameters,
    std::optional<std::string> name)
{
    const auto default_name = next_default_name(CurveType::Lissajous);
    if (!name.has_value() || name->empty()) name = default_name;
    layers_.push_back({next_id_++, std::move(*name), CurveType::Lissajous,
                       parameters, true, false, {}, {}, {}, "", false});
    return layers_.back();
}

CurveLayer& Document::add_harmonograph(
    const curves::HarmonographParameters& parameters,
    std::optional<std::string> name)
{
    const auto default_name = next_default_name(CurveType::Harmonograph);
    if (!name.has_value() || name->empty()) name = default_name;
    layers_.push_back({next_id_++, std::move(*name), CurveType::Harmonograph,
                       parameters, true, false, {}, {}, {}, "", false});
    return layers_.back();
}

std::string Document::suggested_default_name(const CurveType type) const
{
    const auto index = static_cast<std::size_t>(type);
    if (index >= name_counters_.size()) {
        throw std::invalid_argument("Unknown curve type");
    }
    return curve_type_name(type) + " " + std::to_string(name_counters_[index] + 1);
}

CurveLayer* Document::duplicate_layer(const LayerId id, std::optional<std::string> name)
{
    const auto iterator = std::find_if(layers_.begin(), layers_.end(),
        [id](const CurveLayer& layer) { return layer.id == id; });
    if (iterator == layers_.end()) {
        return nullptr;
    }

    const auto source_index = static_cast<std::size_t>(std::distance(layers_.begin(), iterator));
    const auto source = *iterator;
    const auto default_name = next_default_name(source.type);
    if (!name.has_value() || name->empty()) {
        name = default_name;
    }

    CurveLayer duplicate{
        next_id_++,
        std::move(*name),
        source.type,
        source.parameters,
        source.visible,
        false,
        source.appearance,
        source.transform,
        source.copies,
        source.preset_id,
        source.preset_customized,
    };
    const auto inserted = layers_.insert(
        layers_.begin() + static_cast<std::ptrdiff_t>(source_index + 1),
        std::move(duplicate));
    return &*inserted;
}

bool Document::import_layer(CurveLayer layer)
{
    if (layer.id == 0 || layer.name.empty() || find_layer(layer.id) != nullptr) {
        return false;
    }
    const bool compatible =
        (layer.type == CurveType::PolarRose &&
         std::holds_alternative<curves::PolarRoseParameters>(layer.parameters)) ||
        (layer.type == CurveType::Ellipse &&
         std::holds_alternative<curves::EllipseParameters>(layer.parameters)) ||
        (layer.type == CurveType::Lissajous &&
         std::holds_alternative<curves::LissajousParameters>(layer.parameters)) ||
        (layer.type == CurveType::Harmonograph &&
         std::holds_alternative<curves::HarmonographParameters>(layer.parameters)) ||
        ((layer.type == CurveType::Hypotrochoid || layer.type == CurveType::Epitrochoid) &&
         std::holds_alternative<curves::TrochoidParameters>(layer.parameters));
    if (!compatible) {
        return false;
    }

    const auto type_index = static_cast<std::size_t>(layer.type);
    const auto prefix = curve_type_name(layer.type) + " ";
    if (layer.name.starts_with(prefix)) {
        const auto suffix = std::string_view(layer.name).substr(prefix.size());
        std::size_t value = 0;
        bool numeric = !suffix.empty();
        for (const char character : suffix) {
            if (character < '0' || character > '9') {
                numeric = false;
                break;
            }
            value = value * 10 + static_cast<std::size_t>(character - '0');
        }
        if (numeric) {
            name_counters_[type_index] = std::max(name_counters_[type_index], value);
        }
    }

    next_id_ = std::max(next_id_, layer.id + 1);
    layers_.push_back(std::move(layer));
    return true;
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
