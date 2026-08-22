## Additions
- Added gamepad camera controls
  - While holding `R`, use the Left and Right sticks to move/rotate, `A`/`B` to ascend/descend, `L` to recenter
  - Works flawlessly on Steam Deck/Controller
- Gamepads: `L` and `R` now act as an alternate left/right click when hovering ImGui elements
  - For usage with Steam Deck and Controller (you may need to add Pluto to Steam for this to work)
- Added save CC to model

## Bug Fixes
- Fixed inaccessible DJUI inputs
- Near-clip now properly tucks accounts for linked scale
- Fixed some ImGui elements flickering under threading (i.e. CC/GameShark Box)
- Fixed animation frames offsetting by 1 index