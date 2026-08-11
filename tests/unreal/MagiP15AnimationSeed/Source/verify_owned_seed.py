#!/usr/bin/env python3
"""Verify committed P1.5 seed assets inside UE without mutating them."""

import json
from pathlib import Path

import unreal

project_dir = Path(unreal.Paths.project_dir()).resolve()
root = "/Game/MagiP15Seed/magi-p15-owned-seed/"
paths = {
    "material": root + "Materials/MagiP15SeedMaterial.MagiP15SeedMaterial",
    "skeletalMesh": root + "SkeletalMeshes/magi-p15-owned-seed.magi-p15-owned-seed",
    "skeleton": root + "SkeletalMeshes/magi-p15-owned-seed_Skeleton.magi-p15-owned-seed_Skeleton",
    "idle": root + "SkeletalMeshes/magi-p15-owned-seedIdle.magi-p15-owned-seedIdle",
    "moving": root + "SkeletalMeshes/magi-p15-owned-seedMoving.magi-p15-owned-seedMoving",
    "physicsAsset": root + "SkeletalMeshes/magi-p15-owned-seed_PhysicsAsset.magi-p15-owned-seed_PhysicsAsset",
}
expected_classes = {
    "material": "MaterialInstanceConstant",
    "skeletalMesh": "SkeletalMesh",
    "skeleton": "Skeleton",
    "idle": "AnimSequence",
    "moving": "AnimSequence",
    "physicsAsset": "PhysicsAsset",
}
assets = {name: unreal.EditorAssetLibrary.load_asset(path) for name, path in paths.items()}
for name, asset in assets.items():
    if asset is None:
        raise RuntimeError(f"missing committed {name}: {paths[name]}")
classes = {name: asset.get_class().get_name() for name, asset in assets.items()}
if classes != expected_classes:
    raise RuntimeError(f"unexpected committed classes: {classes}")

skeleton_path = assets["skeleton"].get_path_name()
for name in ("skeletalMesh", "idle", "moving"):
    bound = assets[name].get_editor_property("skeleton")
    if bound is None or bound.get_path_name() != skeleton_path:
        raise RuntimeError(f"{name} does not bind exact seed Skeleton")

idle_length = float(assets["idle"].get_editor_property("sequence_length"))
moving_length = float(assets["moving"].get_editor_property("sequence_length"))
if abs(idle_length - 2.0) > 0.05 or abs(moving_length - 1.5) > 0.05:
    raise RuntimeError(f"unexpected sequence lengths: idle={idle_length}, moving={moving_length}")
for name in ("idle", "moving"):
    if bool(assets[name].get_editor_property("enable_root_motion")):
        raise RuntimeError(f"{name} unexpectedly enables root motion")

report = {
    "classes": classes,
    "paths": {name: asset.get_path_name() for name, asset in assets.items()},
    "rootMotion": {name: False for name in ("idle", "moving")},
    "sequenceLengths": {"idle": idle_length, "moving": moving_length},
    "sharedSkeleton": skeleton_path,
}
report_path = project_dir / "Saved" / "p1.5-seed-verification.json"
report_path.parent.mkdir(parents=True, exist_ok=True)
report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
unreal.log("MAGI_P15_SEED_VERIFICATION=" + str(report_path))
