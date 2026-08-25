#include "ui/color_editor_dialog.hpp"
#include "ui/color_preview_button.hpp"

#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace rosettelab::ui {
namespace {

QString rgba_hex(const QColor& color)
{
    return QStringLiteral("#%1%2%3%4")
        .arg(color.red(), 2, 16, QLatin1Char('0'))
        .arg(color.green(), 2, 16, QLatin1Char('0'))
        .arg(color.blue(), 2, 16, QLatin1Char('0'))
        .arg(color.alpha(), 2, 16, QLatin1Char('0'))
        .toUpper();
}

} // namespace

ColorEditorDialog::ColorEditorDialog(QColor initial_color, QString title, QWidget* parent)
    : QDialog(parent)
    , color_(std::move(initial_color))
{
    setWindowTitle(std::move(title));
    setModal(true);
    setMinimumWidth(360);

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    format_ = new QComboBox(this);
    format_->addItem("RGBA", static_cast<int>(Format::Rgba));
    format_->addItem("HSLA", static_cast<int>(Format::Hsla));
    form->addRow("Color model", format_);

    hex_ = new QLineEdit(this);
    hex_->setMaxLength(9);
    hex_->setPlaceholderText("#RRGGBBAA");
    hex_->setToolTip("Web hexadecimal color: #RRGGBB or #RRGGBBAA. Opaque FF is added to RGB values.");
    hex_->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("#?[0-9A-Fa-f]{0,8}")), hex_));
    form->addRow("Hex color", hex_);

    for (std::size_t index = 0; index < channels_.size(); ++index) {
        channel_labels_[index] = new QLabel(this);
        channels_[index] = new QSpinBox(this);
        form->addRow(channel_labels_[index], channels_[index]);
    }
    root->addLayout(form);

    preview_ = new ColorPreviewButton(this);
    preview_->setMinimumHeight(42);
    preview_->setToolTip("Open the graphical color picker");
    root->addWidget(preview_);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(format_, &QComboBox::currentIndexChanged, this, [this] {
        configure_fields();
        update_fields_from_color();
    });
    connect(hex_, &QLineEdit::textChanged, this,
        [this](const QString& text) { update_color_from_hex(text); });
    for (auto* channel : channels_) {
        connect(channel, &QSpinBox::valueChanged, this, [this] {
            if (!updating_) {
                update_color_from_fields();
            }
        });
    }
    connect(preview_, &QPushButton::clicked, this, [this] { choose_visually(); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    configure_fields();
    update_fields_from_color();
}

void ColorEditorDialog::configure_fields()
{
    const auto model = static_cast<Format>(format_->currentData().toInt());
    const std::array<const char*, 4> rgba_labels{"Red", "Green", "Blue", "Alpha"};
    const std::array<const char*, 4> hsla_labels{"Hue", "Saturation", "Lightness", "Alpha"};
    const auto& labels = model == Format::Rgba ? rgba_labels : hsla_labels;

    updating_ = true;
    for (std::size_t index = 0; index < channels_.size(); ++index) {
        channel_labels_[index]->setText(labels[index]);
        channels_[index]->setSuffix(model == Format::Hsla && index > 0 ? " %" : "");
        channels_[index]->setRange(0, model == Format::Rgba ? 255 : (index == 0 ? 359 : 100));
    }
    updating_ = false;
}

void ColorEditorDialog::update_color_from_fields()
{
    const auto model = static_cast<Format>(format_->currentData().toInt());
    if (model == Format::Rgba) {
        color_.setRgb(
            channels_[0]->value(), channels_[1]->value(),
            channels_[2]->value(), channels_[3]->value());
    } else {
        color_ = QColor::fromHslF(
            static_cast<double>(channels_[0]->value()) / 360.0,
            static_cast<double>(channels_[1]->value()) / 100.0,
            static_cast<double>(channels_[2]->value()) / 100.0,
            static_cast<double>(channels_[3]->value()) / 100.0);
    }
    const QSignalBlocker blocker(hex_);
    hex_->setText(rgba_hex(color_));
    update_preview();
}

void ColorEditorDialog::update_fields_from_color()
{
    updating_ = true;
    hex_->setText(rgba_hex(color_));
    const auto model = static_cast<Format>(format_->currentData().toInt());
    if (model == Format::Rgba) {
        channels_[0]->setValue(color_.red());
        channels_[1]->setValue(color_.green());
        channels_[2]->setValue(color_.blue());
        channels_[3]->setValue(color_.alpha());
    } else {
        channels_[0]->setValue(std::max(0, color_.hslHue()));
        channels_[1]->setValue(static_cast<int>(color_.hslSaturationF() * 100.0));
        channels_[2]->setValue(static_cast<int>(color_.lightnessF() * 100.0));
        channels_[3]->setValue(static_cast<int>(color_.alphaF() * 100.0));
    }
    updating_ = false;
    update_preview();
}

void ColorEditorDialog::update_color_from_hex(QString text)
{
    if (updating_) {
        return;
    }

    text = text.trimmed().toUpper();
    if (text.isEmpty()) {
        return;
    }
    if (!text.startsWith('#')) {
        text.prepend('#');
    }
    if (text.size() == 7) {
        text.append("FF");
    }

    if (hex_->text() != text) {
        const QSignalBlocker blocker(hex_);
        hex_->setText(text);
        hex_->setCursorPosition(text.size());
    }
    if (text.size() != 9) {
        return;
    }

    bool valid = false;
    const auto value = text.sliced(1).toULongLong(&valid, 16);
    if (!valid) {
        return;
    }
    color_.setRgb(
        static_cast<int>((value >> 24) & 0xff),
        static_cast<int>((value >> 16) & 0xff),
        static_cast<int>((value >> 8) & 0xff),
        static_cast<int>(value & 0xff));
    update_fields_from_color();
}

void ColorEditorDialog::update_preview()
{
    preview_->setText(rgba_hex(color_) + "  -  Choose visually...");
    preview_->set_preview_color(color_);
}

void ColorEditorDialog::choose_visually()
{
    const auto selected = QColorDialog::getColor(
        color_, this, "Choose color", QColorDialog::ShowAlphaChannel);
    if (selected.isValid()) {
        color_ = selected;
        update_fields_from_color();
    }
}

} // namespace rosettelab::ui
