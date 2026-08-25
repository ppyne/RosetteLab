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
   - Uses an opaque white page by default, independently of the operating-system UI theme.
   - Supports zoom, pan, fit-to-document, and a configurable background.
   - Shows horizontal and vertical scroll bars automatically whenever the zoomed page exceeds the preview viewport.
   - Reflects parameter and appearance changes without requiring an Apply action.

2. **Parameter editor**
   - Displays controls for the selected layer's curve family.
   - Includes curve geometry, drawing, color, opacity, and compositing controls.
   - Provides named presets whose values populate editable controls.
   - Any subsequent edit is permitted and does not destroy the preset's starting values.

3. **Layer stack**
   - Shows one row per curve.
   - Supports drag-and-drop reordering.
   - Recalls the selected layer's parameters in the editor.
   - Provides visibility and lock controls.
   - Supports add, duplicate, rename, and delete operations.
   - Creating a layer prompts for its name, prefilled as `Curve type N`, where the type is the English curve-family name and (N) is the next number for that family (for example, `Polar rose 1`, `Polar rose 2`, or `Lissajous 1`).
   - Default names do not change when mathematical parameters change.
   - A user-defined name remains unchanged until explicitly renamed.

The layout must remain usable on laptop-sized displays. Resizable panes and sensible minimum sizes are required.

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

An open or closed eye control changes whether the layer is rendered. Visibility is stored in the project SVG.

### 4.2 Locking

A padlock prevents accidental changes to curve, transform, and appearance parameters. Locked layers remain selectable and inspectable. Unlocking is always available.

### 4.3 Reordering

Layer rows are reorderable by drag and drop. The SVG paint order and RosetteLab metadata order must remain consistent.

## 5. Curve families

### 5.1 Polar rose

Canonical form:

[
r(\theta) = a \cos(k\theta + \phi)
]

Initial parameters:

- radius/amplitude (a);
- angular multiplier (k);
- phase (phi);
- angular rotation;
- adaptive Bézier tolerance in document units.

The mathematical meaning of (k), including the odd/even petal behavior, must be documented in the interface help.

### 5.2 Trochoid

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
- optional forced closure of a limited trace;
- samples per revolution.

### 5.3 Lissajous

Canonical form:

[
x(t)=A_x\sin(f_x t+\phi_x),\qquad
y(t)=A_y\sin(f_y t+\phi_y)
]

Parameters include both amplitudes, both frequencies, phases, duration, and precision.

### 5.4 Harmonograph

The initial model uses damped oscillations on both axes:

[
x(t)=A_x\sin(f_x t+\phi_x)e^{-d_x t}
]

[
y(t)=A_y\sin(f_y t+\phi_y)e^{-d_y t}
]

Parameters include amplitudes, frequencies, phases, damping factors, duration, and precision.

More complete multi-pendulum models are explicitly deferred.

### 5.5 Spirograph

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

Two synchronized representations are required:

- RGBA;
- HSLA.

A graphical picker may be added where the platform toolkit supports it consistently. Numeric entry must remain available.

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

Initial export targets:

- clean SVG without RosetteLab editing metadata;
- PNG at configurable dimensions and scale.

PDF export is deferred unless Qt's rendering stack provides a reliable low-cost path.

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
- Secondary targets: Linux and Windows, subject to GTK packaging validation.

Platform-specific native widgets should be avoided unless isolated behind an abstraction.

## 13. Version scope

### 13.1 Milestone 0.1 — technical foundation

- C++ project and build system;
- Qt 6 Widgets application shell;
- basic three-pane layout;
- document and layer data model;
- polar rose generator;
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
- PNG export;
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
- PDF export;
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
