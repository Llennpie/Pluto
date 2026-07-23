## Additions
- Added "Center to Avatar" camera feature
  - Recenters the camera on keybind (default V)
  - Toggle option for orbit-style camera, works seamlessly with keyframes
- Added rotation to camera settings (and other small QOL)
- Added in-game HUD options (enabled, disabled, hidden in freeze cam)
- Re-implemented tripoint filtering
- Raised near-clip distance to 15 world units
- Menu windows are now off by default (reduce clutter)

## Bug Fixes
- Error models (missing IDs) are now hidden in Freeze Camera, temporary workaround to Yoshi's issues on Star Road.
- Near-clip now properly tucks under the cutout and transparent layers
- Fixed c-up force-exiting in the Textbox Editor
  - Free Camera c-up no longer crashes the game when opening a textbox
- PAnim bone translation and scale no longer fall out of range from un-animated wiggle and extra bones
- Check for NULL pointer on selected mixtape animation
- Fixed text input occasionally not registering