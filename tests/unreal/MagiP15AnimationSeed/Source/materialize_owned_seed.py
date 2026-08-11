#!/usr/bin/env python3
"""Materialize and semantically verify the owned P1.5 GLB seed with UE 5.8."""

import hashlib
import json
from pathlib import Path

import unreal

project_dir = Path(unreal.Paths.project_dir()).resolve()
source = project_dir / "Source" / "magi-p15-owned-seed.glb"
destination = "/Game/MagiP15Seed"
expected_source_hash = "e5127ab92df4d7414e8a78191513b9b6ad1cde3ef699fb2a2a7f04e035f3f286"

actual_source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
if actual_source_hash != expected_source_hash:
    raise RuntimeError(f"seed source hash mismatch: {actual_source_hash}")

if unreal.EditorAssetLibrary.does_directory_exist(destination):
    if not unreal.EditorAssetLibrary.delete_directory(destination):
        raise RuntimeError("failed to clear prior seed assets")

task = unreal.AssetImportTask()
task.set_editor_property("filename", str(source))
task.set_editor_property("destination_path", destination)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("replace_existing", False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

root = destination + "/magi-p15-owned-seed/SkeletalMeshes/"
paths = {
    "material": destination + "/magi-p15-owned-seed/Materials/MagiP15SeedMaterial.MagiP15SeedMaterial",
    "skeletalMesh": root + "magi-p15-owned-seed.magi-p15-owned-seed",
    "skeleton": root + "magi-p15-owned-seed_Skeleton.magi-p15-owned-seed_Skeleton",
    "idle": root + "magi-p15-owned-seedIdle.magi-p15-owned-seedIdle",
    "moving": root + "magi-p15-owned-seedMoving.magi-p15-owned-seedMoving",
    "physicsAsset": root + "magi-p15-owned-seed_PhysicsAsset.magi-p15-owned-seed_PhysicsAsset",
}
assets = {name: unreal.EditorAssetLibrary.load_asset(path) for name, path in paths.items()}
for name, asset in assets.items():
    if asset is None:
        raise RuntimeError(f"missing imported {name}: {paths[name]}")

classes = {name: asset.get_class().get_name() for name, asset in assets.items()}
expected_classes = {
    "material": "MaterialInstanceConstant",
    "skeletalMesh": "SkeletalMesh",
    "skeleton": "Skeleton",
    "idle": "AnimSequence",
    "moving": "AnimSequence",
    "physicsAsset": "PhysicsAsset",
}
if classes != expected_classes:
    raise RuntimeError(f"unexpected imported classes: {classes}")

skeleton_path = assets["skeleton"].get_path_name()
for name in ("skeletalMesh", "idle", "moving"):
    bound = assets[name].get_editor_property("skeleton")
    if bound is None or bound.get_path_name() != skeleton_path:
        raise RuntimeError(f"{name} does not bind exact seed Skeleton")

idle_length = float(assets["idle"].get_editor_property("sequence_length"))
moving_length = float(assets["moving"].get_editor_property("sequence_length"))
if abs(idle_length - 2.0) > 0.05 or abs(moving_length - 1.5) > 0.05:
    raise RuntimeError(f"unexpected sequence lengths: idle={idle_length}, moving={moving_length}")
if not unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True):
    raise RuntimeError("failed to save seed assets")

report = {
    "source": str(source.relative_to(project_dir)),
    "sourceSha256": actual_source_hash,
    "classes": classes,
    "paths": {name: asset.get_path_name() for name, asset in assets.items()},
    "skeleton": skeleton_path,
    "sequenceLengths": {"idle": idle_length, "moving": moving_length},
}
report_path = project_dir / "Saved" / "p1.5-seed-report.json"
report_path.parent.mkdir(parents=True, exist_ok=True)
report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
unreal.log("MAGI_P15_SEED_REPORT=" + str(report_path))
