#pragma once

#include <QFont>
#include <QRawFont>
#include <QString>

namespace rosettelab::render {

inline QString replace_unsupported_glyphs(const QString& source, const QFont& selected_font)
{
    const QRawFont raw_font = QRawFont::fromFont(selected_font);
    if (!raw_font.isValid()) return QString(source.size(), QChar(0x25A1));
    QString result;
    for (const auto codepoint : source.toUcs4()) {
        if (raw_font.supportsCharacter(codepoint)) {
            const char32_t character = static_cast<char32_t>(codepoint);
            result.append(QString::fromUcs4(&character, 1));
        } else {
            result.append(QChar(0x25A1));
        }
    }
    return result;
}

} // namespace rosettelab::render
