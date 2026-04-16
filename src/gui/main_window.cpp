#include "b2img/gui/main_window.hpp"
#include "b2img/core/encoding.hpp"

#include <array>
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace b2img::gui {

namespace {

struct DropdownOption {
    const char* label;
};

constexpr std::array<DropdownOption, 4> kLayoutLabels {{
    {"linear"},
    {"planar-image"},
    {"planar-row"},
    {"planar-pixel"},
}};

constexpr std::array<DropdownOption, 5> kPaletteLabels {{
    {"grayscale"},
    {"vga16"},
    {"cga0"},
    {"cga1"},
    {"cga2"},
}};

constexpr const char* kLayoutStrings[] = {"linear", "planar-image", "planar-row", "planar-pixel", nullptr};
constexpr const char* kPaletteStrings[] = {"grayscale", "vga16", "cga0", "cga1", "cga2", nullptr};
constexpr const char* kBitOrderStrings[] = {"msb", "lsb", nullptr};

constexpr double kDimensionMin = 1.0;
constexpr double kDimensionMax = 65535.0;
constexpr double kDimensionSliderMax = 2000.0;
constexpr double kZoomMin = 100.0;
constexpr double kZoomMax = 800.0;
constexpr double kZoomStep = 10.0;

std::size_t parse_size(std::string_view value, const char* field_name) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(field_name) + " cannot be empty");
    }

    std::size_t result {};
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto parsed = std::from_chars(begin, end, result);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        throw std::invalid_argument(std::string("invalid numeric value for ") + field_name + ": " + std::string(value));
    }

    return result;
}

std::string encoding_name(const b2img::core::RenderRequest& request) {
    return request.encoding_name;
}

std::string layout_name(b2img::core::Layout layout) {
    return kLayoutLabels[static_cast<std::size_t>(layout)].label;
}

std::string palette_name(b2img::core::PaletteKind palette) {
    return kPaletteLabels[static_cast<std::size_t>(palette)].label;
}

std::string format_report(const b2img::core::RenderRequest& request, const b2img::app::InspectionResult& inspection) {
    return
        "input: " + request.input_path + "\n" +
        "file-size-bytes: " + std::to_string(inspection.file_size_bytes) + "\n" +
        "width: " + std::to_string(request.width) + "\n" +
        "height: " + std::to_string(request.height) + "\n" +
        "encoding: " + encoding_name(request) + "\n" +
        "layout: " + layout_name(request.layout) + "\n" +
        "bits-per-pixel: " + std::to_string(request.bits_per_pixel) + "\n" +
        "bit-offset: " + std::to_string(request.bit_offset) + "\n" +
        "rotation-turns: " + std::to_string(request.rotation_turns) + "\n" +
        "palette: " + palette_name(request.palette) + "\n" +
        "required-bits: " + std::to_string(inspection.analysis.required_bits) + "\n" +
        "available-bits: " + std::to_string(inspection.analysis.available_bits) + "\n" +
        "trailing-bits: " + std::to_string(inspection.analysis.trailing_bits) + "\n" +
        "enough-data: " + std::string(inspection.analysis.enough_data ? "yes" : "no");
}

GtkWidget* create_labeled_row(const char* title, GtkWidget* control) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget* label = gtk_label_new(title);
    gtk_widget_set_size_request(label, 120, -1);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
    gtk_widget_set_hexpand(control, TRUE);
    gtk_box_append(GTK_BOX(row), label);
    gtk_box_append(GTK_BOX(row), control);
    return row;
}

GtkWidget* create_labeled_browse_row(const char* title, GtkWidget* control, GtkWidget* button) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget* label = gtk_label_new(title);
    GtkWidget* wrapper = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_size_request(label, 120, -1);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
    gtk_widget_set_hexpand(wrapper, TRUE);
    gtk_widget_set_hexpand(control, TRUE);
    gtk_box_append(GTK_BOX(wrapper), control);
    gtk_box_append(GTK_BOX(wrapper), button);
    gtk_box_append(GTK_BOX(row), label);
    gtk_box_append(GTK_BOX(row), wrapper);
    return row;
}

GtkWidget* create_spin_scale_group(const char* title, GtkWidget* spin, GtkWidget* scale) {
    GtkWidget* group = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget* label = gtk_label_new(title);

    gtk_widget_set_size_request(label, 120, -1);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
    gtk_widget_set_hexpand(spin, TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_widget_set_hexpand(scale, TRUE);

    gtk_box_append(GTK_BOX(row), label);
    gtk_box_append(GTK_BOX(row), spin);
    gtk_box_append(GTK_BOX(group), row);
    gtk_box_append(GTK_BOX(group), scale);
    return group;
}

std::vector<std::uint8_t> make_rgb_bytes(const b2img::core::Image& image) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(image.pixels().size() * 3U);

    for (const auto& pixel : image.pixels()) {
        bytes.push_back(pixel.red);
        bytes.push_back(pixel.green);
        bytes.push_back(pixel.blue);
    }

    return bytes;
}

bool is_cancelled_error(const GError* error) {
    return error != nullptr && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
}

void sync_dimension_controls(GtkWidget* spin, GtkWidget* scale, bool syncing, bool from_scale) {
    if (syncing) {
        return;
    }

    if (from_scale) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), gtk_range_get_value(GTK_RANGE(scale)));
        return;
    }

    const double spin_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
    gtk_range_set_value(GTK_RANGE(scale), std::min(spin_value, kDimensionSliderMax));
}

}  // namespace

MainWindow::MainWindow(GtkApplication* application)
    : m_application(application) {
    build();
    sync_control_state();
}

GtkWindow* MainWindow::window() const noexcept {
    return GTK_WINDOW(m_window);
}

void MainWindow::present() const {
    gtk_window_present(GTK_WINDOW(m_window));
}

void MainWindow::build() {
    m_window = gtk_application_window_new(m_application);
    gtk_window_set_title(GTK_WINDOW(m_window), "b2img");
    gtk_window_set_default_size(GTK_WINDOW(m_window), 1360, 860);

    GtkDropTarget* drop_target = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
    g_signal_connect(drop_target, "drop", G_CALLBACK(on_file_drop), this);
    gtk_widget_add_controller(m_window, GTK_EVENT_CONTROLLER(drop_target));

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(m_window), root);

    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top(header, 16);
    gtk_widget_set_margin_bottom(header, 12);
    gtk_widget_set_margin_start(header, 16);
    gtk_widget_set_margin_end(header, 16);

    GtkWidget* title = gtk_label_new("b2img");
    gtk_widget_add_css_class(title, "title-2");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0F);

    GtkWidget* subtitle = gtk_label_new("Exploring binary data as image slices");
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0F);
    gtk_widget_add_css_class(subtitle, "dim-label");

    gtk_box_append(GTK_BOX(header), title);
    gtk_box_append(GTK_BOX(header), subtitle);
    gtk_box_append(GTK_BOX(root), header);

    GtkWidget* content = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_start(content, 16);
    gtk_widget_set_margin_end(content, 16);
    gtk_widget_set_margin_bottom(content, 16);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_box_append(GTK_BOX(root), content);

    GtkWidget* left_scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(left_scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(left_scroller, 380, -1);
    gtk_paned_set_start_child(GTK_PANED(content), left_scroller);

    GtkWidget* controls = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(controls, 8);
    gtk_widget_set_margin_bottom(controls, 8);
    gtk_widget_set_margin_start(controls, 8);
    gtk_widget_set_margin_end(controls, 8);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(left_scroller), controls);

    m_input_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(m_input_entry), "/path/to/input.bin");
    g_signal_connect(m_input_entry, "changed", G_CALLBACK(on_live_editable_changed), this);
    GtkWidget* open_button = gtk_button_new_with_label("Open...");
    g_signal_connect(open_button, "clicked", G_CALLBACK(on_open_clicked), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_browse_row("Input file", m_input_entry, open_button));

    m_output_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(m_output_entry), "/path/to/output.ppm");
    gtk_editable_set_text(GTK_EDITABLE(m_output_entry), "output.ppm");
    GtkWidget* output_button = gtk_button_new_with_label("Save As...");
    g_signal_connect(output_button, "clicked", G_CALLBACK(on_pick_output_clicked), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_browse_row("Output file", m_output_entry, output_button));

    m_width_spin = gtk_spin_button_new_with_range(kDimensionMin, kDimensionMax, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_width_spin), 128.0);
    g_signal_connect(m_width_spin, "value-changed", G_CALLBACK(on_live_spin_changed), this);
    m_width_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, kDimensionMin, kDimensionSliderMax, 1.0);
    gtk_range_set_value(GTK_RANGE(m_width_scale), 128.0);
    g_signal_connect(m_width_scale, "value-changed", G_CALLBACK(on_dimension_scale_changed), this);
    gtk_box_append(GTK_BOX(controls), create_spin_scale_group("Width", m_width_spin, m_width_scale));

    m_height_spin = gtk_spin_button_new_with_range(kDimensionMin, kDimensionMax, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_height_spin), 128.0);
    g_signal_connect(m_height_spin, "value-changed", G_CALLBACK(on_live_spin_changed), this);
    m_height_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, kDimensionMin, kDimensionSliderMax, 1.0);
    gtk_range_set_value(GTK_RANGE(m_height_scale), 128.0);
    g_signal_connect(m_height_scale, "value-changed", G_CALLBACK(on_dimension_scale_changed), this);
    gtk_box_append(GTK_BOX(controls), create_spin_scale_group("Height", m_height_spin, m_height_scale));

    m_bit_offset_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(m_bit_offset_entry), "0");
    g_signal_connect(m_bit_offset_entry, "changed", G_CALLBACK(on_live_editable_changed), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_row("Bit offset", m_bit_offset_entry));

    m_bits_per_pixel_spin = gtk_spin_button_new_with_range(1.0, 8.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_bits_per_pixel_spin), 8.0);
    g_signal_connect(m_bits_per_pixel_spin, "value-changed", G_CALLBACK(on_live_spin_changed), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_row("Bits/pixel", m_bits_per_pixel_spin));

    m_encoding_dropdown = gtk_drop_down_new_from_strings(b2img::core::kEncodingNames.data());
    gtk_drop_down_set_selected(GTK_DROP_DOWN(m_encoding_dropdown), 0);
    g_signal_connect(m_encoding_dropdown, "notify::selected", G_CALLBACK(on_dropdown_changed), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_row("Encoding", m_encoding_dropdown));

    m_layout_dropdown = gtk_drop_down_new_from_strings(kLayoutStrings);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(m_layout_dropdown), 0);
    g_signal_connect(m_layout_dropdown, "notify::selected", G_CALLBACK(on_dropdown_changed), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_row("Layout", m_layout_dropdown));

    m_palette_dropdown = gtk_drop_down_new_from_strings(kPaletteStrings);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(m_palette_dropdown), 0);
    g_signal_connect(m_palette_dropdown, "notify::selected", G_CALLBACK(on_dropdown_changed), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_row("Palette", m_palette_dropdown));

    m_bit_order_dropdown = gtk_drop_down_new_from_strings(kBitOrderStrings);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(m_bit_order_dropdown), 0);
    g_signal_connect(m_bit_order_dropdown, "notify::selected", G_CALLBACK(on_dropdown_changed), this);
    gtk_box_append(GTK_BOX(controls), create_labeled_row("Bit order", m_bit_order_dropdown));

    m_flip_vertical_check = gtk_check_button_new_with_label("Flip image vertically");
    g_signal_connect(m_flip_vertical_check, "toggled", G_CALLBACK(on_live_check_toggled), this);
    gtk_box_append(GTK_BOX(controls), m_flip_vertical_check);

    GtkWidget* actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* inspect_button = gtk_button_new_with_label("Inspect");
    GtkWidget* rotate_left_button = gtk_button_new_with_label("Rotate Left");
    GtkWidget* rotate_right_button = gtk_button_new_with_label("Rotate Right");
    GtkWidget* save_button = gtk_button_new_with_label("Save PPM");
    g_signal_connect(inspect_button, "clicked", G_CALLBACK(on_inspect_clicked), this);
    g_signal_connect(rotate_left_button, "clicked", G_CALLBACK(on_rotate_left_clicked), this);
    g_signal_connect(rotate_right_button, "clicked", G_CALLBACK(on_rotate_right_clicked), this);
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), this);
    gtk_box_append(GTK_BOX(actions), inspect_button);
    gtk_box_append(GTK_BOX(actions), rotate_left_button);
    gtk_box_append(GTK_BOX(actions), rotate_right_button);
    gtk_box_append(GTK_BOX(actions), save_button);
    gtk_box_append(GTK_BOX(controls), actions);

    m_status_label = gtk_label_new("Ready.");
    gtk_label_set_xalign(GTK_LABEL(m_status_label), 0.0F);
    gtk_label_set_wrap(GTK_LABEL(m_status_label), TRUE);
    gtk_box_append(GTK_BOX(controls), m_status_label);

    GtkWidget* report_frame = gtk_frame_new("Inspection");
    GtkWidget* report_scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(report_scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(report_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget* report_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(report_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(report_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(report_view), TRUE);
    m_report_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(report_view));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(report_scroll), report_view);
    gtk_frame_set_child(GTK_FRAME(report_frame), report_scroll);
    gtk_box_append(GTK_BOX(controls), report_frame);

    GtkWidget* right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_paned_set_resize_start_child(GTK_PANED(content), FALSE);
    gtk_paned_set_end_child(GTK_PANED(content), right_box);

    GtkWidget* preview_title = gtk_label_new("Preview");
    gtk_widget_add_css_class(preview_title, "heading");
    gtk_label_set_xalign(GTK_LABEL(preview_title), 0.0F);
    gtk_box_append(GTK_BOX(right_box), preview_title);

    m_zoom_adjustment = gtk_adjustment_new(100.0, kZoomMin, kZoomMax, kZoomStep, 50.0, 0.0);
    g_signal_connect(m_zoom_adjustment, "value-changed", G_CALLBACK(on_zoom_changed), this);
    m_zoom_scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, m_zoom_adjustment);
    gtk_scale_set_draw_value(GTK_SCALE(m_zoom_scale), TRUE);
    gtk_scale_set_digits(GTK_SCALE(m_zoom_scale), 0);
    gtk_widget_set_hexpand(m_zoom_scale, TRUE);
    gtk_box_append(GTK_BOX(right_box), create_labeled_row("Zoom %", m_zoom_scale));

    GtkWidget* preview_frame = gtk_frame_new(nullptr);
    gtk_widget_set_hexpand(preview_frame, TRUE);
    gtk_widget_set_vexpand(preview_frame, TRUE);
    gtk_box_append(GTK_BOX(right_box), preview_frame);

    m_preview_scroll = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(m_preview_scroll, TRUE);
    gtk_widget_set_vexpand(m_preview_scroll, TRUE);
    gtk_frame_set_child(GTK_FRAME(preview_frame), m_preview_scroll);

    m_picture = gtk_picture_new();
    gtk_picture_set_can_shrink(GTK_PICTURE(m_picture), FALSE);
    gtk_picture_set_alternative_text(GTK_PICTURE(m_picture), "Rendered preview");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_preview_scroll), m_picture);
}

void MainWindow::sync_control_state() {
    const auto encoding_index = gtk_drop_down_get_selected(GTK_DROP_DOWN(m_encoding_dropdown));
    const bool indexed = encoding_index == 0U;

    gtk_widget_set_sensitive(m_layout_dropdown, indexed);
    gtk_widget_set_sensitive(m_palette_dropdown, indexed);
    gtk_widget_set_sensitive(m_bit_order_dropdown, indexed);
    gtk_widget_set_sensitive(m_bits_per_pixel_spin, indexed);

    if (!indexed) {
        gtk_drop_down_set_selected(GTK_DROP_DOWN(m_layout_dropdown), 0U);
    }
}

b2img::core::RenderRequest MainWindow::read_request(bool require_output_path) const {
    b2img::core::RenderRequest request;
    request.input_path = gtk_editable_get_text(GTK_EDITABLE(m_input_entry));
    request.output_path = gtk_editable_get_text(GTK_EDITABLE(m_output_entry));
    request.width = static_cast<std::size_t>(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_width_spin)));
    request.height = static_cast<std::size_t>(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_height_spin)));
    request.bit_offset = parse_size(gtk_editable_get_text(GTK_EDITABLE(m_bit_offset_entry)), "bit offset");
    request.bits_per_pixel = static_cast<std::uint8_t>(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(m_bits_per_pixel_spin)));
    const auto selected_encoding = b2img::core::kEncodingNames.at(gtk_drop_down_get_selected(GTK_DROP_DOWN(m_encoding_dropdown)));
    request.encoding_name = selected_encoding == nullptr ? "indexed" : selected_encoding;
    const auto encoding_spec = b2img::core::find_encoding_spec(request.encoding_name);
    if (!encoding_spec.has_value()) {
        throw std::invalid_argument("unsupported encoding selected");
    }
    request.encoding_kind = encoding_spec->kind;
    request.layout = static_cast<b2img::core::Layout>(gtk_drop_down_get_selected(GTK_DROP_DOWN(m_layout_dropdown)));
    request.palette = static_cast<b2img::core::PaletteKind>(gtk_drop_down_get_selected(GTK_DROP_DOWN(m_palette_dropdown)));
    request.bit_order = static_cast<b2img::core::BitOrder>(gtk_drop_down_get_selected(GTK_DROP_DOWN(m_bit_order_dropdown)));
    request.flip_vertical = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_flip_vertical_check));
    request.rotation_turns = m_rotation_turns;

    b2img::app::RenderService::validate_request(request, require_output_path);
    return request;
}

void MainWindow::choose_input_file() {
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Open binary file");
    gtk_file_dialog_set_accept_label(dialog, "Open");
    gtk_file_dialog_open(dialog, GTK_WINDOW(m_window), nullptr, on_open_dialog_complete, this);
}

void MainWindow::choose_output_file() {
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Choose output image path");
    gtk_file_dialog_set_accept_label(dialog, "Save");

    const std::string current_output = gtk_editable_get_text(GTK_EDITABLE(m_output_entry));
    const std::string suggested_name = current_output.empty()
        ? "output.ppm"
        : std::filesystem::path(current_output).filename().string();
    gtk_file_dialog_set_initial_name(dialog, suggested_name.c_str());
    gtk_file_dialog_save(dialog, GTK_WINDOW(m_window), nullptr, on_save_dialog_complete, this);
}

void MainWindow::inspect() {
    try {
        const auto request = read_request(false);
        const auto inspection = m_service.inspect(request);
        set_report(format_report(request, inspection));
        set_status("Inspection completed.", false);
    } catch (const std::exception& error) {
        set_status(error.what(), true);
    }
}

void MainWindow::preview() {
    try {
        const auto request = read_request(false);
        const auto result = m_service.render(request);
        m_last_image = result.image;
        update_preview(result.image);
        set_report(format_report(request, result.inspection));
        set_status("Preview rendered.", false);
    } catch (const std::exception& error) {
        clear_preview();
        set_status(error.what(), true);
    }
}

void MainWindow::schedule_preview() {
    if (m_preview_timeout_id != 0U) {
        return;
    }

    m_preview_timeout_id = g_idle_add(on_preview_timeout, this);
}

void MainWindow::run_live_preview() {
    const std::string input_path = gtk_editable_get_text(GTK_EDITABLE(m_input_entry));
    if (input_path.empty()) {
        clear_preview();
        set_report("Choose an input file to start preview.");
        set_status("Choose an input file.", false);
        return;
    }

    preview();
}

void MainWindow::clear_preview() {
    m_last_image.reset();
    gtk_picture_set_paintable(GTK_PICTURE(m_picture), nullptr);
    gtk_widget_set_size_request(m_picture, -1, -1);
}

void MainWindow::rotate_left() {
    m_rotation_turns = (m_rotation_turns + 3) % 4;
    schedule_preview();
}

void MainWindow::rotate_right() {
    m_rotation_turns = (m_rotation_turns + 1) % 4;
    schedule_preview();
}

void MainWindow::save() {
    try {
        const auto request = read_request(true);
        const auto result = m_service.render(request);
        m_service.save_ppm(request.output_path, result.image);
        m_last_image = result.image;
        update_preview(result.image);
        set_report(format_report(request, result.inspection));
        set_status("Saved image to " + request.output_path, false);
    } catch (const std::exception& error) {
        clear_preview();
        set_status(error.what(), true);
    }
}

void MainWindow::set_status(const std::string& text, bool is_error) {
    gtk_label_set_text(GTK_LABEL(m_status_label), text.c_str());
    if (is_error) {
        gtk_widget_add_css_class(m_status_label, "error");
    } else {
        gtk_widget_remove_css_class(m_status_label, "error");
    }
}

void MainWindow::set_report(const std::string& text) {
    gtk_text_buffer_set_text(m_report_buffer, text.c_str(), static_cast<int>(text.size()));
}

void MainWindow::update_preview(const b2img::core::Image& image) {
    const auto bytes = make_rgb_bytes(image);
    const int viewport_width = std::max(1, gtk_widget_get_width(m_preview_scroll));
    const int viewport_height = std::max(1, gtk_widget_get_height(m_preview_scroll));
    const double fit_scale = std::min(
        static_cast<double>(viewport_width) / static_cast<double>(image.width()),
        static_cast<double>(viewport_height) / static_cast<double>(image.height())
    );
    const double base_scale = fit_scale > 0.0 ? fit_scale : 1.0;
    const double zoom_scale = gtk_adjustment_get_value(m_zoom_adjustment) / 100.0;
    const double final_scale = base_scale * zoom_scale;
    const auto scaled_width = std::max(1, static_cast<int>(static_cast<double>(image.width()) * final_scale));
    const auto scaled_height = std::max(1, static_cast<int>(static_cast<double>(image.height()) * final_scale));
    GBytes* pixel_bytes = g_bytes_new(bytes.data(), bytes.size());
    GdkTexture* texture = gdk_memory_texture_new(
        static_cast<int>(image.width()),
        static_cast<int>(image.height()),
        GDK_MEMORY_R8G8B8,
        pixel_bytes,
        image.width() * 3U
    );
    GtkSnapshot* snapshot = gtk_snapshot_new();
    graphene_rect_t bounds;
    graphene_rect_init(&bounds, 0.0F, 0.0F, static_cast<float>(scaled_width), static_cast<float>(scaled_height));
    gtk_snapshot_append_scaled_texture(snapshot, texture, GSK_SCALING_FILTER_NEAREST, &bounds);
    graphene_size_t paintable_size;
    graphene_size_init(&paintable_size, static_cast<float>(scaled_width), static_cast<float>(scaled_height));
    GdkPaintable* paintable = gtk_snapshot_free_to_paintable(snapshot, &paintable_size);

    gtk_picture_set_paintable(GTK_PICTURE(m_picture), paintable);
    gtk_widget_set_size_request(m_picture, scaled_width, scaled_height);
    g_object_unref(paintable);
    g_object_unref(texture);
    g_bytes_unref(pixel_bytes);
}

void MainWindow::on_file_dropped(GFile* file) {
    char* path = g_file_get_path(file);
    if (path == nullptr) {
        set_status("Dropped item is not a local file.", true);
        return;
    }

    gtk_editable_set_text(GTK_EDITABLE(m_input_entry), path);
    set_status("Input file dropped into the window.", false);
    g_free(path);
}

void MainWindow::on_open_clicked(GtkButton*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->choose_input_file();
}

void MainWindow::on_pick_output_clicked(GtkButton*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->choose_output_file();
}

void MainWindow::on_inspect_clicked(GtkButton*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->inspect();
}

void MainWindow::on_rotate_left_clicked(GtkButton*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->rotate_left();
}

void MainWindow::on_rotate_right_clicked(GtkButton*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->rotate_right();
}

void MainWindow::on_save_clicked(GtkButton*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->save();
}

void MainWindow::on_live_editable_changed(GtkEditable*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->schedule_preview();
}

void MainWindow::on_live_spin_changed(GtkSpinButton*, gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    self->m_syncing_dimension_controls = true;
    sync_dimension_controls(self->m_width_spin, self->m_width_scale, false, false);
    sync_dimension_controls(self->m_height_spin, self->m_height_scale, false, false);
    self->m_syncing_dimension_controls = false;
    self->schedule_preview();
}

void MainWindow::on_dimension_scale_changed(GtkRange* range, gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    if (self->m_syncing_dimension_controls) {
        return;
    }

    self->m_syncing_dimension_controls = true;
    if (GTK_WIDGET(range) == self->m_width_scale) {
        sync_dimension_controls(self->m_width_spin, self->m_width_scale, false, true);
    } else if (GTK_WIDGET(range) == self->m_height_scale) {
        sync_dimension_controls(self->m_height_spin, self->m_height_scale, false, true);
    }
    self->m_syncing_dimension_controls = false;
    self->schedule_preview();
}

void MainWindow::on_live_check_toggled(GtkCheckButton*, gpointer user_data) {
    static_cast<MainWindow*>(user_data)->schedule_preview();
}

void MainWindow::on_dropdown_changed(GObject*, GParamSpec*, gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    self->sync_control_state();
    self->schedule_preview();
}

void MainWindow::on_zoom_changed(GtkAdjustment*, gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    if (self->m_last_image.has_value()) {
        self->update_preview(*self->m_last_image);
    }
}

gboolean MainWindow::on_preview_timeout(gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    self->m_preview_timeout_id = 0U;
    self->run_live_preview();
    return G_SOURCE_REMOVE;
}

gboolean MainWindow::on_file_drop(GtkDropTarget*, const GValue* value, double, double, gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    if (G_VALUE_HOLDS(value, G_TYPE_FILE)) {
        auto* file = G_FILE(g_value_get_object(value));
        if (file == nullptr) {
            return FALSE;
        }

        self->on_file_dropped(file);
        return TRUE;
    }

    return FALSE;
}

void MainWindow::on_open_dialog_complete(GObject* source_object, GAsyncResult* result, gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    GtkFileDialog* dialog = GTK_FILE_DIALOG(source_object);
    GError* error = nullptr;
    GFile* file = gtk_file_dialog_open_finish(dialog, result, &error);

    if (error != nullptr) {
        if (!is_cancelled_error(error)) {
            self->set_status(error->message, true);
        }
        g_clear_error(&error);
        g_object_unref(dialog);
        return;
    }

    self->on_file_dropped(file);

    g_object_unref(file);
    g_object_unref(dialog);
}

void MainWindow::on_save_dialog_complete(GObject* source_object, GAsyncResult* result, gpointer user_data) {
    auto* self = static_cast<MainWindow*>(user_data);
    GtkFileDialog* dialog = GTK_FILE_DIALOG(source_object);
    GError* error = nullptr;
    GFile* file = gtk_file_dialog_save_finish(dialog, result, &error);

    if (error != nullptr) {
        if (!is_cancelled_error(error)) {
            self->set_status(error->message, true);
        }
        g_clear_error(&error);
        g_object_unref(dialog);
        return;
    }

    char* path = g_file_get_path(file);
    if (path != nullptr) {
        gtk_editable_set_text(GTK_EDITABLE(self->m_output_entry), path);
        g_free(path);
        self->set_status("Output path selected.", false);
    }

    g_object_unref(file);
    g_object_unref(dialog);
}

}  // namespace b2img::gui
