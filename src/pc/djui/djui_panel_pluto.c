#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "pc/gfx/gfx_window_manager_api.h"
#include "pc/pc_main.h"
#include "pc/utils/misc.h"
#include "pc/configfile.h"

#define MSAA_ORIGINAL_UNSET ((u32)-1)

static struct DjuiInputbox* sFrameLimitInput = NULL;
static struct DjuiSelectionbox* sInterpolationSelectionBox = NULL;
static struct DjuiText* sRestartText = NULL;
static u32 sMsaaSelection = 0;
static u32 sMsaaOriginal = MSAA_ORIGINAL_UNSET;

static void djui_panel_pluto_apply(UNUSED struct DjuiBase* caller) {
    configWindow.settings_changed = true;
}

void djui_panel_pluto_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create("PLUTO");
    struct DjuiBase* body = djui_three_panel_get_body(panel);

    djui_checkbox_create(body, "Hide UI from OBS", &configWindow.secret_ui, djui_panel_pluto_apply);
    djui_checkbox_create(body, "Auto Reload Models", &configAutoReloadModels, djui_panel_pluto_apply);

    djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    djui_panel_add(caller, panel, NULL);
}
