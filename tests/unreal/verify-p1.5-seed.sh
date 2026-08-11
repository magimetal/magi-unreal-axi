#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.5-seed-provenance.json"
seed_root="$repo_root/tests/unreal/MagiP15AnimationSeed"
engine_root=${UE_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}
editor_cmd="$engine_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
version_file="$engine_root/Engine/Binaries/Mac/UnrealEditor.version"
[[ -f "$manifest" && -f "$repo_root/LICENSE" ]]
jq -e '
  .schemaVersion == 1 and .ownership.license == "MIT" and
  .materialization.engine == {version:"5.8.1",changelist:56057345,hostArchitecture:"arm64"} and
  (.files | length) == 6 and ([.files[].class] | sort) == (["AnimSequence","AnimSequence","MaterialInstanceConstant","PhysicsAsset","SkeletalMesh","Skeleton"] | sort)
' "$manifest" >/dev/null

while IFS=$'\t' read -r relative expected_hash expected_size; do
  file="$repo_root/$relative"
  [[ -f "$file" ]]
  [[ $(shasum -a 256 "$file" | cut -d' ' -f1) == "$expected_hash" ]]
  [[ $(stat -f %z "$file") == "$expected_size" ]]
done < <(jq -r '.files[] | [.path,.sha256,(.sizeBytes|tostring)] | @tsv' "$manifest")

generator=$(jq -r .source.generator "$manifest")
glb=$(jq -r .source.glb "$manifest")
materializer=$(jq -r .materialization.script "$manifest")
verifier=$(jq -r .verification.script "$manifest")
[[ $(shasum -a 256 "$repo_root/$generator" | cut -d' ' -f1) == "$(jq -r .source.generatorSha256 "$manifest")" ]]
[[ $(shasum -a 256 "$repo_root/$glb" | cut -d' ' -f1) == "$(jq -r .source.glbSha256 "$manifest")" ]]
[[ $(stat -f %z "$repo_root/$glb") == "$(jq -r .source.glbSizeBytes "$manifest")" ]]
[[ $(shasum -a 256 "$repo_root/$materializer" | cut -d' ' -f1) == "$(jq -r .materialization.scriptSha256 "$manifest")" ]]
[[ $(shasum -a 256 "$repo_root/$verifier" | cut -d' ' -f1) == "$(jq -r .verification.scriptSha256 "$manifest")" ]]

tmp=$(mktemp -d "${TMPDIR:-/tmp}/magi-p15-seed.XXXXXX")
cleanup() { /usr/bin/trash "$tmp" 2>/dev/null || true; }
trap cleanup EXIT
python3 "$repo_root/$generator" "$tmp/seed.glb" >"$tmp/generate.txt"
cmp "$repo_root/$glb" "$tmp/seed.glb"
grep -qx "size=$(jq -r .source.glbSizeBytes "$manifest")" "$tmp/generate.txt"
grep -qx "sha256=$(jq -r .source.glbSha256 "$manifest")" "$tmp/generate.txt"


[[ -x "$editor_cmd" && -f "$version_file" ]]
[[ "$(uname -m)" == arm64 ]]
[[ "$(plutil -extract MajorVersion raw -o - "$version_file")" == 5 ]]
[[ "$(plutil -extract MinorVersion raw -o - "$version_file")" == 8 ]]
[[ "$(plutil -extract PatchVersion raw -o - "$version_file")" == 1 ]]
[[ "$(plutil -extract Changelist raw -o - "$version_file")" == 56057345 ]]
report="$seed_root/Saved/p1.5-seed-verification.json"
/usr/bin/trash "$report" 2>/dev/null || true
"$editor_cmd" "$seed_root/MagiP15AnimationSeed.uproject" -unattended -nop4 -nosplash -nullrhi -NoSound -run=PythonScript -Script="$repo_root/$verifier" >"$tmp/unreal.txt" 2>&1
jq -e '
  .classes == {idle:"AnimSequence",material:"MaterialInstanceConstant",moving:"AnimSequence",physicsAsset:"PhysicsAsset",skeletalMesh:"SkeletalMesh",skeleton:"Skeleton"} and
  .sequenceLengths.idle == 2 and .sequenceLengths.moving == 1.5 and
  .rootMotion == {idle:false,moving:false} and
  .sharedSkeleton == "/Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seed_Skeleton.magi-p15-owned-seed_Skeleton"
' "$report" >/dev/null
echo "P1.5 owned animation seed: PASS ($(jq -r .source.glbSha256 "$manifest"))"
