# RosetteLab Product Specifications

## 1. Purpose

RosetteLab is a standalone desktop application for composing mathematical drawings from multiple editable curve layers. It replaces the earlier extension-oriented workflow with an application designed around persistent parameters, live superposition, and reversible editing.

The application UI, file metadata keys, error messages, and user documentation are in English.

## 2. Product principles

- **Editable by construction:** generated curves retain the parameters that created them.
- **SVG-native:** the project file is a renderable SVG, not an opaque proprietary container.
- **Non-destructive composition:** layers can be added, edited, reordered, hidden, locked, duplicated, and removed independently.
- **Immediate visual feedback:** parameter changes update the composition preview.
- **Predictable reopening:** only SVG documents carrying supported RosetteLab metadata are opened as native projects.
- **Portable core:** curve generation and SVG serialization are independent of the GUI.

## 3. Main window

The main window contains three primary regions:

1. **Preview canvas**
   - Displays all visible layers in stack order.
   - Uses an opaque white page by default, independently of the operating-system UI theme.\n   - When the selected page background is transparent or translucent, the preview displays it over a checkerboard made of alternating white and 50% grey (`RGB 127, 127, 127`) squares.
   - Supports zoom, pan, fit-to-document, and a configurable background.
   - Shows horizontal and vertical scroll bars automatically whenever the zoomed page exceeds the preview viewport.
   - Reflects parameter and appearance changes without requiring an Apply action.

2. **Parameter editor**
   - Displays controls for the selected layer's curve family.
   - Includes curve geometry, drawing, color, opacity, and compositing controls.
   - Provides named presets whose values populate editable controls.
   - Any subsequent edit is permitted and does not destroy the preset's starting values.\n   - The complete editor is vertically scrollable whenever its controls exceed the available window height. Switching curve families must not resize the main window.

3. **Layer stack**
   - Shows one row per curve.
   - Supports drag-and-drop reordering.
   - Recalls the selected layer's parameters in the editor.
   - Each layer row is ordered as: visibility glyph, lock glyph, visual layer thumbnail, then layer name.\n   - The thumbnail has the same square dimensions as the visibility and lock controls and previews the layer's current geometry and appearance. Its background reproduces the document background: an opaque document background is shown as a solid color, while a transparent or translucent document background is composited over the standard alternating white and 50% grey (`RGB 127, 127, 127`) checkerboard. Stroke width is reduced proportionally to the thumbnail geometry scale, with a small documented minimum that keeps thin strokes visible and a cap that prevents thick strokes from obscuring the thumbnail. It updates when curve parameters, appearance parameters, or the document background change.
   - The visibility control uses clickable open-eye and closed-eye UTF-8 glyphs instead of a checkbox.
   - The lock control uses clickable unlocked and locked UTF-8 padlock glyphs instead of a separate Lock/Unlock button.
   - Both glyph controls have stable dimensions, tooltips, keyboard access, and accessible names; changing state must never resize the layer panel.
   - Supports add, duplicate, rename, and delete operations.
   - The primary `Add…` command opens a curve-type selector rather than creating a predetermined family directly.
   - The selector lists Polar rose, Ellipse, Hypotrochoid, Epitrochoid, Lissajous, Harmonograph, and Spirograph; only implemented families are enabled.
   - Creating a layer prompts for its name, prefilled as `Curve type N`, where the type is the English curve-family name and (N) is the next number for that family (for example, `Polar rose 1`, `Polar rose 2`, or `Lissajous 1`).
   - Default names do not change when mathematical parameters change.
   - A user-defined name remains unchanged until explicitly renamed.\n   - A name wider than the available row width is elided with a trailing ellipsis. Hovering the elided name displays its complete value in a tooltip.

The layout must remain usable on laptop-sized displays. Resizable panes and sensible minimum sizes are required. The application restores the last main-window size, position, and maximized state on the next launch; Qt must keep a restored window reachable when the previous screen arrangement is no longer available.

## 4. Layer model

Every curve layer has:

- stable identifier;
- user-visible name;
- curve family and family-specific parameters;
- layer order;
- visibility state;
- lock state;
- geometric transform;
- stroke style;
- fill style;
- fill rule;
- layer opacity;
- blend mode;
- optional preset provenance;
- schema version.

### 4.1 Visibility

A clickable open-eye or closed-eye UTF-8 glyph changes whether the layer is rendered. It is the first control in the layer row. Visibility is stored in the project SVG.

### 4.2 Locking

A clickable unlocked or locked UTF-8 padlock glyph is the second control in the layer row. It prevents accidental changes to curve, transform, and appearance parameters. Locked layers remain selectable and inspectable. Unlocking is always available. No separate Lock/Unlock button is displayed below the layer list.

### 4.3 Reordering

Layer rows are reorderable by drag and drop. The topmost painted layer is shown at the top of the list and the bottommost painted layer at the bottom. Internally, SVG elements remain serialized in standard paint order from bottom to top, while the UI presents the exact visual inverse. Drag-and-drop changes must keep both representations consistent.

## 5. Curve families

### 5.1 Polar rose

Canonical form:

[
r(\theta) = a \cos(k\theta + \phi)
]

Initial parameters:

- radius/amplitude (a);
- k representation:
  - **Decimal**, with a real-valued k, traced over one angular turn and left open unless it closes exactly;
  - **Fraction**, with non-zero integer numerator n and denominator d, where k = n/d and the complete rational closing period is calculated exactly;
- decimal k, or fractional n and d;
- phase (phi);
- angular rotation;
- adaptive Bézier tolerance in document units.

The mathematical meaning of k, including the odd/even petal behavior, must be documented in the interface help. Decimal approximations such as 0.33 must never be silently interpreted as the exact fraction 1/3. RosetteLab must not forcibly close an incomplete decimal trace.

### 5.2 Ellipse

Canonical form:

\[
x(t)=r_x\cos(t),\qquad y(t)=r_y\sin(t)
\]

The curve may then be rotated by an angle \(\alpha\) around its centre.

Parameters:

- horizontal radius \(r_x\);
- vertical radius \(r_y\);
- angular rotation;
- adaptive Bézier tolerance in document units.

Ellipses are represented preferentially as cubic Bézier arcs, use the common layer appearance controls, and preserve all parameters in native RosetteLab SVG metadata.

### 5.3 Trochoid

The family selector contains:

- Hypotrochoid
- Epitrochoid

Hypotrochoid:

[
x(t)=(R-r)\cos t+d\cos\left(\frac{R-r}{r}t\right)
]

[
y(t)=(R-r)\sin t-d\sin\left(\frac{R-r}{r}t\right)
]

Epitrochoid:

[
x(t)=(R+r)\cos t-d\cos\left(\frac{R+r}{r}t\right)
]

[
y(t)=(R+r)\sin t-d\sin\left(\frac{R+r}{r}t\right)
]

Parameters:

- fixed radius or tooth count (R);
- rolling radius or tooth count (r);
- pen offset (d);
- complete mathematically closed trace or limited turns;
- number of turns around the fixed gear;
- optional forced closure of a limited trace, represented by the SVG closing segment;
- angular rotation;
- adaptive Bézier tolerance in document units.

For a complete trace, RosetteLab resolves the rational ratio `R/r` and traces the denominator number of revolutions required for exact closure. Ratios whose reduced denominator would exceed the supported safety bound must use limited tracing or produce a clear validation error.

### 5.4 Lissajous

Canonical form:

[
x(t)=A_x\sin(f_x t+\phi_x),\qquad
y(t)=A_y\sin(f_y t+\phi_y)
]

Parameters include both amplitudes, both frequencies, phases, duration, and precision.

### 5.5 Harmonograph

The initial model uses damped oscillations on both axes:

[
x(t)=A_x\sin(f_x t+\phi_x)e^{-d_x t}
]

[
y(t)=A_y\sin(f_y t+\phi_y)e^{-d_y t}
]

Parameters include amplitudes, frequencies, phases, damping factors, duration, and precision.

More complete multi-pendulum models are explicitly deferred.

### 5.6 Spirograph

This mode exposes terms familiar from a physical Spirograph rather than requiring abstract radii:

- fixed ring or wheel tooth count;
- rolling wheel tooth count;
- inside/outside rolling;
- numbered pen hole;
- hole radial position or a wheel-specific hole mapping;
- number of revolutions;
- complete or limited trace;
- drawing scale and rotation.

The corresponding equation and resolved mathematical parameters are visible to the user. Initial releases may use a generic hole-distance model. Exact commercial wheel datasets require independently verifiable measurements and are deferred.

## 6. Presets

Each curve family offers a curated set of mathematically notable and visually distinctive presets.

Requirements:

- selecting a preset writes its values into the actual editable fields;
- editing a field keeps the layer editable and marks the state as modified/custom;
- a reset action restores the selected preset;
- presets may include geometry, copies, rotations, and scale progressions;
- user-selected colors and stroke widths are not overwritten unless a preset explicitly declares an appearance theme and the user opts into it.

Custom user preset storage is deferred.

## 7. Curve representation and rendering

### 7.1 Bézier-first policy

Whenever mathematically and geometrically appropriate, RosetteLab represents generated curves as sequences of cubic Bézier segments rather than dense polylines.

Requirements:

- curve generators provide positions and analytical derivatives when practical;
- an adaptive fitter subdivides a segment until its measured deviation from the mathematical curve is below the selected tolerance;
- tolerance is expressed in document units and is independent of preview zoom;
- generated SVG uses cubic path commands (`C`) preferentially;
- closed mathematical curves produce topologically closed SVG paths;
- inflection points, cusps, discontinuities, and singularities are split explicitly;
- point-count and segment-count safety limits remain enforced;
- the preview and exported SVG must derive from the same fitted geometry;
- tests compare fitted Bézier paths against reference samples from the original equation.

Polyline output is permitted only when a Bézier approximation is unavailable, would be less faithful, is explicitly requested for a specialized export, or is used internally as a temporary reference for validation. It must not be the normal representation of supported smooth curve families.

User-facing precision presets map to documented geometric tolerances, with an optional custom tolerance.

## 8. Drawing and appearance

### 8.1 Stroke

- default color: opaque black;
- color;
- opacity from 0 to 100%;
- width;
- line cap and join when relevant.

### 8.2 Fill

- none or color;
- opacity from 0 to 100%;
- fill rule: `nonzero` or `evenodd`.

### 8.3 Color input

Three synchronized representations are required:

- RGBA;
- HSLA;
- web hexadecimal as `#RRGGBB` or `#RRGGBBAA`, displayed below the numeric color channels.

The hexadecimal field supports typing and clipboard copy/paste. A missing leading `#` is inserted automatically, and a six-digit RGB value is normalized to eight digits by appending the opaque alpha component `FF`. Whenever a complete valid hexadecimal value changes, the RGBA or HSLA channels and the visual preview update immediately; edits made through numeric channels or the graphical picker update the hexadecimal field in return.

A graphical picker may be added where the platform toolkit supports it consistently. Numeric entry must remain available. The persistent Background, Stroke color, and Fill color controls in the settings panel, as well as the color-preview strip at the bottom of every corresponding selector, display transparent or translucent colors over a checkerboard made of alternating white and 50% grey (`RGB 127, 127, 127`) squares. Its `#RRGGBBAA - Choose visually...` label uses white text on a dark composited preview and black text on a light composited preview, taking alpha into account.

### 8.4 Layer rendering

Each layer has an opacity percentage and a blend mode. The first supported set should map directly to SVG/CSS compositing:

- Normal
- Multiply
- Screen
- Overlay
- Darken
- Lighten
- Color Dodge
- Color Burn
- Hard Light
- Soft Light
- Difference
- Exclusion
- Hue
- Saturation
- Color
- Luminosity

Unsupported backend modes must fail gracefully or be disabled rather than silently rendered incorrectly.

## 9. Superposition and copies

A layer may represent one curve or a generated group of related copies.

Possible parameters:

- number of copies;
- angular offset between copies;
- scale progression;
- optional parameter progression;
- optional color progression.

Version 1.0 requires copy count, angular offset, and scale progression. General parameter and color sequences are deferred.

## 10. SVG project format

### 10.1 Native project detection

RosetteLab opens a file as a project only when all of the following are present and supported:

- root SVG element;
- RosetteLab namespace declaration;
- RosetteLab document metadata;
- supported schema version;
- reconstructable layer metadata.

A normal SVG without these elements is not opened as a RosetteLab project. A clear message explains why. General SVG import is a later feature.

### 10.2 Compatibility

The file remains a valid SVG that other SVG applications can render. RosetteLab metadata must not be required to display the generated paths.

Suggested namespace:

`https://rosettelab.app/ns/1`

Until a stable public namespace is established, schema identifiers must be versioned and centralized in code.

### 10.3 Stored information

The SVG stores:

- document schema and application version;
- canvas size, view box, and background;
- ordered curve layers;
- curve family and parameters;
- generated path data;
- transform and appearance;
- visibility and lock state;
- blend mode and opacity;
- preset provenance when applicable.

### 10.4 Round-trip requirement

Saving, reopening, and saving without edits must preserve the visible composition and all supported RosetteLab parameters. Unknown metadata from a newer schema must not be discarded silently.

### 10.5 Export

The **File → Export** submenu provides:

- **To PNG…**: raster export at a user-selected resolution from 72 to 1200 DPI; document alpha is preserved and the UI checkerboard is never exported;
- **To JPEG…**: raster export at a user-selected resolution from 72 to 1200 DPI and high quality; transparent document areas are composited onto opaque white because JPEG has no alpha channel; the filename extension is always normalized to `.jpg`, including when `.jpeg` was entered;
- **To PDF…**: export at the exact document dimensions. Documents using only Normal compositing retain vector cubic Bézier paths, fills, strokes, and opacity. Because Qt's PDF paint engine does not reliably preserve layer blend modes, a document containing any visible non-Normal blend mode is precomposed at 300 DPI and embedded as a raster page so that the PDF matches the preview. On Qt 6.8 and later, a modal choice offers RGB or CMYK before the save dialog. RGB is the default and embeds Qt's default sRGB output intent. CMYK asks Qt to convert the RGB source colors to CMYK and does not attach the sRGB output intent. Older Qt versions retain their legacy PDF color behavior because this selection API is unavailable.

Preview and all export formats use the shared document renderer and the same fitted curve geometry. Raster export rejects dimensions above 32,767 pixels per side or 100 million pixels in total. Clean SVG without RosetteLab editing metadata remains a planned export target.

## 11. Safety and validation

- Numeric controls have documented ranges and reject non-finite values.
- Estimated point counts are bounded before generation.
- Excessively complex layers show a warning instead of freezing the UI.
- Invalid metadata never causes arbitrary code execution.
- XML parsing prohibits external entity expansion and external resource loading.
- Autosave and crash recovery are desirable but deferred from the first milestone.

## 12. Accessibility and platform behavior

- Keyboard navigation for all controls.
- Accessible labels for eye, lock, layer, and color controls.
- Host theme integration, including dark mode through Qt's platform integration.
- The main window should open in front when launched normally, without forcing permanent always-on-top behavior.
- Primary target: macOS.
- Secondary targets: Linux and Windows, subject to Qt packaging validation.

Platform-specific native widgets should be avoided unless isolated behind an abstraction.

## 13. Version scope

### 13.1 Milestone 0.1 — technical foundation

- C++ project and build system;
- Qt 6 Widgets application shell;
- basic three-pane layout;
- document and layer data model;
- polar rose and ellipse generators;
- live preview;
- unit tests for curve generation.

### 13.2 Milestone 0.2 — editable composition

- layer add/select/rename/duplicate/delete;
- visibility and lock controls;
- drag-and-drop reordering;
- stroke, fill, opacity, fill rule, and essential blend modes;
- presets that populate editable controls.

### 13.3 Milestone 0.3 — native SVG round-trip

- versioned RosetteLab metadata schema;
- project save;
- strict native-project opening;
- round-trip tests;
- clean SVG export.

### 13.4 Milestone 0.4 — core curve set

- hypotrochoid and epitrochoid;
- Lissajous;
- simple harmonograph;
- limited/complete trochoid tracing;
- per-family curated presets.

### 13.5 Milestone 1.0 — first stable release

- Spirograph mode with generic wheel/hole mapping;
- copy-based superposition;
- PNG and JPEG export;
- vector PDF export;
- undo/redo for document edits;
- user documentation;
- macOS application package;
- tested project migration policy for the 1.x schema.

## 14. Deferred features

The following may wait until after 1.0 unless implementation proves inexpensive:

- full multi-pendulum harmonograph;
- exact branded Spirograph wheel and hole database;
- arbitrary SVG import as editable geometry;
- user-created preset libraries;
- gradients and patterns;
- masks, clipping, and filters;
- animation;
- scripting or plugin API;
- cloud synchronization;
- collaborative editing;

- autosave and crash recovery;
- advanced color-management workflows;
- GPU-specific renderer.

## 15. Acceptance criteria for 1.0

RosetteLab 1.0 is acceptable when a user can:

1. Create a multi-layer composition containing every core curve family.
2. Select any unlocked layer and recover all of its editable parameters.
3. Reorder, hide, lock, duplicate, and delete layers.
4. Use stroke, fill, alpha, fill rules, opacity, and supported blend modes.
5. Save the work as an SVG that renders outside RosetteLab.
6. Reopen that RosetteLab SVG and continue editing without loss.
7. Export a clean SVG and PNG.
8. Complete the workflow in English on a supported macOS system without Inkscape.
