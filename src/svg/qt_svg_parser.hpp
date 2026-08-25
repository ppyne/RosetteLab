#pragma once

#include "rosettelab/document/document.hpp"

#include <QByteArray>

namespace rosettelab::svg {

[[nodiscard]] document::Document parse_rosettelab_svg(const QByteArray& data);

} // namespace rosettelab::svg

