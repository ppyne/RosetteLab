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

} // namespace

int main()
{
    try {
        test_default_names_and_stable_ids();
        test_custom_name_and_layer_state();
        test_layer_reordering();
        std::cout << "All RosetteLab document tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
