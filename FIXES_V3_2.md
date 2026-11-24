# DHMT AEON – Version 3.2 targeted fixes

Date: 2026-11-18

## 1. B1 ceiling occlusion

**Cause:** `SceneRenderer::drawShell()` drew the ceiling slab on every frame, including when the user switched from the exterior to B1. The camera then looked at the roof slab rather than the parking layout.

**Fix:** The ceiling slab is drawn only while the exterior view is active. In B1 mode the slab is omitted, producing an open-top cutaway. The B1 floor slab and perimeter walls remain.

## 2. Camera mode transition

**Cause:** Pressing `V` changed only `targetY`; the exterior yaw, pitch and 88 m camera distance were retained for B1.

**Fix:** `Camera` now exposes `focusExterior()` and `focusBasement()`. Pressing `V` applies the corresponding camera preset. The B1 preset uses its own assumed yaw, pitch, distance and top-view height, all defined in `aeon_layout.h`. Switching modes exits top view so the expected three-quarter framing is restored.

## 3. HUD text outside the exterior panel

**Cause:** The exterior HUD panel height is 142 px, but the text-position lambda used `HUD_HEIGHT_PX` (322 px). Exterior text was therefore drawn above the visible panel and outside the viewport.

**Fix:** Text Y positions are now based on the active `panelHeight`, so the same layout logic works for both exterior and B1 panels.

## Updated controls

- `V`: switch exterior/B1 and reset to that scene's camera preset.
- `T`: toggle the top-down view; the top-view height follows the currently selected scene.
- Left mouse drag: orbit the current camera.
- Mouse wheel: zoom.

## Verification and delivery note

- Source-level checks confirm the three defects' code paths have been corrected.
- A new Windows build and visual runtime screenshots are still required.
- The prior `.exe` and cached `build` folder were intentionally omitted from this source archive because they were compiled before these changes. Rebuild with the instructions in `README_AEON.md`.
