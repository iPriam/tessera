# 09 — Text

No glyph has ever been rasterised on this machine. The splash is drawn from
geometry precisely because there is no text.

## What text actually requires

Four separate problems that get conflated, in the order they happen:

1. **A font file**, parsed — glyph outlines, metrics, and the tables that map
   characters to glyphs.
2. **Shaping** — turning a run of characters into a run of positioned glyphs.
   This is not a lookup: ligatures, kerning, contextual forms, marks, and
   bidirectional text all happen here.
3. **Rasterisation** — turning an outline into coverage values at a size.
4. **Composition** — getting those pixels onto the screen efficiently, which for
   a GPU means a glyph atlas and a cache.

Skipping any of them produces something that looks like text and is wrong in a
way that is hard to name and impossible to unsee.

## The engine

Tessera's SDK is Kira and RIDL ([14](14-tooling-and-ports.md)), so all four
steps are Kira code in KCoreAnimation, the layer under Kira UI that owns text
and symbols ([10](10-toolkit.md)). There is no C text engine on Tessera.

On the platforms where Kira UI already ran, KCoreAnimation draws text through
`kiratext`: FreeType for parsing and rasterisation and HarfBuzz for shaping,
vendored as C in `kcoreanimation/NativeLibs/Text/`. That package row has no
Tessera target and never gets one. Until the Kira engine replaces it, a text
call on Tessera reaches a binding with no library behind it and traps by name,
which is why nothing on this machine draws a word yet.

Half of what the Kira engine needs is already there:

- **Rasterisation.** Symbols are drawn in Kira now
  (`kcoreanimation/app/Content/Symbol/`): outlines are scanned row by row,
  several sub-rows to a pixel, into a coverage plane, with strokes as a
  distance field. A glyph outline is the same problem with quadratic and cubic
  curves, and lands in the same plane.
- **The atlas.** Glyphs and symbols share one atlas texture, packed in shelves
  as they arrive and indexed by a Foundation `Map`; a glyph is drawn as a quad
  with atlas coordinates.

Reading the font and shaping are Kira too, in `kcoreanimation/app/Text/`.
`FontFile` reads `cmap`, `hmtx`, `glyf` and the variation tables; `CFF`
outlines are refused for now. The shaper (`TextShaping`) takes a run, its
script and its direction:

- It maps the run's scalars to glyphs.
- It runs the `GSUB` lookups that `ccmp`, `locl`, `rlig`, `rclt`, `calt`,
  `liga` and `clig` turn on. These cover single, multiple and ligature
  substitution, plus contextual and chained contextual rules in all three
  formats.
- It runs the `GPOS` lookups that `kern`, `mark` and `mkmk` turn on: single
  and pair adjustment, with the item variation store moving kerning with a
  variable face's weight, and mark-to-base, mark-to-ligature and
  mark-to-mark attachment. A mark is drawn where its anchor meets the
  glyph's it hangs from, and takes no room on the line.
- A run that reads right to left is then turned into the order it is seen
  in. Substitution and positioning both run in the order it was written, as
  HarfBuzz runs them.
- Lookup flags are honoured through `GDEF`'s glyph classes, mark classes and
  mark sets.

Measuring and drawing read the same shaped run, so a run's box and its glyphs
stay one width. Still to come: cursive attachment,
contextual positioning, alternates, reverse chaining, the old `kern` table,
and the bidirectional pass above the shaper.

## The font

Tessera has to ship one, and it is a decision, not a detail — it is most of what
the system looks like.

Requirements:

- **A licence that permits redistribution in a sealed image.** SIL Open Font
  Licence or similar. Not a font whose licence assumes a person installed it.
- **A real family**, not a single weight. A UI needs at least regular and
  medium, ideally with a variable axis so weights cost one file.
- **Wide enough coverage** that a missing glyph is rare. Latin at minimum;
  anything beyond is a size decision.
- **A monospace companion**, if anything ever shows code or a log.

The size matters more here than usual: a sealed system volume carries the font
forever and a variable font with broad coverage is several megabytes.

There is no need to decide this before the pipeline works — a placeholder loads
the same way — but it should be decided before anything ships, because changing
the system font late changes every layout.

## The pipeline on Tessera

    a file on the system volume       /system/share/fonts/…
      ↓ read whole                    [05]
    the font's tables, read in Kira
      ↓ set size, in pixels at the display's scale
    shape(text, font, script, direction, language)
      ↓ glyph ids and positions
    rasterise each glyph once into a coverage plane
      ↓ coverage levels
    a glyph atlas texture on the GPU  [07]
      ↓
    one draw call: quads with atlas coordinates

**The cache is the design.** Rasterising a glyph is expensive and happens once
per glyph per size per style; drawing it is a quad. An atlas that is a single
texture, packed as glyphs arrive, with an eviction story for when it fills, is
what makes text cost nothing after the first frame.

Getting this wrong is the classic performance failure of a young UI stack — a
draw call per glyph, or a rasterisation per frame — and it is invisible until
there is a screen full of text.

## Decisions

- **Hinting: off, or light.** Full hinting distorts outlines to fit a pixel grid
  and was designed for low-resolution displays. Tessera targets a Retina-class
  display and the modern answer is to leave outlines alone. Also simpler.
- **Antialiasing: grayscale, not subpixel.** Subpixel rendering assumes a known
  physical arrangement of R, G and B, which is false on rotated displays and on
  anything that composites. Grayscale coverage is correct everywhere.
- **Gamma.** Coverage is not brightness. Blending glyph coverage in a
  non-linear space makes light-on-dark text look bolder than dark-on-light, and
  it is why so much software's dark mode looks wrong. Compose in linear space.
- **Subpixel positioning: yes.** A glyph placed at a fractional position looks
  correct at small sizes; snapping to integers makes spacing uneven. Costs a few
  atlas entries per glyph.
- **Bidirectional text and complex scripts: not yet, and do not design them
  out.** The shaper takes a direction and a script from the first version on.
  What is missing is the bidi reordering pass above it, and a layout that assumes left-to-right in its data
  structures is the thing that makes adding it later expensive.

## What sits above

Shaping produces positioned glyphs for a *run* — one font, one size, one script,
one direction. A paragraph is several runs, and turning text into runs is
itemisation: splitting on script changes, style changes, and font fallback when
a glyph is missing.

Then line breaking, which has a Unicode algorithm and is not "split on spaces".

Both belong in Kira UI Foundation rather than in Tessera
([10](10-toolkit.md)), and both are places where a simple version works for a
long time and then suddenly does not.

## Checkpoints

- [x] Symbols are rasterised in Kira, with no C, into the atlas glyphs use.
- [x] Fonts are read, shaped and rasterised in Kira, and `kiratext` is gone
      from KCoreAnimation. Reading (`FontFile`, variations through `gvar`,
      `HVAR` and `CFF2` blends), shaping (`TextShaping`, with `GSUB`
      substitution, Arabic joining forms, and `GPOS` kerning, cursive and
      mark attachment and contextual positioning) and rasterising
      (`GlyphOutline` for `glyf`, `Charstrings` for `CFF ` and `CFF2`,
      `GlyphCoverage`) are Kira, and KCoreAnimation links no C. Source Serif
      4 Variable, a `CFF2` face, sets "Hog" at weights 400 and 900 with
      `hb-shape`'s advances, and each glyph's control box within a unit of
      its extents; Inter at 700 sets "Hamburgefonstiv" with `hb-shape`'s
      glyphs and advances (one-off comparisons, the face not being one macOS
      ships). `tests/text_kik` sets its runs in the bundled Inter
      (`foundationTextUseBundledFace`) and checks on the host that it kerns
      "AV" and "ΑΥ", sets "->" as one arrow, hangs a combining acute from its
      letter's anchor and stacks a second one on the first; and, in macOS's
      STIX General, that a `CFF ` face's capitals stand at its cap height and
      its "g" hangs below the line; and, in Noto Nastaliq Urdu, that Arabic
      letters take their joining forms (`isol`, `fina`, `medi`, `init`,
      after `ccmp`) and hang from each other by cursive attachment, at the
      widths `hb-shape` sets (a one-off dump matched it glyph for glyph, in
      advances, offsets and rises). A run is set in one face: there are no
      per-script fallback faces yet, so Arabic in the system UI still draws
      in Inter, which has no Arabic.
- [x] A font ships on the system volume and is read at startup: Inter, in
      `/System/Library/Frameworks/KCoreAnimation.framework/Resources/Fonts`.
- [x] One glyph, shaped and rasterised, is on the display: every label of the
      desktop, its Dock and its applications.
- [x] A line of text, correctly kerned, with a ligature visible: "AV->"
      typed into the launcher's search on a 2880x1800 output draws with A and
      V kerned together and "->" as Inter's arrow, the caret right after it.
- [x] Glyphs are cached in an atlas and a second frame rasterises nothing.
      `-services-multi` booted with
      `KIRA_LOG=KCoreAnimation.Render.UiBatchStats=debug` logs each frame's
      `a frame rasterised glyphs`: a client's first frame fills its atlas
      (`count: 24`, `count: 62`), every frame after it draws from the atlas
      with `count: 0`, and only text that was not on the screen before, the
      keys the boot types, rasterises again (`count: 8`, `count: 14`).
- [x] A screen full of text is one draw call. Glyphs and the rectangles
      around them share one pipeline and one atlas, so a layer of content is
      one indexed draw however much text it holds. Measured on the host with
      kira-ui's showcase (`KCoreAnimation` `399c3f9` counts draw calls; the
      per-run breakdown was a temporary print): the Showcase page, thirteen
      rows of labels, separators and chevrons, is 135 quads in one draw call,
      replayed once for each glass capture above it. Text on glass chrome
      adds a draw call for each change of blend: the tab bar's plain labels
      and its dodged (vibrant) ones are separate runs.
- [ ] Text composed in linear space looks the same weight light-on-dark as
      dark-on-light.
