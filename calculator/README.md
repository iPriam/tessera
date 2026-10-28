# Calculator

The Calculator application for Tessera. It follows
[roadmap/29](../tessera/roadmap/29-system-apps.md), Calculator.

## Rules

The arithmetic is the `calculation` library (`../calculation`, its README has
the rules): exact decimal `Number`s, half-to-even quotients at 18 places, the
overflow bound, division by zero, and the display's 15 significant digits.

- History keeps the expression, the answer as shown and the exact value. Reusing
  a history row puts the exact value back into the expression, so the reuse
  matches the stored result.

## Input

The keypad edits one text. The digit keys also answer the keyboard. Operators,
`.`, Return and `⌫` from the keyboard are not yet handled: KiraUI's keyboard
shortcuts take letters, digits and Space only, so they need a key press handler
in KiraUI. `=` evaluates the text; `AC` clears the expression and the last result.

## Decisions

These follow Apple's Calculator on iOS 26.

- `%` is percent, not remainder. `50%` is 0.5, and `200+10%` is 220. The rule is
  in the `calculation` README.
- Operator keys pass an orange tint to KiraUI's `glassEffect(... tint:)`
  (kira-ui `dc716e4`). Known gap: the tint does not show on screen yet. The keys
  draw as plain dark glass in the VM, even with a forced red tint in KiraUI.
  Parked; the cause is not found.
- The background is black under the status bar and toolbar, so no gray band
  shows. Apple's bars are clear over the content.
- The icon at the top right opens Math Notes or the Converter on Apple's
  Calculator. Neither exists here, so the icon is disabled until the Converter
  is built (roadmap/29, unit conversion).

## Not yet

- Paste. It reads the pasteboard with the interaction token of the key press or
  click that asked for it (`interactionCurrent`). The pasteboard service asks
  the Presence service to verify the token (roadmap/16 §14.1), and the text is
  typed into the expression only when it is a valid expression. Outside a key
  press or click there is no token, and Paste says so. Copy writes the value
  the display shows through `pasteboardWrite` (roadmap/23 §5.1).
- Verified in the dev VM: type `5`, Copy, type `9`, Paste shows `595`
  on the dev VM with the presence verdict wait and the memory-type fallback in
  ember.
- Locale-sensitive decimal entry. Only `.` is accepted.
- Scientific functions and unit conversion.
- History is kept while the application runs, not across launches.

## Tests

The evaluator's tests are in the `calculation` library.
