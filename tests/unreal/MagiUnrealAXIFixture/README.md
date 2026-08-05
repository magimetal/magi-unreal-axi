# M6 fixture

`MagiUnrealAXIFixture` is source-owned and disposable. `AMagiFixturePawn`, `AMagiFixtureInteractable`, and fixture GameMode expose deterministic PIE movement, interaction, observation, and reset state. Certification copies fixture to canonical cache workspace before Unreal loads project code.

Run from repository root:

```sh
./tests/unreal/certify-m6.sh
./tests/unreal/certify-m6-live.sh
```

Native certification builds plugin and fixture, compiles Blueprints, and runs M6 plus mutation regression contracts. Live certification covers input assets, component/GameMode persistence, Blueprint success/failure diagnostics, next-tick verified PIE input, bounded PNG screenshot, synchronous verified stop, deterministic reset, receipt lookup, and fixture project build.

Latest evidence:

- native: `~/Library/Caches/magi-unreal-axi/m6/native/evidence.T25zpG`
- live: `~/Library/Caches/magi-unreal-axi/m6/live/evidence.m9J5dO`

M6 is complete for UE 5.8.1/56057345 macOS arm64. Blueprint authoring is excluded. M7 production pipeline certification uses this source fixture for build/automation and the sibling blueprint-only package fixture for cook/package.
