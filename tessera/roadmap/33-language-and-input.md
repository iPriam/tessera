# 33 — Language and input

Writing in the person's language is an OS feature. Tessera needs selectable
keyboard layouts, composed input, input methods, an emoji picker, dictation,
spellcheck and text replacement. Applications should receive consistent text
editing behaviour without each implementing its own keyboard and language stack.

The current named US keymap and Kira text engine provide a starting point.
Font shaping is not the whole input system: entering, composing, selecting,
breaking, displaying and navigating multilingual text are separate contracts.
These changes extend [06](06-devices-and-input.md) and [09](09-text.md).

## Language and keyboard preferences

Setup and Settings choose display language, preferred language order, region,
number/date conventions and enabled input sources. Applications can select a
different supported language without changing the entire system. Missing
translations fall back predictably while preserving the requested preference.

Keyboard layouts are data with documented provenance, not handwritten tables
inside each application. Hardware key position, shortcut modifiers and resulting
text remain distinct. A layout change affects text entry without silently
changing the meaning of every physical-key command; shortcut matching must
have a defined policy for positional and textual bindings.

Layout switching has a visible indicator and a discoverable shortcut. Dead
keys and compose sequences maintain state across key events and cancel safely
on input-source or focus changes. A preview shows the active layout and modifier
levels. Hardware keyboards and an on-screen keyboard use the same input-source
configuration while retaining appropriate platform presentation.

## Composition and input methods

Introduce an explicit text-input protocol: surrounding-text requests with
bounded scope, marked/composing text, selection, replacement ranges, commit
and cancel. KiraUI text controls implement it once. An input method proposes
compositions and candidates; it never simulates a sequence of raw keystrokes
and hopes the target editor produces the intended document.

Candidate selection supports touch, pointer, keyboard and accessibility.
Candidate windows follow the actual caret rectangle, output scale and editor
clipping, including after scrolling or changing displays. A focus change ends
or cancels composition according to the editor contract, with no text committed
to the newly focused app. Secure fields disallow surrounding-text disclosure,
history learning, replacement and unapproved third-party input processing.

Use a supervised input service for source selection and composition sessions,
with typed RIDL messages to the text-input client. PlatformKit owns the focused
client's identity; the service cannot attach to arbitrary background editors.
Language-specific engines are Kira components with bounded dictionaries and
versioned data. Start with one complete composition workflow, then extend
language coverage without changing every application's event loop.

## Emoji, spelling and replacement

The emoji/symbol picker searches names and categories, inserts through the
text-input protocol and remembers recently chosen symbols locally. Variants
and multi-codepoint emoji remain one selectable user-facing sequence where
the Unicode editing rules require it. A missing glyph has a useful fallback,
not a broken editor or a silently different character.

Spellcheck marks suspected words, offers replacements and supports per-language
dictionaries and a private learned-word list. It does not rewrite text without
the chosen correction policy. Text replacement has editable triggers, results
and enablement; a replacement is one undoable edit and does not run in secure
fields. App overrides and document language are explicit inputs.

Dictation is an authorized microphone session with a visible active state,
start/stop controls and partial text represented as composition until committed.
The recognition engine and language support must be selected and implemented
before declaring the feature available. On-device and network-backed recognition
have distinct disclosure and offline behaviour; audio is not uploaded by default
because a microphone permission was granted. Cancellation removes uncommitted
text and stops capture.

## Rendering, editing and localisation

The shared text stack needs grapheme-aware selection and deletion, word and
line movement, bidirectional paragraph ordering, font fallback, script/item
segmentation and Unicode line breaking. Shape runs through KCoreAnimation's
Kira engine, then lay out paragraphs in UI Foundation. Input and navigation
offsets must use a defined coordinate unit and conversion, so a UTF-8 byte
offset cannot accidentally be interpreted as a glyph index.

Localised UI uses resources and locale-aware formatting, including plurals.
Right-to-left layout is a semantic direction setting; icons, navigation and
mixed-direction content are reviewed rather than blindly mirroring every pixel.
User text and filenames retain their bytes/normalisation contract independently
of the current interface language. Existing documents are not rewritten when
a language preference changes.

## Implementation order

1. Define text ranges, composition and focus handover; add data-driven layouts,
   dead keys, source switching and one complete composing input method.
2. Complete grapheme/bidi/fallback/line-break behaviour in shared text controls
   and expose localisation resources and locale formatting.
3. Add emoji, spellcheck and undoable replacement using the same editing path.
4. Add dictation after microphone authorization, recognition provenance and
   offline behaviour are defined; extend language coverage with fixtures.

## Status

MosaicDisplay compiles in four XKB layouts at their first two levels, `us`,
`gb`, `de` and `fr`, and types through the one `keyboard.layout` names,
following a change from the next key. Keys XKB makes dead type their
character at once, and AltGr is not compiled in. The language chosen in
setup is kept (`language.preferred`); nothing is translated yet.

## Checkpoints

- [ ] Switch between US and a different physical layout, enter dead-key text
      and retain the chosen source across reboot without breaking documented shortcuts.
- [ ] Compose and choose candidates in a supported input method, scroll the
      editor and change displays. The candidate window follows the real caret.
- [ ] Change focus midway through composition and verify that no text lands
      in another app; secure fields expose no surrounding text or learned history.
- [ ] Enter mixed right-to-left and left-to-right text, combining marks and
      multi-codepoint emoji. Selection, deletion, caret movement and undo remain correct.
- [ ] Change system language and region, including a right-to-left locale;
      translated navigation, plurals, numbers and dates follow the documented fallback rules.
- [ ] Correct a spelling, learn a word and expand a replacement; undo each
      operation and confirm the same behaviour in two applications.
- [ ] Dictate with explicit capture, commit and cancel; revocation or network
      loss stops according to the selected engine's documented behaviour.
