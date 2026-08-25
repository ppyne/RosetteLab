# RosetteLab Architecture

## 1. Technology choice

RosetteLab is planned as a native desktop application written in **modern C++** with a **Qt 6 Widgets** user interface.

Initial baseline:

- C++20;
- Qt 6 Widgets;
- QPainter and QPainterPath for vector preview rendering;
- a dedicated SVG/XML library selected after a small compatibility prototype;
- CMake as the build system;
- Catch2 or GoogleTest for unit tests.

The exact dependency versions will be pinned only after confirming the versions that can be packaged reliably on the supported macOS target.

## 2. Architectural boundaries

The mathematical and document layers must not depend on GTK.

```text
Application / commands
        |
GTK presentation layer
        |
Document model and undo/redo
        |
Curve generators | SVG serializer | Raster exporter
        |
Math and geometry primitives
```

This separation allows curve equations, metadata round-trips, and sampling limits to be tested without starting a graphical session.

## 3. Proposed source layout

```text
RosetteLab/
├── CMakeLists.txt
├── README.md
├── SPECIFICATIONS.md
├── ARCHITECTURE.md
├── LICENSE
├── cmake/
├── docs/
├── resources/
│   ├── icons/
│   ├── styles/
│   └── presets/
├── src/
│   ├── app/
│   ├── core/
│   ├── curves/
│   ├── document/
│   ├── rendering/
│   ├── svg/
│   └── ui/
└── tests/
    ├── curves/
    ├── document/
    └── svg/
```

Directories will be introduced as implementation requires them; empty placeholder directories are avoided.

## 4. Core types

### 4.1 Document

Owns:

- canvas settings;
- ordered collection of layers;
- schema version;
- dirty state;
- undo/redo history.

### 4.2 CurveLayer

Owns:

- stable UUID;
- display name;
- curve definition as a tagged variant;
- appearance;
- transform;
- copy/superposition settings;
- visible and locked flags;
- cached geometry revision.

### 4.3 Curve definitions

Use strongly typed parameter structures rather than a string dictionary:

- `PolarRoseParameters`;
- `TrochoidParameters`;
- `LissajousParameters`;
- `HarmonographParameters`;
- `SpirographParameters`.

A `std::variant` can represent the selected family while retaining compile-time validation.

### 4.4 Appearance

Contains:

- RGBA stroke and fill colors;
- independent stroke and fill alpha;
- stroke width, cap, and join;
- fill rule;
- layer opacity;
- blend mode.

HSLA is an editing representation converted through tested color utilities; SVG serialization uses standards-compliant color and opacity values.

## 5. Curve generation

A generator receives validated parameters and a sampling policy, and returns geometry in document coordinates.

Generation requirements:

- deterministic results;
- finite coordinates only;
- explicit open/closed state;
- estimated and actual point-count limits;
- no GUI calls;
- cancellation support for expensive future jobs.

The first renderer may use polylines or Cairo paths. SVG path simplification and Bézier fitting can be evaluated later without changing the curve API.

## 6. Preview rendering

The Qt preview widget renders from the document model.

Initial strategy:

- QPainter-backed vector drawing;
- redraw invalidated layers after edits;
- cache sampled geometry per layer revision;
- composite layers in document order;
- expose zoom, pan, and fit transforms independently of document geometry.

Blend-mode parity between preview and SVG export must be tested. Modes not rendered accurately are disabled until supported.

Longer generation should move away from the UI thread once profiling demonstrates the need. Updates must be cancellable so rapidly changing controls do not queue obsolete renders.

## 7. Preset system

Presets are data, not hard-coded widget state.

A preset record contains:

- stable ID;
- English display name and description;
- curve family;
- typed curve parameters;
- optional copy settings;
- optional appearance proposal.

Selecting a preset copies data into the layer model. Widgets then display ordinary editable values. Reset reapplies the same record. Preset files should be validated at startup and covered by schema tests.

## 8. SVG serialization

The serializer has two modes:

1. **RosetteLab project SVG**
   - rendered SVG geometry;
   - versioned application metadata;
   - complete editable layer definitions.

2. **Clean SVG export**
   - rendered geometry and standard SVG appearance;
   - no RosetteLab editing metadata unless explicitly requested.

The parser accepts native project SVG only when the root metadata and supported schema are present. XML external entities and network resource resolution are disabled.

A schema migration layer will convert older supported metadata into the current in-memory model. A file from an unsupported newer schema opens read-only or is rejected with a precise message; it is never silently rewritten.

## 9. Commands and undo/redo

All document mutations should be expressed as commands or reversible transactions:

- add/remove/duplicate layer;
- reorder layer;
- change visibility or lock;
- change curve parameters;
- change appearance;
- change document settings.

Continuous control changes should be coalesced into one logical undo operation when practical.

## 10. GUI responsibilities

Qt widgets bind to the selected layer through a controller or view-model layer. Direct mutation from individual callbacks should be avoided.

The GUI is responsible for:

- selection and focus;
- displaying validation errors;
- translating user actions into document commands;
- synchronizing RGBA and HSLA controls;
- enabling/disabling controls for locked layers;
- scheduling preview refreshes.

The core remains responsible for validation and invariants.

## 11. Testing strategy

### Unit tests

- equation reference points;
- closure periods;
- bounded sampling;
- color conversion;
- layer ordering and locking;
- command undo/redo;
- metadata parsing and serialization.

### Round-trip tests

- document model → SVG → document model equivalence;
- stable rendering attributes;
- unknown and unsupported schema handling;
- malformed and hostile XML rejection.

### Golden rendering tests

A small curated set of SVG or raster fixtures may be used for visual regression testing. Mathematical assertions remain primary so tests do not become platform-renderer dependent.

### UI tests

Keep UI automation focused on critical workflows:

- create layer;
- apply and edit preset;
- reorder;
- lock;
- save and reopen.

## 12. Packaging

The first supported package is a macOS application bundle.

Packaging work must account for:

- Qt 6 runtime libraries and plugins;
- platform theme integration and dark-mode behavior;
- application icons;
- library relocation;
- code signing and notarization when distribution begins.

Linux packaging is expected to follow after the application architecture stabilizes. Windows packaging is a separate validation milestone.

## 13. Open technical decisions

Before committing to the first implementation, short prototypes will decide:

- Qt 6 version available through the selected macOS toolchain;
- SVG/XML library and metadata preservation behavior;
- QPainter composition-mode coverage for the required blend modes;
- best Qt color-control integration with RGBA/HSLA synchronization;
- dependency packaging strategy for a self-contained macOS bundle.

These decisions must be recorded as short architecture decision records under `docs/adr/`.
