#pragma once

#include "b2img/app/render_service.hpp"
#include "b2img/core/image.hpp"
#include "b2img/core/render_request.hpp"

#include <gtk/gtk.h>

#include <optional>
#include <string>

namespace b2img::gui {

class MainWindow {
public:
    explicit MainWindow(GtkApplication* application);

    [[nodiscard]] GtkWindow* window() const noexcept;
    void present() const;

private:
    GtkApplication* m_application {};
    GtkWidget* m_window {};
    GtkWidget* m_input_entry {};
    GtkWidget* m_output_entry {};
    GtkWidget* m_width_spin {};
    GtkWidget* m_height_spin {};
    GtkWidget* m_width_scale {};
    GtkWidget* m_height_scale {};
    GtkWidget* m_bit_offset_entry {};
    GtkWidget* m_bits_per_pixel_spin {};
    GtkWidget* m_encoding_dropdown {};
    GtkWidget* m_layout_dropdown {};
    GtkWidget* m_palette_dropdown {};
    GtkWidget* m_bit_order_dropdown {};
    GtkWidget* m_flip_vertical_check {};
    GtkWidget* m_zoom_scale {};
    GtkWidget* m_picture {};
    GtkWidget* m_preview_scroll {};
    GtkWidget* m_status_label {};
    GtkAdjustment* m_zoom_adjustment {};
    GtkTextBuffer* m_report_buffer {};
    guint m_preview_timeout_id {};
    int m_rotation_turns {};
    bool m_syncing_dimension_controls {false};

    b2img::app::RenderService m_service;
    std::optional<b2img::core::Image> m_last_image;

    void build();
    void sync_control_state();
    [[nodiscard]] b2img::core::RenderRequest read_request(bool require_output_path) const;
    void choose_input_file();
    void choose_output_file();
    void inspect();
    void preview();
    void schedule_preview();
    void run_live_preview();
    void clear_preview();
    void rotate_left();
    void rotate_right();
    void save();
    void set_status(const std::string& text, bool is_error);
    void set_report(const std::string& text);
    void update_preview(const b2img::core::Image& image);
    void on_file_dropped(GFile* file);

    static void on_open_clicked(GtkButton* button, gpointer user_data);
    static void on_pick_output_clicked(GtkButton* button, gpointer user_data);
    static void on_inspect_clicked(GtkButton* button, gpointer user_data);
    static void on_rotate_left_clicked(GtkButton* button, gpointer user_data);
    static void on_rotate_right_clicked(GtkButton* button, gpointer user_data);
    static void on_save_clicked(GtkButton* button, gpointer user_data);
    static void on_live_editable_changed(GtkEditable* editable, gpointer user_data);
    static void on_live_spin_changed(GtkSpinButton* spin_button, gpointer user_data);
    static void on_dimension_scale_changed(GtkRange* range, gpointer user_data);
    static void on_live_check_toggled(GtkCheckButton* check_button, gpointer user_data);
    static void on_dropdown_changed(GObject* object, GParamSpec* spec, gpointer user_data);
    static void on_zoom_changed(GtkAdjustment* adjustment, gpointer user_data);
    static gboolean on_preview_timeout(gpointer user_data);
    static gboolean on_file_drop(GtkDropTarget* target, const GValue* value, double x, double y, gpointer user_data);
    static void on_open_dialog_complete(GObject* source_object, GAsyncResult* result, gpointer user_data);
    static void on_save_dialog_complete(GObject* source_object, GAsyncResult* result, gpointer user_data);
};

}  // namespace b2img::gui
