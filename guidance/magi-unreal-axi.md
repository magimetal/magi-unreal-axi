# Magi Unreal AXI agent guidance

Use `magi-unreal-axi` for non-interactive Unreal project inspection, catalogued safe mutation, explicit persistence, PIE observation, and build/test/cook/package workflows. It is a self-contained CLI plus native editor plugin; never use MCP, Python, Node.js, HTTP, WebSocket, JSON-RPC, console commands, or arbitrary shell as substitutes.

## Workflow

1. Run no-argument home in project directory. Use `project doctor`, then explicit plugin setup when needed.
2. Search or describe capabilities before generic execution. Read exact target and revision before mutation.
3. Pass required revision/idempotency/force flags. Never automatically retry mutation or UAT operations.
4. Check exit code and structured result/receipt. Save explicitly, restart editor, then read back when persistence matters.
5. Use `--dry-run` for destructive actions and pipeline argv inspection. Keep child logs in managed log paths.

## Discovery availability contract

`capability describe` reports tri-state runtime availability. `local` capabilities are `available` offline and use an empty `reasons` array. Native capabilities are `unknown` with reason `editor_offline` when no live matching editor exists. With live matching editor, native entries are `available` with structured reasons empty, or `unavailable` with one or more structured reasons. Execute only when availability is `available`; inspect reasons otherwise.

## Failed Blueprint compile contract

Failed `blueprint.compile` exits `1` with error reason/type `blueprint_compile_failed` and a validated failed receipt. Receipt state is `failed`, transaction is `non-atomic`, `savedPackages` is empty, and revisions, observed status, changed objects, and bounded diagnostics describe current state. Invalid dirty authoring remains dirty with exact `dirtyPackages`; an already-invalid clean asset may truthfully report `persistence:unchanged`. Failure makes no rollback or saved-persistence claim. Inspect `operation view <operation-id>` before retry; do not automatically retry. If editor is offline, `operation view` reads bounded journal fallback.

## Non-interactive examples

```sh
magi-unreal-axi --format json
magi-unreal-axi --project /path/Game.uproject --format json project doctor
magi-unreal-axi --project /path/Game.uproject --engine /path/UE_5.8 --format json project build --dry-run
magi-unreal-axi --project /path/Game.uproject --engine /path/UE_5.8 --format json project test list --limit 100
magi-unreal-axi --project /path/Game.uproject --engine /path/UE_5.8 --format json project cook --output ./Cooked --dry-run
magi-unreal-axi --project /path/Game.uproject --format json operation view proc-example
```

Never expose bridge tokens, emit screenshot bytes, parse child output from stdout, or claim mutation/persistence before receipt and readback evidence.
