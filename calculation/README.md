# Calculation

The arithmetic behind Calculator and system search
([roadmap/29](../tessera/roadmap/29-system-apps.md),
[roadmap/26](../tessera/roadmap/26-system-search.md)). A Kira library: it takes
expression text and returns a `Number`, or why there is none.

## Rules

Values are Foundation's `Number`: a 65-bit mantissa and at most 18 fractional
digits.

- Operators: `+ - * /` and parentheses. A leading `-` negates its operand.
- `%` is percent, as on Apple's Calculator: it divides by one hundred, so `50%`
  is 0.5 and `50%*4` is 2. In a sum, `a + b%` and `a - b%` take b per cent of a,
  so `200+10%` is 220.
- Quotients are rounded half-to-even at 18 decimal places, as `Number` does.
- A result whose integer part reaches 2^64 - 616 is "The result is too large".
  A `Number`'s mantissa cannot hold 2^64, and the 616 covers the rounding of a
  quotient, so no operation reaches a trap. Each operation is refused from its
  operands before it runs (`Arithmetic.kira`).
- Division by zero: "Cannot divide by zero".
- A typed number has at most 19 integer digits and 18 decimal places. More
  decimal places: "At most 18 decimal places". More integer digits: "The result
  is too large".
- The display shows 15 significant digits, rounded half-to-even, and never past
  18 decimal places. The exact value is `String(value)`, and it is what history
  keeps.

## Interface

- `public function calculate(text: borrow String) -> Result<Number, CalculationFailure>`
- `public enum CalculationFailure`, and `public function failureText(failure) -> String`
  for its message.
- `public function displayText(value: Number) -> String`: the display text.
- `public function shownExpression(expression: borrow String) -> String`: the
  expression with `×`, `÷` and `−`.

All numeric values cross the interface as `Number`, the failure as an enum.

## Tests

`kira test` runs `app/Tests/`. The Tessera build graph runs them on the host in
its `tests` step, which needs no SDK.
