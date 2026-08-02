# Tests

`Tests/` contains Unreal Automation coverage for the engine-independent Core and its Unreal integration boundaries.

- `AutomataCoreTests.cpp`: compiler diagnostics, VM semantics, exact costs, safety, combat rules, replay integrity, headless execution, and 1,000-run determinism.
- `AWGameTests.cpp`: network submission validation, examples, replay services, payload bounds, and desync verification.
- `AWPresentationTests.cpp`: replay navigation, generated language metadata, editor construction, and real content-asset resolution.

Tests may instantiate lightweight Unreal objects where required, but the headless simulation test deliberately loads no presentation asset. Run the complete group with `Automation RunTests AutomataWar`.