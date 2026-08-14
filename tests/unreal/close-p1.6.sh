#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
fail(){ echo "P1.6 closure failed: $*" >&2; exit 1; }
[[ $# == 6 || ( $# == 1 && $1 == --self-test ) ]] || { echo "Usage: $0 write|verify ARTIFACT RUN COMBINED REVIEW.json CLOSURE.json | --self-test" >&2; exit 2; }
if [[ ${1-} == --self-test ]]; then
  exec ruby "$repo_root/tests/unreal/support/p16-closure.rb" self-test "$repo_root"
fi
for command in ruby codesign lipo git; do command -v "$command" >/dev/null || fail "$command missing"; done
for name in P16_EXPECTED_ARTIFACT_SHA256 P16_EXPECTED_SOURCE_COMMIT P16_EXPECTED_RUN_INVENTORY_SHA256 P16_EXPECTED_COMBINED_EVIDENCE_TREE_SHA256 P16_EXPECTED_REVIEW_SHA256 P16_EXPECTED_REVIEWER_ID; do [[ -n ${!name:-} ]] || fail "$name required"; done
mode=$1; [[ "$mode" == write || "$mode" == verify ]] || fail "mode must be write or verify"
ruby "$repo_root/tests/unreal/support/p16-closure.rb" "$mode" "$2" "$3" "$4" "$5" "$6" || fail "closure gate"
printf 'P1.6 closure gate: PASS (%s)\n' "$mode"
