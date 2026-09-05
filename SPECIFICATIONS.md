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
   - When the selected page background is transparent or translucent, the preview displays it over a checkerboard made of alternating white and 50% grey (`RGB 127, 127, 127`) squares.
   - Supports zoom, pan, fit-to-document, and a configurable background.
   - Shows horizontal and vertical scroll bars automatically whenever the zoomed page exceeds the preview viewport.
   - Reflects parameter and appearance changes without requiring an Apply action.

2. **Parameter editor**
   - Displays controls for the selected layer's curve family.
   - Includes curve geometry, drawing, color, opacity, and compositing controls.
   - Provides named presets whose values populate editable controls.
   - Any subsequent edit is permitted and does not destroy the preset's starting values.
   - The complete editor is vertically scrollable whenever its controls exceed the available window height. Switching curve families must not resize the main window.

3. **Layer stack**
   - Shows one row per curve.
   - Supports drag-and-drop reordering.
   - Recalls the selected layer's parameters in the editor.
   - Each layer row is ordered as: visibility glyph, lock glyph, visual layer thumbnail, then layer name.
   - The thumbnail has the same square dimensions as the visibility and lock controls and previews the layer's current geometry and appearance. Its background reproduces the document background: an opaque document background is shown as a solid color, while a transparent or translucent document background is composited over the standard alternating white and 50% grey (`RGB 127, 127, 127`) checkerboard. Stroke width is reduced to half of its geometrically scaled width, with a 0.25 px visibility minimum and a 1.25 px cap so that fill remains visually dominant. It updates when curve parameters, appearance parameters, transforms, copies, or the document background change.
   - The visibility control uses clickable open-eye and closed-eye UTF-8 glyphs instead of a checkbox.
   - The lock control uses clickable unlocked and locked UTF-8 padlock glyphs instead of a separate Lock/Unlock button.
   - Both glyph controls have stable dimensions, tooltips, keyboard access, and accessible names; changing state must never resize the layer panel.
   - Supports add, duplicate, rename, and delete operations.
   - The primary **Add new layer…** command opens a curve-type selector rather than creating a predetermined family directly.
   - The selector lists Polar rose, Ellipse, Hypotrochoid, Epitrochoid, Lissajous, Harmonograph, and Droplet Rosette; only implemented families are enabled.
   - Creating a layer prompts for its name, prefilled as `Curve type N`, where the type is the English curve-family name and (N) is the next number for that family (for example, `Polar rose 1`, `Polar rose 2`, or `Lissajous 1`).
   - Default names do not change when mathematical parameters change.
   - A user-defined name remains unchanged until explicitly renamed.
   - A name wider than the available row width is elided with a trailing ellipsis. Hovering the elided name displays its complete value in a tooltip.

The layout must remain usable on laptop-sized displays. Resizable panes and sensible minimum sizes are required. The application restores the last main-window size, position, and maximized state on the next launch; Qt must keep a restored window reachable when the previous screen arrangement is no longer available. The widths of the parameter editor, preview canvas, and layer stack are persisted through the main horizontal splitter state and restored on the next launch.

### 3.1 Document commands and recent files

The **File** menu provides:

- **New**: creates a fresh document with the default page and initial Polar rose layer, clears the current document path, and restores the untitled RosetteLab window title;
- **Open…**: opens a supported RosetteLab SVG and establishes its path as the current save target;
- **Open Recent**: lists at most the six most recently opened or successfully saved RosetteLab documents, most recent first. Entries persist between application launches. Each entry retains its absolute path, and **Clean History** is always the final command in the submenu;
- **Save**: writes directly to the established path and is enabled only after a document has been opened or successfully saved with **Save As…**;
- **Save As…**: requests an SVG path, saves the document, establishes that path for subsequent **Save** commands, and adds it to the recent-file history.

RosetteLab stores three independent last-used directories in application preferences: one for **Open…**, one for **Save As…**, and one shared by all **File → Export** commands. Each file dialog starts in its corresponding remembered directory, falling back to the user's home directory when no value exists. A directory is updated only after the user validates a file selection; cancelling a dialog leaves the stored value unchanged. When the current document has a known filename, **Save As…** preselects that filename in its remembered directory. Every export preselects the current document's basename with the target extension (`.svg`, `.pdf`, `.jpg`, or `.png`) in the remembered export directory. An unnamed document uses `Untitled` as its suggested basename. These suggestions never change the current project path and never bypass the clean-SVG protection against overwriting the editable RosetteLab SVG.

**New**, **Open…**, **Save**, and **Save As…** use the platform-standard Qt key sequences, including Command-N, Command-O, Command-S, and Shift-Command-S on macOS. Cleaning the recent-file history does not close or alter the current document.

### 3.2 Unsaved-change protection

Any change to document settings, curve parameters, layer geometry, appearance, visibility, locking, naming, order, addition, duplication, or removal marks the document as modified. The native modified indicator is shown in the window title. A successful **Open…**, **Save**, or **Save As…** operation clears the modified state; view-only changes such as zoom and panel resizing do not set it.

Before **New**, opening another document (including through **Open Recent**), or closing the main window can discard unsaved changes, RosetteLab presents **Save**, **Discard**, and **Cancel**. **Save** continues only after a successful save, **Discard** performs the requested operation without saving, and **Cancel** leaves the current document untouched.

### 3.3 Undo and redo

The **Edit** menu provides **Undo** and **Redo** using the platform-standard Qt key sequences, including Command-Z and Shift-Command-Z on macOS. History covers document settings, curve parameters, layer geometry and appearance, visibility, locking, naming, ordering, addition, duplication, and removal. View-only changes are excluded.

### 3.4 Document defaults and zoom

The last page width, page height, and RGBA background chosen in the **Document**
block are stored in application preferences and used for the next application launch
and for **New** documents. **Reset defaults** restores a white `210 × 210 mm` page
and updates both the current document and the saved defaults.

Zoom remains directly editable from `0.10%` through `3200.00%`. The **View** block
also offers the following exact predefined levels: `0.10`, `0.20`, `0.30`, `0.40`,
`0.50`, `0.75`, `1.00`, `1.50`, `2.00`, `3.00`, `4.00`, `5.00`, `6.25`, `8.33`,
`12.50`, `16.67`, `25.00`, `33.33`, `50.00`, `66.67`, `100.00`, `125.00`,
`150.00`, `200.00`, `300.00`, `400.00`, `500.00`, `600.00`, `800.00`,
`1200.00`, `1600.00`, and `3200.00` percent.

**Fit to workspace** computes the greatest zoom that contains the complete page in
the preview viewport, including the preview margin. It is the default view mode at
startup and is recomputed after relevant window or splitter resizing. Choosing or
typing another zoom leaves fit mode and does not modify the document or its history.
**Actual size (100%)** immediately selects the `100.00%` predefined level.

While the pointer is over the preview, moving the mouse wheel toward the user steps
up through the predefined levels and enlarges the page; moving it upward steps down
and reduces it. A custom or fitted value starts from the nearest predefined level.
When the page exceeds the viewport, the mathematical point beneath the pointer stays
under the pointer after each wheel step; this pointer anchor replaces corner-based
zooming. An axis on which the complete page fits remains automatically centered.
Holding the left mouse button over the preview displays a closed-hand cursor and
pans the page by dragging both scroll positions, allowing navigation without direct
use of the scroll bars.

The history retains up to 200 complete document states. A new edit after Undo discards the former Redo branch. The saved history position is tracked independently: returning to it clears the modified indicator, while discarding that saved state through a new history branch keeps the document marked as modified. Creating or opening a document starts a new history; saving establishes the current state as its saved position.

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

The built-in Polar rose catalogue includes neutral mathematical starting points
with radius 100, zero phase, and zero rotation: decimal `k = 2` and `k = 3`, plus
the exact fractions `1/2`, `3/2`, `5/2`, `7/2`, `1/3`, `2/3`, `4/3`, `5/3`,
`7/3`, `1/4`, `2/4`, `3/4`, `5/4`, `6/4`, and `7/4`. Fractions are retained
exactly as selected, including reducible forms such as `2/4` and `6/4`, so the
interface can present the requested numerator and denominator rather than only
their reduced mathematical value.

Closed roses are generated over their smallest exact closing period. An odd integer
`k` uses \(\pi\), while an even integer `k` uses \(2\pi\). More generally, a reduced
fraction `n/d` uses \(d\pi\) when both integers are odd and \(2d\pi\) otherwise.
The generator must not retrace an already complete curve: duplicate traversal would
cancel the visible fill under the default `Even-odd` rule.

### 5.2 Ellipse

Canonical form:

\[
x(t)=r_x\cos(t),\qquad y(t)=r_y\sin(t)
\]

The curve may then be rotated by an angle \(\alpha\) around its centre.

Parameters:

- horizontal radius \(r_x\);
- vertical radius \(r_y\);
- **Perfect circle — link radii** constraint;
- angular rotation;
- adaptive Bézier tolerance in document units.

When the circle constraint is enabled, \(r_y\) is immediately set to \(r_x\), the vertical-radius field is disabled, and every subsequent change to \(r_x\) updates \(r_y\). Disabling the constraint restores independent editing without changing the current values. The constraint participates in Undo/Redo and is stored explicitly in native SVG metadata; older project files without the attribute default to independent radii.

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

Parameters include both amplitudes, both positive integer frequencies, both phases, angular rotation, and adaptive Bézier tolerance. The complete closed period is derived from the greatest common divisor of the two frequencies so a reducible ratio is not traced repeatedly. This single general parametrization must remain the only Lissajous mode; paper-specific auxiliary variables must not complicate the editor.

The English user documentation includes the 56 configurations shown in Figure 1 of Wang, Zhang, and You, *Design rules for dense and rapid Lissajous scanning*. They are documented as seven frequency ratios combined with eight derived phase values. The paper's auxiliary value \(k\) remains explanatory documentation rather than a RosetteLab field, is allowed to take non-integer values, and must be distinguished explicitly from the polar-rose parameter \(k\).

### 5.5 Harmonograph

The initial model uses damped oscillations on both axes:

[
x(t)=A_x\sin(f_x t+\phi_x)e^{-d_x t}
]

[
y(t)=A_y\sin(f_y t+\phi_y)e^{-d_y t}
]

Parameters include amplitudes, positive real-valued frequencies, phases, non-negative
damping factors, trace duration, angular rotation, and adaptive Bézier tolerance in
document units. The initial harmonograph trace is deliberately open: duration ends
the physical drawing process and the serializer must not add an artificial closing
segment. It is rendered as adaptive cubic Bézier segments and all parameters are
stored in RosetteLab SVG metadata for lossless reopening and editing.

More complete multi-pendulum models are explicitly deferred.

### 5.6 Droplet Rosette

Droplet Rosette is a radial geometric composition inspired by two-part taijitu and
multi-part tomoe constructions. It is an original parametric implementation rather
than a transcription of any published construction plate. A layer contains `n`
congruent drop-shaped closed contours and has rotational symmetry of order `n`:

\[
\theta_k=\alpha+\frac{2\pi k}{n},\qquad k=0,\ldots,n-1
\]

Parameters:

- droplet count `n`, from 2 to 128;
- outer radius, which bounds every droplet;
- angular rotation of the complete rosette.

The construction is the same for every `n`, including `n = 2`. Let `R` be the
outer radius and let `r` be the radius of each of the `n` equal inner circles:

\[
r=\frac{R\sin(\pi/n)}{1+\sin(\pi/n)}
\]

Their centres lie at radius `R-r`; consequently adjacent inner circles are
externally tangent and every inner circle is internally tangent to the outer
circle. Each closed droplet contour comprises the short arc of the outer circle,
the major arc of one inner circle, and the minor arc of the preceding inner
circle. The two inner arcs have a common tangent at their kissing point. This is
the single compass-derived construction used by **Taijitu pair**, **Triple
tomoe**, **Fivefold wheel**, and all other counts. For `n = 2`, it reduces to the
three tangent semicircles of the standard taijitu reference. For `n = 3` and
`n = 5`, it produces the circular heads, tapering tails, and central curvilinear
polygon shown in the supplied historical construction plate.

All droplets are stored as closed subpaths of one compound path, so stroke, fill,
fill rule, opacity, blending, layer transforms, copies, and vector exports apply
uniformly. Native SVG metadata uses the stable type identifier
`droplet-rosette`. New files store only `n`, `R`, and rotation. The reader accepts
and ignores the obsolete `core-radius`, `swirl-degrees`, `width-percent`, and
`roundness` attributes when opening older RosetteLab files.

## 6. Presets

Each curve family offers a curated set of mathematically notable and visually distinctive presets.

The initial catalog contains editable starting points for every curve family,
including 2-, 3-, 5-, and 8-part Droplet Rosettes.
The catalog includes both canonical mathematical cases and deliberately surprising
near-resonant or asymmetric configurations.

Requirements:

- selecting a preset writes its values into the actual editable fields;
- editing a field keeps the layer editable and marks the state as modified/custom;
- a reset action restores the selected preset;
- the reset action remains available after the selector changes to `Custom` and
  restores the last preset chosen for the active layer;
- each layer stores its last preset identifier and whether its parameters have been
  customized; both values persist in RosetteLab SVG metadata;
- switching layers or reopening a document selects the remembered preset only while
  its parameters remain unmodified, otherwise the selector displays `Custom`;
- a newly created layer whose default parameters exactly match a built-in preset is
  associated with that preset rather than being labelled `Custom`;
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

- **Enabled** checkbox, enabled by default and displayed above the stroke-color control;
- default color: opaque black;
- color;
- opacity from 0 to 100%;
- width;
- line cap and join when relevant.

Disabling Stroke removes the contour from the canvas, thumbnails, SVG rendering, and exports while preserving its editable color, alpha, and width. Stroke color and width controls are disabled visually until Stroke is re-enabled. The enabled state and inactive stroke color round-trip through RosetteLab SVG metadata and participate in Undo/Redo.

### 8.2 Fill

- none or color;
- opacity from 0 to 100%;
- fill rule: `nonzero` or `evenodd`, with `evenodd` selected by default for every new layer.

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

`Hue`, `Saturation`, `Color`, and `Luminosity` are present but disabled in the
blend-mode selector because Qt's current raster preview backend cannot render
them accurately. The native PDF writer understands their PDF equivalents, but
they must not be user-selectable until the live preview, thumbnails, and raster
exports can produce the same result. A project created by a future version may
retain such a value when opened; the current UI must show it as unavailable and
must not silently substitute another stored value.

## 9. Superposition and copies

A layer may represent one curve or a generated group of related copies. The supported arrangements are:

- **Superimposed**: every copy shares the layer position while progressive rotation and scale remain available;
- **Linear**: each copy receives progressive horizontal and vertical offsets;
- **Circular**: copy centres lie on an orbit around the layer position.

Circular arrangement provides orbit radius, start angle, angle per copy, and **Rotate with orbit**. Orbital position is computed from the exact cosine and sine of each copy angle. When orbital rotation is enabled, that angle is added to the layer rotation and the independent progressive copy rotation. **Distribute over 360 deg** sets the angular step to `360 / count` as one Undo/Redo operation. Count and angle remain independent so partial arcs, gaps, and multiple turns are possible.

General mathematical-parameter and color sequences are deferred.

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

### 10.3.1 Layer transforms and copy compositions

Every curve layer has a non-destructive transform applied after mathematical curve generation:

- horizontal and vertical position in document units;
- uniform scale by default, with an explicit link control for independent X/Y scales;
- global layer rotation;
- **Reset transform**, which restores position 0/0, linked scale 100%, and rotation 0 degrees without changing curve parameters or copy settings.

Every curve layer can also render a copy composition from the same parametric path:

- copy count (1–1000);
- rotation added for each successive copy;
- multiplicative scale progression, expressed as a percentage in the UI (for example, 95% yields factors 1, 0.95, 0.95², …);
- horizontal and vertical offset added for each successive linear copy;
- circular radius, start angle, angle per copy, and optional orbital orientation.

The **Reset copies** command restores Superimposed arrangement, one copy, zero rotation, offsets, orbit radius and orbit angles, 100% scale progression, and enabled orbital orientation. It does not alter the layer transform, curve-family parameters, appearance, or preset state, and the reset is recorded as one Undo/Redo history operation.

Transform and copy values are editable independently from curve-family parameters and presets. They are stored in RosetteLab SVG metadata, emitted as SVG path transforms, restored on open, duplicated with the layer, and applied identically in the live preview, layer thumbnail, PNG, JPEG, PDF, and SVG output. Each edit creates a normal document-history entry and is therefore supported by Undo/Redo.

Undo/Redo history must never contain consecutive duplicate document states. Composite actions—including applying or restoring a preset, Reset transform, and Reset copies—produce exactly one history entry, so one Undo always restores the complete state immediately preceding the action and one Redo reapplies it.

Rapid successive changes to the same continuous numeric property are coalesced into one history entry while consecutive changes remain no more than 500 ms apart. Coalescing applies independently to curve parameters, document dimensions, layer position/scale/rotation, copy counts and numeric copy settings, stroke width, and layer opacity. A different property, a different layer, a discrete control, a reset or preset action, Undo/Redo, or a pause longer than 500 ms starts a new history entry. Coalescing must never replace the currently saved document state.

### 10.4 Round-trip requirement

Saving, reopening, and saving without edits must preserve the visible composition and all supported RosetteLab parameters. Unknown metadata from a newer schema must not be discarded silently.

### 10.5 Export

The **File → Export** submenu provides:

- **To SVG…**: exports a clean standards-based SVG containing the page background, visible layer groups, standard titles, rendered cubic Bézier paths, transforms, copy compositions, strokes, fills, fill rules, alpha, opacity, stacking order, and blend-mode styles. Hidden layers, `rosettelab:*` attributes, parametric curve elements, preset provenance, locks, and other editing metadata are omitted. The output is intentionally not accepted by **Open…** as an editable RosetteLab project. To prevent accidental loss of editability, clean export refuses to overwrite the current native RosetteLab project path;
- **To PNG…**: raster export at a user-selected resolution from 72 to 1200 DPI; document alpha is preserved and the UI checkerboard is never exported;
- **To JPEG…**: raster export at a user-selected resolution from 72 to 1200 DPI and high quality; transparent document areas are composited onto opaque white because JPEG has no alpha channel; the filename extension is always normalized to `.jpg`, including when `.jpeg` was entered;
- **To PDF…**: export at the exact document dimensions. The default **Preserve vector blend modes** renderer emits PDF 1.7 directly and retains cubic Bézier paths, transformations, copies, fills, strokes, both fill rules, stroke/fill alpha, layer opacity, stacking order, and every supported blend mode. Each visible RosetteLab layer is represented by an isolated PDF transparency-group Form XObject in the selected blending colour space. The blend mode is applied while each copy is painted inside that group, so overlapping copies of a single layer interact exactly as they do in the preview. The completed group is then composited with the same blend mode against lower layers, while layer opacity is applied once to that completed group. The page declares a transparency group in the selected RGB or CMYK device colour space. No image XObject may be introduced by this mode. A second explicit **Rasterize for compatibility** renderer precomposes the page at 300 DPI through Qt for readers or workflows that do not reproduce native PDF transparency correctly. Rasterization is never selected automatically because of a layer's blend mode. On Qt 6.8 and later, the modal export choices offer RGB or CMYK; earlier Qt versions export RGB.

The native renderer maps RosetteLab modes to the standard PDF blend names `/Normal`, `/Multiply`, `/Screen`, `/Overlay`, `/Darken`, `/Lighten`, `/ColorDodge`, `/ColorBurn`, `/HardLight`, `/SoftLight`, `/Difference`, `/Exclusion`, `/Hue`, `/Saturation`, `/Color`, and `/Luminosity`. The last four remain disabled in the application as described in section 8.4. Vector PDF tests must verify the use of transparency groups and `ExtGState`, the requested `/BM`, distinct stroke/fill alpha, group opacity, Bézier operators, fill-rule operators, RGB/CMYK selection, valid cross-reference data, and the absence of `/Subtype /Image`. Release validation additionally covers Adobe Reader, Apple Preview, Affinity, Poppler, and MuPDF.

**Layer-opacity preview limitation:** native PDF correctly applies layer opacity once to the completed transparency group. The current Qt preview and raster renderer applies it to every copy while drawing; consequently, overlaps can differ when a copied layer has opacity below 100%. Until the preview is changed to precompose each layer, users requiring identical output should either keep layer opacity at 100% and use stroke/fill alpha for transparency, or choose **Rasterize for compatibility** to preserve the current preview appearance. The native PDF semantics must not be weakened to reproduce the preview defect.

**CMYK limitation:** the native CMYK option emits generic `DeviceCMYK` values and does not embed an ICC profile or PDF OutputIntent. The export dialog therefore labels it **CMYK (generic, no ICC profile)**. It is suitable for exploratory output but not for a calibrated prepress contract. For colour-critical printing, export RGB vector PDF and perform the final conversion in a colour-managed prepress application using the printer's requested ICC profile. A future calibrated workflow will allow selecting an ICC profile, convert colours through a colour-management engine, embed the profile as an ICCBased colour space, and add the matching OutputIntent.

Preview and all export formats use the same fitted curve geometry. Raster export rejects dimensions above 32,767 pixels per side or 100 million pixels in total.

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
- Supported desktop targets: macOS, Windows, and Linux.
- Native package builds are automated by a GitHub Actions matrix and use the
  platform-specific icon supplied in the repository.
- Test distributions are not notarized. macOS test bundles receive an ad hoc
  signature after Qt deployment to preserve bundle integrity. Apple Developer ID
  signing/notarization and Windows Authenticode signing remain release-
  infrastructure work because they require project-owned certificates.

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
- parametric Droplet Rosette with compound closed subpaths;
- limited/complete trochoid tracing;
- per-family curated presets.

### 13.5 Milestone 1.0 — first stable release

- non-destructive per-layer transforms and copy-based superposition;
- PNG and JPEG export;
- fully vector PDF export with native PDF blend modes, with rasterization retained only as an explicit compatibility fallback;
- undo/redo for document edits;
- user documentation;
- macOS application bundle in a DMG;
- Windows NSIS installer and portable ZIP;
- Linux DEB package and portable tarball;
- tested project migration policy for the 1.x schema.

## 14. Deferred features

The following may wait until after 1.0 unless implementation proves inexpensive:

- full multi-pendulum harmonograph;
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
