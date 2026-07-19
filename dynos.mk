# ----------------------
# Dynamic Options System
# ----------------------

DYNOS_APPDATA_DIR := $(APPDATA)/Llennpie/Pluto
DYNOS_INPUT_DIR := ./dynos
DYNOS_OUTPUT_DIR := $(DYNOS_APPDATA_DIR)/dynos
DYNOS_PACKS_DIR := $(DYNOS_APPDATA_DIR)/dynos/packs
DYNOS_INIT := \
    mkdir -p "$(DYNOS_INPUT_DIR)"; \
    mkdir -p "$(DYNOS_OUTPUT_DIR)"; \
    mkdir -p "$(DYNOS_PACKS_DIR)"; \
    cp -f -r "$(DYNOS_INPUT_DIR)" "$(DYNOS_APPDATA_DIR)" 2>/dev/null || true ; \
    cp -f "sound/Panimotion.mp3" "$(DYNOS_APPDATA_DIR)/Panimotion.mp3" 2>/dev/null || true ; \
    \
    cp -f "actors/mario/mario_eyes_center.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/eyes_open.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_eyes_half_closed.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/eyes_half.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_eyes_closed.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/eyes_closed.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_eyes_dead.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/eyes_dead.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_eyes_left_unused.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/saturn_eye_custom.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_overalls_button.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/saturn_buttons_creator.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_mustache.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/saturn_mustache_creator.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_sideburn.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/saturn_sideburn_mario.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_logo.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/saturn_cap_mario.rgba32.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_metal.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/metal.rgba16.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_wing.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/wing_1.rgba16.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_wing_tip.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/wing_2.rgba16.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_wing.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/mario_wing_rgba16_png.rgba16.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_wing_tip.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/mario/mario_wing_tip_rgba16_png.rgba16.png" 2>/dev/null || true ; \
    \
    cp -f "actors/mario/mario_overalls_button.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/expressions/buttons/0-default.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_logo.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/expressions/cap/mario.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_mustache.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/expressions/mustache/0-default.png" 2>/dev/null || true ; \
    cp -f "actors/mario/mario_sideburn.rgba16.png" "$(DYNOS_PACKS_DIR)/Pluto Mario/expressions/sideburn/0-default.png" 2>/dev/null || true ; \

DYNOS_DO := $(shell $(call DYNOS_INIT))
INCLUDE_CFLAGS += -DDYNOS
