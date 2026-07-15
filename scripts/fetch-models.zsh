#!/usr/bin/env zsh
# Download only the model assets pinned in models/*.json. Model binaries are
# intentionally ignored by Git; release packaging will fetch and verify them.
set -euo pipefail

ROOT="${0:A:h:h}"
MODEL_PATH="$ROOT/models/face_landmarker.task"
MODEL_URL="https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task"
EXPECTED_SHA256="64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff"

mkdir -p "$ROOT/models"
curl --fail --location --retry 3 --output "$MODEL_PATH" "$MODEL_URL"
ACTUAL_SHA256="$(shasum -a 256 "$MODEL_PATH" | awk '{print $1}')"

if [[ "$ACTUAL_SHA256" != "$EXPECTED_SHA256" ]]; then
  rm -f "$MODEL_PATH"
  print -u2 "Model checksum mismatch: expected $EXPECTED_SHA256, got $ACTUAL_SHA256"
  exit 1
fi

print "Verified $MODEL_PATH"
