#pragma once

#include "rosettelab/document/document.hpp"

#include <QWidget>

#include <functional>

class QLabel;
class QMouseEvent;
class QToolButton;

namespace rosettelab::ui {

class ElidedLabel;

class LayerListItemWidget final : public QWidget {
public:
    using ToggleCallback = std::function<void(bool)>;
    using SelectionCallback = std::function<void()>;

    LayerListItemWidget(
        document::LayerId id,
        QString name,
        bool visible,
        bool locked,
        SelectionCallback selected,
        ToggleCallback visibility_changed,
        ToggleCallback lock_changed,
        QWidget* parent = nullptr);

    [[nodiscard]] document::LayerId layer_id() const noexcept { return layer_id_; }
    void set_name(const QString& name);
    void set_layer_preview(const document::CurveLayer& layer);
    void set_visible_state(bool visible);
    void set_locked_state(bool locked);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void refresh_visibility_control();
    void refresh_lock_control();

    document::LayerId layer_id_{};
    bool visible_{true};
    bool locked_{false};
    SelectionCallback selected_;
    ToggleCallback visibility_changed_;
    ToggleCallback lock_changed_;
    QToolButton* visibility_button_{};
    QToolButton* lock_button_{};
    QLabel* preview_label_{};
    ElidedLabel* name_label_{};
};

} // namespace rosettelab::ui
