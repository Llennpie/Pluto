## Additions
- Added "Center to Avatar" camera feature
  - Recenters the camera on keybind (default V)
  - Toggle option for orbit-style camera, works seamlessly with keyframes
- Added gamepad camera controls
  - While holding `R`, use the Left and Right sticks to move/rotate, `A`/`B` to ascend/descend, `L` to recenter
  - Works flawlessly on Steam Deck/Controller
- Added rotation to camera settings (and other small QOL)
- Added in-game HUD options (enabled, disabled, hidden in freeze cam)
- Added option to save color codes to models
- Re-implemented tripoint filtering
- Menu windows are now off by default (reduce clutter)
- Gamepads: `L` and `R` now act as an alternate left/right click when hovering ImGui elements
  - For usage with Steam Deck and Controller (you may need to add Pluto to Steam for this to work)

## Bug Fixes
- Error models (missing IDs) are now hidden in Freeze Camera, temporary workaround to Yoshi's issues on Star Road.
- Near-clip now properly tucks under the cutout and transparent layers, and accounts for linked scale
- Fixed c-up force-exiting in the Textbox Editor
  - Free Camera c-up no longer crashes the game when opening a textbox
- PAnim bone translation and scale no longer fall out of range from un-animated wiggle and extra bones
- Check for NULL pointer on selected mixtape animation
- Fixed text input occasionally not registering
- Fixed some ImGui elements flickering under threading (i.e. CC/GameShark Box)