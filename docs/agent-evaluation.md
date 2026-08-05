# Agent evaluation record

Status: **completed**

Target: release candidate on UE 5.8.1 / changelist 56057345 / macOS arm64. Evidence: `~/Library/Caches/magi-unreal-axi/m8/agent-evaluation/run.Gy9dcQ/evidence`.

Evaluated archive SHA-256 was `fafa0a4cf38d4bc9eb2c96a255db14aafce7fb307b9a4814a4315b03f54b76a0`. This identifies historical evaluation input only; it is not final release checksum.

## Jobs and results

Token estimates use `ceil(captured stdout bytes / 4)`.

| PLAN job | Result | CLI calls | Stdout bytes | Est. tokens | Material evidence |
|---|---|---:|---:|---:|---|
| 1. Orient in unknown project | Partial | 11 | 4,127 | 1,032 | Project/engine/plugin/editor state identified without mutation; bridge reads correctly blocked while plugin/editor absent. |
| 2. Diagnose missing/incompatible plugin | Success | 7 | 5,028 | 1,257 | Missing plugin distinguished from incompatible plugin; exact safe repair and dry-run identified. |
| 3. Find actor and inspect properties | Success | 14 | 21,596 | 5,399 | Stable `PlayerStart` identity, actor data, component list, and collision component inspected without mutation. |
| 4. Create level and configured actor | Success | 26 | 14,747 | 3,687 | Level and actor created; receipt readback matched. Agent key is not independently exposed by actor view/list. |
| 5. Save and prove persistence | Success | 27 | 29,731 | 7,433 | Explicit save, stop/restart, exact level reopen, same actor ID/GUID/location, and zero dirty packages. |
| 6. Run PIE and observe behavior | Success | 11 | 9,564 | 2,391 | Start/status/observe/stop/final-status completed; 19 actors observed and stopped state proved. |
| 7. Diagnose compile/build failure | Success | 6 | 7,087 | 1,772 | Controlled compiler error resolved to exact source, line, and diagnostic through structured result plus managed log. |
| 8. Package validated C++ source fixture | Failed validation | 17 | 31,271 | 7,818 | Build passed, automation exposed 14/15 pre-fix result, then game-target arm64 link failure; no package artifact produced. |
| 8b. Package separate Blueprint-only fixture | Success | 13 | 8,348 | 2,087 | Package receipt proved 37 files and 1,389,331,062 bytes, including packaged `.app`. |

Jobs 2–7 succeeded for requested intent. Job 1 is intentionally partial orientation: offline evidence was sufficient to identify next action, but editor-backed project inspection could not occur before plugin installation and editor launch. Job 8 remains truthful failure evidence for C++ source fixture packaging. A later concrete `UInputAction` fixture fix produced full MagiUnrealAXI automation 15/15 at `~/Library/Caches/magi-unreal-axi/read-fixture-fix.KGV2FR/evidence`; it does not remove job 8 game-target arm64 linker evidence.

## Packaging scope

C++ source fixture is build/editor proof only. It is not certified as source game-target packaging proof. Blueprint-only job 8b is certified cook/package target for this engine cell. Do not combine these results into a claim that source game target packaging passed.

## Inefficiencies and ambiguities

- Command-shape guesses caused avoidable usage calls: `capability list`, `project info`, component positional/flag mismatches, level save syntax, top-level `doctor`/`package`, and `operation view --id`. Focused help corrected each without hidden side effects.
- Jobs 3–5 carried highest token cost from broad lists, repeated help, full views, receipt lookups, and restart diagnosis. Actor list returned 100 of 145; selected actor was on first page.
- Actor read schemas omit `agentKey`; job 4 proves submitted key through request/receipt and runtime PIE tag, not independent actor view/list field.
- Editor restart opens an untitled level rather than prior saved level. Job 5 required explicit `level open` before persistence readback.
- Pipeline failures report stable CLI exit 1 plus managed child status/log. Job 7 needed managed log for exact compiler location; job 8 needed managed log for linker symbols.
- Job 8 standalone editor-target build passed, while package-triggered game-target link failed. Structured package receipt correctly contained no package statistics.
- Job 8b was a distinct supported-fixture evaluation, not retry or reinterpretation of job 8.

## M8 release relation

Evaluation and current hardened exact-artifact live certification are complete. Historical evaluation hash remains separate from release checksum in `SHA256SUMS`.
