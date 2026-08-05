# M7 live certification

Runs disposable source-backed and blueprint-only repository fixtures with default certified engine (`UE_ENGINE_ROOT` optional override). Script builds release CLI and plugin, asserts exact dry-runs, build/repeat, automation list/run, cook materialization, packaged `.app`, output protection, structured failures, bounded logs, durable operation readback, and retains evidence path.

```sh
./tests/unreal/certify-m7-live.sh
```

Set `KEEP_M7_WORK=1` to retain disposable workspace for diagnosis.

Latest passing evidence: `~/Library/Caches/magi-unreal-axi/m7/live/evidence.Ekd7HQ`.