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

    require(!rosettelab::render::requires_flattened_output(document),
            "normal layers should retain vector PDF output");

    layer.appearance.blend_mode = rosettelab::document::BlendMode::Multiply;
    require(rosettelab::render::requires_flattened_output(document),
            "a visible blended layer should flatten PDF output");

    layer.visible = false;
    require(!rosettelab::render::requires_flattened_output(document),
            "a hidden blended layer should not flatten PDF output");

    return EXIT_SUCCESS;
}
