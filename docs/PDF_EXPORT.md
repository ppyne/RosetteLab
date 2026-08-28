# PDF export

RosetteLab offers two PDF rendering paths.

## Preserve vector blend modes

This is the default. It writes PDF 1.7 directly and preserves cubic Bézier
geometry, transformations, copies, fill rules, stroke and fill alpha, layer
opacity, stacking order, and native PDF blend modes. Copies blend with other
copies in their layer; the completed layer also blends with lower layers. This
path does not introduce raster images.

## Rasterize for compatibility

This path renders the complete page through Qt at 300 DPI and embeds the result
in a PDF. Use it when a PDF reader or downstream workflow reproduces native PDF
transparency incorrectly, or when an exact match to a current raster preview is
more important than retaining vector paths.

## Known limitations and workarounds

### Unavailable blend modes

`Hue`, `Saturation`, `Color`, and `Luminosity` are greyed out. PDF supports
them, but the Qt preview, thumbnails, and raster exports cannot currently render
them accurately. Enabling them would make the interface preview misleading.

### Layer opacity with copies

Native PDF applies layer opacity once to the completed layer group. The current
Qt renderer applies it while drawing each copy, so overlap regions can differ
when layer opacity is below 100%.

Until the Qt renderer precomposes each layer:

- keep **Layer opacity** at 100% and use stroke/fill alpha when practical; or
- select **Rasterize for compatibility** when the PDF must match the current
  preview exactly.

The native vector writer deliberately keeps the correct group-opacity
semantics.

### Generic CMYK

**CMYK (generic, no ICC profile)** writes DeviceCMYK values without an embedded
ICC profile or OutputIntent. It is not a calibrated prepress output.

For colour-critical printing, export an RGB vector PDF and convert it in a
colour-managed prepress application using the ICC profile requested by the
printer. A future RosetteLab workflow may add profile selection, managed colour
conversion, an ICCBased colour space, and a matching PDF OutputIntent.

PDF readers can also differ in their rendering of transparency and colour. The
release test matrix includes Apple Preview, Adobe Reader, Affinity, Poppler, and
MuPDF.
