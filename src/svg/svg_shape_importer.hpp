#pragma once

#include "rosettelab/core/geometry.hpp"

#include <QByteArray>
#include <QString>

namespace rosettelab::svg {

[[nodiscard]] core::BezierPath parse_svg_path_data(const QString& data);
[[nodiscard]] core::BezierPath import_svg_shapes(
    const QByteArray& data, double page_width, double page_height);

} // namespace rosettelab::svg
