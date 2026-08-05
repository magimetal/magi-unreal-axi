---
name: magi-unreal-axi
description: Use when an agent must inspect, mutate, save, run, build, test, cook, package, or diagnose an Unreal Engine project through the non-interactive magi-unreal-axi CLI. Not for non-Unreal work or general shell automation.
---

# Magi Unreal AXI agent guidance

Use `magi-unreal-axi` for non-interactive Unreal project inspection, catalogued safe mutation, explicit persistence, PIE observation, and build/test/cook/package workflows. It is a self-contained CLI plus native editor plugin; never use MCP, Python, Node.js, HTTP, WebSocket, JSON-RPC, console commands, or arbitrary shell as substitutes.

## Workflow

1. Run no-argument home in project directory. Use `project doctor`, then explicit plugin setup when needed.
2. Search or describe capabilities before generic execution. Read exact target and revision before mutation.
3. Pass required revision/idempotency/force flags. Never automatically retry mutation or UAT operations.
4. Check exit code and structured result/receipt. Save explicitly, restart editor, then read back when persistence matters.
5. Use `--dry-run` for destructive actions and pipeline argv inspection. Keep child logs in managed log paths.

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
