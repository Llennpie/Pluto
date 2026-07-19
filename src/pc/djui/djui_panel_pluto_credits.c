#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "djui_panel_changelog.h"
#include "pc/lua/utils/smlua_misc_utils.h"

static char sInfo[1024];
extern ALIGNED8 u8 texture_coopdx_logo[];

void djui_panel_pluto_credits_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create("");
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    {
        struct DjuiImage* logo = djui_image_create(body, texture_coopdx_logo, 2048, 1024, 32);
            djui_base_set_size(&logo->base, 370.0f, 185.0f);
            djui_base_set_alignment(&logo->base, DJUI_HALIGN_CENTER, DJUI_VALIGN_TOP);
            djui_base_set_location_type(&logo->base, DJUI_SVT_RELATIVE, DJUI_SVT_ABSOLUTE);
            djui_base_set_location(&logo->base, 0, -50.0f);

        snprintf(sInfo, 1024, "Pluto is an all-in-one machinima studio, forked from \
sm64coopdx 0.2 by the Coop Deluxe team. \n\
\n\
Created by Llennpie (sm64rise) \n\
Co-Developed by fgetcat \n\
\n\
Menu Music \"Panimotion\" by yellow64 \n\
Default Expressions by FluffaMario \n\
Default Animations by SNG1 \n\
\"Pan\" Model by SuperG64 \n\
\n\
Special Thanks to the Coop Deluxe Team and the sm64coopdx Community"
        );

        struct DjuiText* text = djui_text_create(body, sInfo);
        djui_base_set_location(&text->base, 0, 0);
        djui_base_set_size(&text->base, (DJUI_DEFAULT_PANEL_WIDTH * (configDjuiThemeCenter ? DJUI_THEME_CENTERED_WIDTH : 1)) - 64, 480);
        djui_base_set_color(&text->base, 220, 220, 220, 255);
        djui_text_set_drop_shadow(text, 64, 64, 64, 100);
        djui_text_set_alignment(text, DJUI_HALIGN_CENTER, DJUI_VALIGN_TOP);

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
