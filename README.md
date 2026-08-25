# RosetteLab

RosetteLab is a desktop application for designing, layering, editing, and exporting mathematical curves as structured SVG documents.

It combines a live drawing canvas, an editable layer stack, and curve-specific controls. Unlike a one-shot SVG generator, RosetteLab preserves every layer's mathematical parameters inside the SVG so a composition can be reopened and edited later.

## Planned curve families

- Polar roses
- Hypotrochoids and epitrochoids
- Lissajous curves
- Harmonographs
- A physical **Spirograph** mode using wheel and hole parameters familiar to users of the drawing toy

The equations and the meaning of their parameters will be shown in the interface and documented.

## Core workflow

1. Add a mathematical curve as a layer.
2. Select the layer and edit its parameters.
3. Reorder layers with drag and drop.
4. Hide or show layers with an eye control.
5. Lock layers to prevent accidental parameter changes.
6. Combine stroke, fill, opacity, fill rule, and blend modes.
7. Save the complete editable composition as a RosetteLab SVG.

RosetteLab will only open SVG files containing the RosetteLab metadata required to reconstruct the document. Ordinary SVG files remain import/export candidates rather than editable RosetteLab project files.

## Interface

The application and its documentation are written in English. The planned desktop interface uses **Qt 6 Widgets** and follows the host operating system theme where supported.

The main window is divided into three working areas:

- live composition preview;
- curve and appearance parameters;
- reorderable layer stack.

## Appearance controls

Each layer is planned to support:

- stroke color and opacity;
- fill color and opacity;
- RGBA and HSLA color entry;
- stroke width;
- `evenodd` and `nonzero` fill rules;
- layer opacity;
- SVG-compatible blend modes such as Normal, Darken, Multiply, Screen, Overlay, and related modes.

## File format

The native project format is SVG with namespaced RosetteLab metadata. The visible SVG geometry remains usable in standards-compliant SVG applications, while RosetteLab-specific metadata stores curve type, parameters, layer order, visibility, locking, appearance, and document settings.

See [SPECIFICATIONS.md](SPECIFICATIONS.md) for the product requirements and staged scope, and [ARCHITECTURE.md](ARCHITECTURE.md) for the initial technical direction.

## Status

The project is currently in specification and architecture setup. The first implementation milestone will establish the Qt application shell, the document model, SVG metadata round-tripping, and a live polar-rose preview.

## License

RosetteLab is licensed under the [BSD 3-Clause License](LICENSE).
