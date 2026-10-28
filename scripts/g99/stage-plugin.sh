#!/usr/bin/env bash
# Stage a pinned, LGPL-2.1 plugin without needing a second fork.
set -euo pipefail
renderer=$(cd "$(dirname "$0")/../.." && pwd)
plugin=$(cd "${1:?usage: stage-plugin.sh /path/to/plugin}" && pwd)
expected=f5105e4ddd1752cccdcc6421c402960d40ec3d9d
actual=$(git -C "$plugin" rev-parse HEAD)
[ "$actual" = "$expected" ] || { echo "Plugin source SHA mismatch" >&2; exit 1; }
# Only replace the empty/uninitialised submodule path in the separate checkout.
if [ -d "$plugin/MobileGlues" ] && [ ! -L "$plugin/MobileGlues" ]; then
    rmdir "$plugin/MobileGlues"
fi
if [ ! -e "$plugin/MobileGlues" ]; then
    ln -s "$renderer" "$plugin/MobileGlues"
fi
[ "$(readlink -f "$plugin/MobileGlues")" = "$renderer" ]
cp -a "$renderer/android-plugin-overlay/." "$plugin/"
mkdir -p "$plugin/app/src/main/assets/g99-source"
cp "$renderer/LICENSE" "$plugin/app/src/main/assets/g99-source/renderer-LICENSE.txt"
git -C "$plugin" show HEAD:LICENSE > "$plugin/app/src/main/assets/g99-source/plugin-LICENSE.txt"
printf '%s\n' "Renderer: t86952356-eng/MobileGlues-source @ $(git -C "$renderer" rev-parse HEAD)" \
    "Plugin: MobileGL-Dev/MobileGlues-plugin @ $expected" \
    "Overlay source: renderer/android-plugin-overlay" \
    "Experimental fork, not an official MobileGL-Dev release. No measured FPS claim." \
    > "$plugin/app/src/main/assets/g99-source/SOURCE.txt"
cp "$renderer/docs/G99-TEST-AR.md" "$plugin/app/src/main/assets/g99-source/README-AR.md"
