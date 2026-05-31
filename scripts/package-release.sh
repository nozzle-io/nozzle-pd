#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "usage: scripts/package-release.sh PLATFORM PACKAGE_NAME PACKAGE_ROOT" >&2
  exit 2
fi

platform="$1"
package_name="$2"
package_root="$3"

case "$platform" in
  macos) ext=".pd_darwin" ;;
  linux) ext=".pd_linux" ;;
  windows) ext=".dll" ;;
  *) echo "unsupported platform: $platform" >&2; exit 2 ;;
esac

externals=(
  pix_nozzle_send
  pix_nozzle_receive
  pix_nozzle_gl_send
  pix_nozzle_gl_receive
)
help_patches=(
  pix_nozzle_send-help.pd
  pix_nozzle_receive-help.pd
  pix_nozzle_gl_send-help.pd
  pix_nozzle_gl_receive-help.pd
)

rm -rf package "$package_name" package-contents.txt
mkdir -p "package/$package_root"

for name in "${externals[@]}"; do
  src=".build/${name}${ext}"
  if [ ! -f "$src" ]; then
    echo "missing external: $src" >&2
    exit 1
  fi
  cp "$src" "package/$package_root/"
done

for patch in "${help_patches[@]}"; do
  src="help/${patch}"
  if [ ! -f "$src" ]; then
    echo "missing help patch: $src" >&2
    exit 1
  fi
  cp "$src" "package/$package_root/"
done

if [ -f README.md ]; then
  cp README.md "package/$package_root/"
fi
if [ -f LICENSE ]; then
  cp LICENSE "package/$package_root/"
elif [ -f LICENSE.md ]; then
  cp LICENSE.md "package/$package_root/"
fi

(
  cd package
  zip -r "../$package_name" "$package_root"
)

test -f "$package_name"
unzip -l "$package_name" > package-contents.txt

for name in "${externals[@]}"; do
  grep -F "${package_root}/${name}${ext}" package-contents.txt
done
for patch in "${help_patches[@]}"; do
  grep -F "${package_root}/${patch}" package-contents.txt
done
grep -F "${package_root}/README.md" package-contents.txt
