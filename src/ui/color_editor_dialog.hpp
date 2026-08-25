#pragma once

#include <QColor>
#include <QDialog>

#include <array>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace rosettelab::ui {

class ColorPreviewButton;

class ColorEditorDialog final : public QDialog {
public:
    ColorEditorDialog(QColor initial_color, QString title, QWidget* parent = nullptr);

    [[nodiscard]] QColor color() const noexcept { return color_; }

private:
    enum class Format {
        Rgba,
        Hsla,
    };

    void configure_fields();
    void update_color_from_fields();
    void update_fields_from_color();
    void update_color_from_hex(QString text);
    void update_preview();
    void choose_visually();

    QColor color_;
    bool updating_{false};
    QComboBox* format_{};
    QLineEdit* hex_{};
    std::array<QLabel*, 4> channel_labels_{};
    std::array<QSpinBox*, 4> channels_{};
    ColorPreviewButton* preview_{};
};

} // namespace rosettelab::ui
