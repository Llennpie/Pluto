#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "src/pc/configfile.h"

void djui_panel_controls_pluto_create(struct DjuiBase* caller) {
    f32 bindBodyHeight = 28 * 12 + 1 * 10;

    struct DjuiThreePanel* panel = djui_panel_menu_create(DLANG(CONTROLS, CONTROLS), false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    {
        struct DjuiFlowLayout* bindBody = djui_flow_layout_create(body);
        djui_base_set_size_type(&bindBody->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
        djui_base_set_size(&bindBody->base, 1.0f, bindBodyHeight);
        djui_base_set_color(&bindBody->base, 0, 0, 0, 0);
        djui_flow_layout_set_margin(bindBody, 1);
        {
            djui_bind_create(&bindBody->base, "Open Menu", configKeyPlutoMenu);
            djui_bind_create(&bindBody->base, "Screenshot", configKeyPlutoScreenshot);
            djui_bind_create(&bindBody->base, "Chroma Key", configKeyPlutoChroma);
            djui_bind_create(&bindBody->base, "Freeze Camera", configKeyPlutoFreezeCamera);
            djui_bind_create(&bindBody->base, "Toggle HUD", configKeyPlutoHud);
            djui_bind_create(&bindBody->base, "Play Anim", configKeyPlutoPlayAnim);
            djui_bind_create(&bindBody->base, "Pause Anim", configKeyPlutoPauseAnim);
        }

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
