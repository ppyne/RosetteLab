# ADR 0001: Native vector PDF blend modes

## Status

Accepted and implemented.

## Context

`QPdfWriter` does not preserve the `QPainter` composition modes used by
RosetteLab. The former exporter therefore rasterized the complete page whenever
a visible layer used a non-Normal blend mode. The visual composition survived,
but all curve geometry was lost.

PDF 1.7 can represent RosetteLab's required compositing directly with Form
XObjects, transparency groups, and `ExtGState` dictionaries.

## Decision

RosetteLab owns a small PDF 1.7 serializer tailored to its document model.
Every visible layer is emitted as an isolated transparency-group Form XObject:

- the curve and all its transformed copies are painted inside the form;
- stroke and fill alpha are applied while painting the paths;
- layer opacity and the PDF `/BM` blend mode are applied once when the finished
  form is composited onto the page;
- the page itself declares a transparency group in DeviceRGB or DeviceCMYK;
- the native path never emits image XObjects.

The distinction between path alpha and group opacity is intentional. Applying
layer opacity independently to copies would change the appearance where copies
overlap.

The existing Qt raster path remains available only through an explicit
**Rasterize for compatibility** choice. Native vector output is the default.

## Consequences

Blend-mode compositions remain editable and scalable in exported PDFs. The
serializer is deliberately constrained to RosetteLab's primitives, keeping the
implementation auditable, but RosetteLab must maintain PDF object, stream, and
cross-reference generation itself. Tests therefore inspect the emitted PDF
structure in addition to visual release checks in multiple PDF readers.
