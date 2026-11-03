#!/usr/bin/env bash
# Validates the built plugin (HARN-02). The same script runs locally and in CI.
# Usage: scripts/validate.sh [build dir]
set -euo pipefail

BUILD="${1:-build}"
ART="$BUILD/Kikkuh_artefacts/Release"
TOOLS="$BUILD/tools"
PLUGINVAL_VERSION=v1.0.4
CLAP_VALIDATOR_VERSION=0.3.2
mkdir -p "$TOOLS"

echo "== architecture"
for binary in "$ART/VST3/Kikkuh.vst3/Contents/MacOS/Kikkuh" \
              "$ART/AU/Kikkuh.component/Contents/MacOS/Kikkuh" \
              "$ART/CLAP/Kikkuh.clap/Contents/MacOS/Kikkuh"; do
    lipo -archs "$binary" | grep -qw arm64 || { echo "no arm64 in $binary"; exit 1; }
done

echo "== pluginval, strictness 10"
if [ ! -x "$TOOLS/pluginval.app/Contents/MacOS/pluginval" ]; then
    curl -fsSL -o "$TOOLS/pluginval.zip" "https://github.com/Tracktion/pluginval/releases/download/$PLUGINVAL_VERSION/pluginval_macOS.zip"
    unzip -qo "$TOOLS/pluginval.zip" -d "$TOOLS"
fi
"$TOOLS/pluginval.app/Contents/MacOS/pluginval" --strictness-level 10 --validate-in-process "$ART/VST3/Kikkuh.vst3"

echo "== clap-validator"
if [ ! -x "$TOOLS/binaries/clap-validator" ]; then
    curl -fsSL "https://github.com/free-audio/clap-validator/releases/download/$CLAP_VALIDATOR_VERSION/clap-validator-$CLAP_VALIDATOR_VERSION-macos-universal.tar.gz" | tar xz -C "$TOOLS"
fi
"$TOOLS/binaries/clap-validator" validate "$ART/CLAP/Kikkuh.clap"

echo "== auval (needs the component installed by the build)"
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aumu Kik1 Cmot

echo "validate ok"
