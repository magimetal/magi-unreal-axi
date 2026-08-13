#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
manifest="$repo_root/tests/unreal/p1.6-manifest.json"
p15_manifest="$repo_root/tests/unreal/p1.5-manifest.json"
catalog="$repo_root/capabilities/catalog.json"
catalog_hash=a1f1906449ba158584f4b07f0adc0cccb4dba27df12f371e04aadb88452aae8f
cell=5.8.1-macos-arm64-56057345
fail() { echo "P1.6 kickoff verification failed: $*" >&2; exit 1; }
[[ -f "$manifest" && -f "$p15_manifest" && -f "$catalog" ]] || fail "manifest, P1.5 manifest, or catalog missing"
command -v jq >/dev/null || fail "jq missing"
command -v shasum >/dev/null || fail "shasum missing"
command -v ruby >/dev/null || fail "ruby missing"
ruby "$repo_root/tests/unreal/support/p16-proofs.rb" verify-plan "$repo_root/tests/unreal/p1.6-operation-proofs.json" "$manifest" "$catalog" "$repo_root" || fail "strict operation proof plan"

jq -e --slurpfile p15 "$p15_manifest" '
  .phase == "P1.6" and .status == "active" and .certificationComplete == false and
  .engine == {version:"5.8.1",changelist:56057345,hostArchitecture:"arm64"} and
  .catalog == {count:79,sha256:"a1f1906449ba158584f4b07f0adc0cccb4dba27df12f371e04aadb88452aae8f"} and
  .historicalManifests == {
    "P1.0":{file:"tests/unreal/p1.0-manifest.json",sha256:"c22bb2a272a657e46046cf1f8247b18c9d295b08a61184e155a966493ea84d74"},
    "P1.1":{file:"tests/unreal/p1.1-manifest.json",sha256:"83787d5a7f32296d682d10ccbdc7a7c5f7751702633477af9bc9e56145d645c1"},
    "P1.2":{file:"tests/unreal/p1.2-manifest.json",sha256:"6fd8b1c4e5d0cce554c2feece0a552e1d3c9999452859fe96e021215bdc5d9fe"},
    "P1.3":{file:"tests/unreal/p1.3-manifest.json",sha256:"ded6459db96db45d8f5b8c075d60c2d407cb816fadc2c2185b31e0bbcfeeef27"},
    "P1.4":{file:"tests/unreal/p1.4-manifest.json",sha256:"aa76e7672b0b627711f2712f434ae5ec72bcf9e25e7ba49db4527246ed52b224"},
    "P1.5":{file:"tests/unreal/p1.5-manifest.json",sha256:"dd8f7969abbe53327b8058d265677a35e08ca0218c6534e52f85e72001f52f18"}
  } and
  .automation.tests == $p15[0].automationTests and
  .automation.warningBaseline == $p15[0].knownRegressionWarnings and
  ([.automation.warningBaseline[].warnings]|add) == 10 and
  ([.traceability.baseline,.traceability.interaction,.traceability.ui,.traceability.ai,.traceability.animation]|map(length)) == [34,14,7,15,9] and
  ([.traceability|to_entries[]|.value]|add|unique|length) == 79 and
  (.p1Operations|length) == 45 and (.p1Operations|unique|length) == 45 and
  .plannedAgentJobs == ["unknown-project-orientation","interaction-loop","ui-state-loop","ai-navigation-loop","animation-state-loop"] and
  .releaseArchive == {
    namePattern:"magi-unreal-axi-{version}-macos-arm64.tar.gz",
    trustedDigestEnv:"P16_EXPECTED_ARTIFACT_SHA256",
    maxCompressedBytes:67108864,
    maxFileBytes:67108864,
    maxTotalUncompressedBytes:134217728,
    allowlist:["magi-unreal-axi-{version}-macos-arm64/","magi-unreal-axi-{version}-macos-arm64/magi-unreal-axi","magi-unreal-axi-{version}-macos-arm64/README.md","magi-unreal-axi-{version}-macos-arm64/LICENSE","magi-unreal-axi-{version}-macos-arm64/THIRD_PARTY_NOTICES.md","magi-unreal-axi-{version}-macos-arm64/CHANGELOG.md","magi-unreal-axi-{version}-macos-arm64/Cargo.lock","magi-unreal-axi-{version}-macos-arm64/skills/","magi-unreal-axi-{version}-macos-arm64/skills/magi-unreal-axi/","magi-unreal-axi-{version}-macos-arm64/skills/magi-unreal-axi/SKILL.md","magi-unreal-axi-{version}-macos-arm64/docs/","magi-unreal-axi-{version}-macos-arm64/docs/engine-support.md","magi-unreal-axi-{version}-macos-arm64/docs/agent-evaluation.md"]
  } and
  (.remainingGates|index("independent review is clean")) != null
' "$manifest" >/dev/null || fail "manifest shape or frozen contract"

actual_catalog_hash=$(shasum -a 256 "$catalog" | awk '{print $1}')
[[ "$actual_catalog_hash" == "$catalog_hash" ]] || fail "catalog hash changed"
while IFS=$'\t' read -r file expected; do
  [[ -f "$repo_root/$file" ]] || fail "historical manifest missing: $file"
  actual=$(shasum -a 256 "$repo_root/$file" | awk '{print $1}')
  [[ "$actual" == "$expected" ]] || fail "historical manifest hash: $file"
done < <(jq -r '.historicalManifests[]|[.file,.sha256]|@tsv' "$manifest")

jq -e --slurpfile catalog "$catalog" '
  ([.traceability|to_entries[]|.value[]]|sort) == ($catalog[0]|map(.id)|sort) and
  ([.traceability|to_entries[]|.value[]]|group_by(.)|all(length==1)) and
  (.p1Operations|sort) == ([.traceability.interaction[],.traceability.ui[],.traceability.ai[],.traceability.animation[]]|sort)
' "$manifest" >/dev/null || fail "traceability does not partition catalog and P1 operations exactly"
jq -e --arg cell "$cell" 'length == 79 and all(.[]; .engineSupport.certified == [$cell])' "$catalog" >/dev/null || fail "catalog certified-cell inventory"
if [[ ${1-} == --self-test ]]; then
  ruby "$repo_root/tests/unreal/support/p16-proofs.rb" self-test || fail "strict operation proof verifier self-test"
  ruby "$repo_root/tests/unreal/support/p16-archive.rb" self-test || fail "strict archive verifier self-test"
  printf 'P1.6 kickoff strict proof and archive verifier self-tests: PASS\n'
  exit 0
fi
artifact=${1-}
if [[ -z "$artifact" ]]; then
  [[ $# == 0 ]] || fail "usage: $0 [--self-test|ARTIFACT.tar.gz]"
  printf 'P1.6 kickoff: ACTIVE (certificationComplete=false); catalog=79, automation=38, warnings=10, traceability=79, P1 operations=45; artifact and combined gates pending\n'
  exit 0
fi
[[ $# == 1 && -f "$artifact" ]] || fail "usage: $0 [ARTIFACT.tar.gz]"
for command in tar file codesign cmp trash ruby; do command -v "$command" >/dev/null || fail "$command missing"; done
trusted_hash=${P16_EXPECTED_ARTIFACT_SHA256:-}
[[ $trusted_hash =~ ^[0-9a-fA-F]{64}$ ]] || fail "P16_EXPECTED_ARTIFACT_SHA256 must supply trusted candidate digest"
trusted_hash=$(printf '%s' "$trusted_hash" | tr '[:upper:]' '[:lower:]')
artifact=$(cd "$(dirname "$artifact")" && pwd -P)/$(basename "$artifact")
stage_dir=${P16_ARTIFACT_STAGE_DIR:-$(mktemp -d "$HOME/Library/Caches/magi-unreal-axi/p1.6/stage.XXXXXX")}
mkdir -p "$stage_dir"
staged_artifact="$stage_dir/$(basename "$artifact")"
if [[ ${P16_ARTIFACT_STAGED:-0} != 1 ]]; then
  cp -p -- "$artifact" "$staged_artifact"
  [[ -f "$(dirname "$artifact")/SHA256SUMS" ]] && cp -p -- "$(dirname "$artifact")/SHA256SUMS" "$stage_dir/SHA256SUMS"
fi
artifact="$staged_artifact"
version=$(sed -n 's/^version = "\([^"]*\)"/\1/p' "$repo_root/Cargo.toml" | head -1)
name="magi-unreal-axi-${version}-macos-arm64.tar.gz"
root="magi-unreal-axi-${version}-macos-arm64"
[[ $(basename "$artifact") == "$name" ]] || fail "exact artifact filename"
checksums=$(dirname "$artifact")/SHA256SUMS
[[ -f "$checksums" ]] || fail "adjacent SHA256SUMS missing"
expected_hash=$(awk -v name="$name" '$2==name && $1~/^[0-9a-fA-F]{64}$/ && NF==2{print tolower($1); count++} END{if(count!=1)exit 1}' "$checksums") || fail "unique adjacent checksum missing"
artifact_hash=$(shasum -a 256 "$artifact" | awk '{print $1}')
[[ "$artifact_hash" == "$expected_hash" && "$artifact_hash" == "$trusted_hash" ]] || fail "artifact, adjacent checksum, and trusted digest differ"
max_compressed=$(jq -r .releaseArchive.maxCompressedBytes "$manifest")
compressed_bytes=$(wc -c <"$artifact" | tr -d ' ')
[[ $compressed_bytes -le $max_compressed ]] || fail "compressed archive exceeds bound"

cache="$HOME/Library/Caches/magi-unreal-axi/p1.6/kickoff"
mkdir -p "$cache"
entries_file=$(mktemp "$cache/archive-list.XXXXXX")
work=$(mktemp -d "$cache/work.XXXXXX")
cleanup() {
  local status=$?
  trap - EXIT
  trash "$entries_file" "$work" >/dev/null 2>&1 || true
  [[ ${P16_ARTIFACT_STAGED:-0} == 1 ]] || trash "$stage_dir" >/dev/null 2>&1 || true
  exit "$status"
}
trap cleanup EXIT

ruby "$repo_root/tests/unreal/support/p16-archive.rb" verify "$artifact" "$manifest" "$version" || fail "strict archive verification"
tar -tzf "$artifact" >"$entries_file"
[[ -s "$entries_file" ]] || fail "empty archive"
tar -xzf "$artifact" -C "$work"

bin="$work/$root/magi-unreal-axi"
[[ -f "$bin" && -x "$bin" && ! -L "$bin" ]] || fail "binary missing, non-executable, or linked"
codesign --verify --strict --verbose=2 "$bin" >"$work/codesign.txt" 2>&1 || fail "strict codesign failed"
file -b "$bin" | grep -Eiq 'arm64|aarch64' || fail "binary is not arm64"
grep -aFq "$catalog_hash" "$bin" || fail "binary does not embed current catalog hash"
"$bin" --version | grep -Fxq "magi-unreal-axi $version" || fail "binary version mismatch"
while IFS=$'\t' read -r source packaged; do
  cmp -s "$repo_root/$source" "$work/$root/$packaged" || fail "packaged file differs from reviewed source: $source"
done <<'FILES'
README.md	README.md
CHANGELOG.md	CHANGELOG.md
LICENSE	LICENSE
THIRD_PARTY_NOTICES.md	THIRD_PARTY_NOTICES.md
Cargo.lock	Cargo.lock
skills/magi-unreal-axi/SKILL.md	skills/magi-unreal-axi/SKILL.md
docs/engine-support.md	docs/engine-support.md
docs/agent-evaluation.md	docs/agent-evaluation.md
FILES

evidence=$(mktemp -d "$cache/evidence.XXXXXX")
cp "$manifest" "$evidence/manifest.json"
cp "$entries_file" "$evidence/archive-list.txt"
cp "$work/codesign.txt" "$evidence/codesign.txt"
cp "$checksums" "$evidence/SHA256SUMS"
printf '%s\n' "$artifact" >"$evidence/artifact-path.txt"
printf '%s  %s\n' "$artifact_hash" "$name" >"$evidence/artifact-sha256.txt"
printf '%s\n' "$trusted_hash" >"$evidence/trusted-artifact-sha256.txt"
"$bin" --version >"$evidence/version.txt"
printf 'phase=P1.6\nstatus=active\ncertificationComplete=false\nslice=archive-contract-and-traceability\ncatalogTraceability=79/79\np1Operations=45/45\nautomationInventory=38\nwarningBaseline=10-exact\narchivePreflight=passed\ntrustedDigest=matched\nremaining=native38,orientation,interaction,ui,ai,animation,combined-package,docs,independent-review\n' >"$evidence/summary.txt"
printf 'artifactSha256=%s\n' "$artifact_hash" >"$evidence/statement.txt"
tree_sha256=$(ruby "$repo_root/tests/unreal/support/p16-evidence.rb" write "$evidence" "$evidence/evidence-tree.json")
ruby "$repo_root/tests/unreal/support/p16-evidence.rb" verify "$evidence" "$evidence/evidence-tree.json" >/dev/null
printf 'evidence tree sha256=%s (returned outside evidence root)\n' "$tree_sha256"
printf 'P1.6 kickoff: ACTIVE (certificationComplete=false); artifact preflight evidence retained at %s; combined gates pending\n' "$evidence"
