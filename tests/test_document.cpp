#include "rosettelab/document/document.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(const bool condition, const std::string_view message)
{
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

void test_default_names_and_stable_ids()
{
    rosettelab::document::Document document;
    const auto first_id = document.add_polar_rose().id;
    const auto second_id = document.add_polar_rose().id;

    require(first_id != second_id, "layer IDs should be unique");
    require(document.layers()[0].name == "Polar rose 1", "first default name should end in 1");
    require(document.layers()[1].name == "Polar rose 2", "second default name should end in 2");

    require(document.remove_layer(first_id), "existing layer should be removable");
    const auto third_id = document.add_polar_rose().id;
    require(document.find_layer(third_id)->name == "Polar rose 3",
            "default names should not be reused after deletion");
}

void test_custom_name_and_layer_state()
{
    rosettelab::document::Document document;
    const auto id = document.add_polar_rose({}, "My flower").id;

    require(document.find_layer(id)->name == "My flower", "custom name should be retained");
    require(document.set_layer_visible(id, false), "visibility should be editable");
    require(document.set_layer_locked(id, true), "lock should be editable");
    require(!document.find_layer(id)->visible, "layer should be hidden");
    require(document.find_layer(id)->locked, "layer should be locked");
    require(document.rename_layer(id, "Final name"), "layer should be renameable");
    require(document.find_layer(id)->name == "Final name", "new name should be retained");
    require(!document.rename_layer(id, ""), "empty layer name should be rejected");

    const auto next = document.add_polar_rose().id;
    require(document.find_layer(next)->name == "Polar rose 2",
            "custom names should still advance the family sequence");
}

void test_layer_reordering()
{
    rosettelab::document::Document document;
    const auto first = document.add_polar_rose({}, "First").id;
    const auto second = document.add_polar_rose({}, "Second").id;
    const auto third = document.add_polar_rose({}, "Third").id;

    require(document.move_layer(0, 2), "valid move should succeed");
    require(document.layers()[0].id == second, "second layer should move to the front");
    require(document.layers()[1].id == third, "third layer should remain in the middle");
    require(document.layers()[2].id == first, "first layer should move to the back");
    require(!document.move_layer(3, 0), "out-of-range move should fail");
}

void test_layer_duplication()
{
    rosettelab::document::Document document;
    auto parameters = rosettelab::curves::PolarRoseParameters{};
    parameters.k = 11.0;
    parameters.bezier_tolerance = 0.0125;
    const auto source_id = document.add_polar_rose(parameters, "Source").id;
    static_cast<void>(document.set_layer_visible(source_id, false));
    static_cast<void>(document.set_layer_locked(source_id, true));
    auto* source = document.find_layer(source_id);
    source->appearance.stroke = {0.2, 0.4, 0.8, 0.5};
    source->appearance.stroke_width = 2.5;
    source->appearance.fill_enabled = true;
    source->appearance.fill_rule = rosettelab::document::FillRule::EvenOdd;
    source->appearance.opacity = 0.75;
    source->appearance.blend_mode = rosettelab::document::BlendMode::Multiply;
    const auto expected_appearance = source->appearance;

    const auto* duplicate = document.duplicate_layer(source_id);
    require(duplicate != nullptr, "existing layer should be duplicable");
    require(duplicate->id != source_id, "duplicate should receive a new ID");
    require(duplicate->name == "Polar rose 2", "duplicate should receive the next default name");
    require(!duplicate->visible, "duplicate should preserve visibility");
    require(!duplicate->locked, "duplicate should start unlocked");
    require(duplicate->appearance == expected_appearance,
            "duplicate should preserve all appearance properties");
    require(std::get<rosettelab::curves::PolarRoseParameters>(duplicate->parameters).k == 11.0,
            "duplicate should preserve mathematical parameters");
    require(std::get<rosettelab::curves::PolarRoseParameters>(duplicate->parameters).bezier_tolerance == 0.0125,
            "duplicate should preserve curve tolerance");
    require(document.layers()[1].id == duplicate->id, "duplicate should follow its source");
    require(document.duplicate_layer(999999) == nullptr, "missing layer should not be duplicated");
}

void test_controlled_layer_import()
{
    rosettelab::document::Document document;
    rosettelab::document::CurveLayer imported;
    imported.id = 42;
    imported.name = "Polar rose 12";
    require(document.import_layer(imported), "valid native layer should be importable");
    require(!document.import_layer(imported), "duplicate imported ID should be rejected");
    const auto& next = document.add_polar_rose();
    require(next.id == 43, "new ID should follow the greatest imported ID");
    require(next.name == "Polar rose 13", "default name should follow imported sequence");
}

void test_document_settings_defaults()
{
    rosettelab::document::Document document;
    require(document.settings().page_width == 210.0, "default page width should be 210");
    require(document.settings().page_height == 210.0, "default page height should be 210");
    require(document.settings().unit == "mm", "default document unit should be millimetres");
    require(document.settings().background == rosettelab::document::RgbaColor{1.0, 1.0, 1.0, 1.0},
            "default page background should be opaque white");
}

} // namespace

int main()
{
    try {
        test_default_names_and_stable_ids();
        test_custom_name_and_layer_state();
        test_layer_reordering();
        test_layer_duplication();
        test_controlled_layer_import();
        test_document_settings_defaults();
        std::cout << "All RosetteLab document tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
