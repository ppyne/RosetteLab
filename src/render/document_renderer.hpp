#pragma once

#include "rosettelab/document/document.hpp"

#include <QRectF>

class QPainter;

namespace rosettelab::render {

[[nodiscard]] bool requires_flattened_output(const document::Document& document);

void render_document(
    QPainter& painter,
    const document::Document& document,
    const QRectF& page_rect);

} // namespace rosettelab::render
