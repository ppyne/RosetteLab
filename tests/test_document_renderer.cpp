#include "render/document_renderer.hpp"

#include <cstdlib>
#include <iostream>

namespace {

void require(const bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main()
{
    rosettelab::document::Document document;
    auto& layer = document.add_polar_rose();

    require(!rosettelab::render::has_visible_blend_modes(document),
            "normal layers should not report blend modes");

    layer.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;
    require(rosettelab::render::has_visible_blend_modes(document),
            "a visible blended layer should report blend modes");

    layer.visible = false;
    require(!rosettelab::render::has_visible_blend_modes(document),
            "a hidden blended layer should not report blend modes");

    return EXIT_SUCCESS;
}
