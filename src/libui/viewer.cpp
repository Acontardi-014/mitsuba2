#include <mitsuba/ui/viewer.h>
#include <mitsuba/ui/texture.h>
#include <mitsuba/core/bitmap.h>

#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/popupbutton.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/tabwidget.h>
#include <nanogui/imageview.h>
#include <nanogui/progressbar.h>
#include <nanogui/opengl.h>

#include "libui_resources.h"

NAMESPACE_BEGIN(mitsuba)

MitsubaViewer::MitsubaViewer()
    : ng::Screen(ng::Vector2i(1024, 768), "Mitsuba 2",
                      /* resizable */ true, /* fullscreen */ false,
                      /* depth_buffer */ true, /* stencil_buffer */ true,
                      /* float_buffer */ true) {
    using namespace nanogui;

    inc_ref();
    m_contents = new Widget(this);

    AdvancedGridLayout *layout = new AdvancedGridLayout({30, 0}, {50, 5, 0}, 5);
    layout->set_row_stretch(0, 1);
    layout->set_col_stretch(1, 1);

    m_contents->set_layout(layout);
    m_contents->set_size(m_size);

    Widget *tools = new Widget(m_contents);
    tools->set_layout(new BoxLayout(Orientation::Vertical,
                                    Alignment::Middle, 0, 6));

    m_btn_menu = new PopupButton(tools, "", ENTYPO_ICON_MENU);
    Popup *menu = m_btn_menu->popup();

    menu->set_layout(new GroupLayout());
    menu->set_visible(true);
    menu->set_size(ng::Vector2i(200, 140));
    new Button(menu, "Open ..", ENTYPO_ICON_FOLDER);
    PopupButton *recent = new PopupButton(menu, "Open Recent");
    auto recent_popup = recent->popup();
    recent_popup->set_layout(new GroupLayout());
    new Button(recent_popup, "scene1.xml");
    new Button(recent_popup, "scene2.xml");
    new Button(recent_popup, "scene3.xml");
    new Button(menu, "Export image ..", ENTYPO_ICON_EXPORT);
    Button *about = new Button(menu, "About", ENTYPO_ICON_HELP_WITH_CIRCLE);
    about->set_callback([&]() {
        auto dlg = new MessageDialog(
            this, MessageDialog::Type::Information, "About Mitsuba 2",
            "Mitsuba 2 is freely available under a BSD-style license. "
            "If you use renderings created with this software, we kindly "
            "request that you acknowledge this and link to the project page at\n\n"
            "\thttp://www.mitsuba-renderer.org/\n\n"
            "In the context of scientific articles or books, please cite paper\n\n"
            "Mitsuba 2: A Retargetable Rendering System\n"
            "Merlin Nimier-David, Delio Vicini, Tizian Zeltner, and Wenzel Jakob\n"
            "In ...\n");
        dlg->message_label()->set_fixed_width(550);
        dlg->message_label()->set_font_size(20);
        perform_layout(nvg_context());
        dlg->center();
    });

    m_btn_play = new Button(tools, "", ENTYPO_ICON_CONTROLLER_PLAY);
    m_btn_play->set_text_color(nanogui::Color(100, 255, 100, 150));
    m_btn_play->set_callback([this]() {
            m_btn_play->set_icon(ENTYPO_ICON_CONTROLLER_PAUS);
            m_btn_play->set_text_color(nanogui::Color(255, 255, 255, 150));
            m_btn_stop->set_enabled(true);
        }
    );
    m_btn_play->set_tooltip("Render");

    m_btn_stop = new Button(tools, "", ENTYPO_ICON_CONTROLLER_STOP);
    m_btn_stop->set_text_color(nanogui::Color(255, 100, 100, 150));
    m_btn_stop->set_enabled(false);
    m_btn_stop->set_tooltip("Stop rendering");

    m_btn_reload = new Button(tools, "", ENTYPO_ICON_CYCLE);
    m_btn_reload->set_tooltip("Reload file");

    m_btn_settings = new PopupButton(tools, "", ENTYPO_ICON_COG);
    m_btn_settings->set_tooltip("Scene configuration");

    auto settings_popup = m_btn_settings->popup();
    AdvancedGridLayout *settings_layout =
        new AdvancedGridLayout({ 30, 0, 15, 50 }, { 0, 5, 0, 5, 0, 5, 0 }, 5);
    settings_popup->set_layout(settings_layout);
    settings_layout->set_col_stretch(0, 1);
    settings_layout->set_col_stretch(1, 1);
    settings_layout->set_col_stretch(2, 1);
    settings_layout->set_col_stretch(3, 10);
    settings_layout->set_row_stretch(1, 1);
    settings_layout->set_row_stretch(5, 1);

    using Anchor = nanogui::AdvancedGridLayout::Anchor;
    settings_layout->set_anchor(new Label(settings_popup, "Integrator", "sans-bold"),
                                Anchor(0, 0, 4, 1));
    settings_layout->set_anchor(new Label(settings_popup, "Max depth"), Anchor(1, 1));
    settings_layout->set_anchor(new Label(settings_popup, "Sampler", "sans-bold"),
                                Anchor(0, 4, 4, 1));
    settings_layout->set_anchor(new Label(settings_popup, "Sample count"), Anchor(1, 5));
    IntBox<uint32_t> *ib1 = new IntBox<uint32_t>(settings_popup);
    IntBox<uint32_t> *ib2 = new IntBox<uint32_t>(settings_popup);
    ib1->set_editable(true);
    ib2->set_editable(true);
    ib1->set_alignment(TextBox::Alignment::Right);
    ib2->set_alignment(TextBox::Alignment::Right);
    ib1->set_fixed_height(25);
    ib2->set_fixed_height(25);
    settings_layout->set_anchor(ib1, Anchor(3, 1));
    settings_layout->set_anchor(ib2, Anchor(3, 5));
    settings_popup->set_size(ng::Vector2i(0, 0));
    settings_popup->set_size(settings_popup->preferred_size(nvg_context()));

    for (Button *b : { (Button *) m_btn_menu.get(), m_btn_play.get(),
                       m_btn_stop.get(), m_btn_reload.get(),
                       (Button *) m_btn_settings.get() }) {
        b->set_fixed_size(ng::Vector2i(25, 25));
        PopupButton *pb = dynamic_cast<PopupButton *>(b);
        if (pb) {
            pb->set_chevron_icon(0);
            pb->popup()->set_anchor_offset(12);
            pb->popup()->set_anchor_size(12);
        }
    }

    TabWidget *tab = new TabWidget(m_contents);
    m_view = new ImageView(tab);
    m_view->set_draw_border(false);

    mitsuba::ref<Bitmap> bitmap;// = new Bitmap(new MemoryStream(mitsuba_logo_png, mitsuba_logo_png_size));

    m_view->set_pixel_callback(
        [bitmap](const Vector2i &pos, char **out, size_t out_size) {
            const uint8_t *data = (const uint8_t *) bitmap->data();
            for (int i = 0; i < 4; ++i)
                snprintf(out[i], out_size, "%.3g",
                         (double) data[4 * (pos.x() + pos.y() * bitmap->size().x()) + i]);
        }
    );

    //m_view->set_image(new mitsuba::Texture(bitmap,
                                           //mitsuba::Texture::InterpolationMode::Trilinear,
                                           //mitsuba::Texture::InterpolationMode::Nearest));

    tab->append_tab("cbox.xml", m_view);
    tab->append_tab("matpreview.xml", m_view);
    tab->append_tab("hydra.xml", m_view);
    tab->set_tabs_closeable(true);
    tab->set_tabs_draggable(true);
    tab->set_remove_children(false);
    tab->set_padding(1);

    //view->set_texture_id(nvgImageIcon(nvg_context(), mitsuba_logo));

    //ImageView *view = m_view;
    //tab->set_popup_callback([tab, view](int id, Screen *screen) -> Popup * {
        //Popup *popup = new Popup(screen);
        //Button *b1 = new Button(popup, "Close", ENTYPO_ICON_CROSS);
        //Button *b2 = new Button(popup, "Duplicate", ENTYPO_ICON_PLUS);
        //b1->set_callback([tab, id]() { tab->remove_tab(id); });
        //b2->set_callback([tab, id, view]() { tab->insert_tab(tab->tab_index(id), tab->tab_caption(id), view); });
        //return popup;
    //});

    layout->set_anchor(tools, Anchor(0, 0, Alignment::Minimum, Alignment::Minimum));
    layout->set_anchor(tab, Anchor(1, 0, Alignment::Fill, Alignment::Fill));

    m_progress_panel = new Widget(m_contents);
    layout->set_anchor(m_progress_panel, Anchor(1, 2, Alignment::Fill, Alignment::Fill));

    Label *label1 = new Label(m_progress_panel, "Rendering:", "sans-bold");
    Label *label2 = new Label(m_progress_panel, "30% (ETA: 0.2s)");
    m_progress_bar = new ProgressBar(m_progress_panel);
    m_progress_bar->set_value(.3f);

    AdvancedGridLayout *progress_layout = new AdvancedGridLayout({0, 5, 0, 10, 0}, {0}, 0);
    progress_layout->set_col_stretch(4, 1);
    m_progress_panel->set_layout(progress_layout);
    progress_layout->set_anchor(label1, Anchor(0, 0));
    progress_layout->set_anchor(label2, Anchor(2, 0));
    progress_layout->set_anchor(m_progress_bar, Anchor(4, 0));

    Screen::set_resize_callback([this](const Vector2i &size) {
        m_progress_panel->set_size(Vector2i(0, 0));
        m_view->set_size(Vector2i(0, 0));
        m_contents->set_size(size);
        perform_layout();
    });

    perform_layout();
    m_view->reset();

    m_redraw = true;
    draw_all();
}

bool MitsubaViewer::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        set_visible(false);
        return true;
    }
    return false;
}

NAMESPACE_END(mitsuba)
