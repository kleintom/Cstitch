#!/bin/sh
# Build Cstitch.app and a drag-to-Applications Cstitch-<version>.dmg for
# Apple Silicon Macs.  Requires the official Qt 6.12 binaries (not Homebrew's
# qt, whose libraries only run on the macOS they were built on) and librsvg.
# See INSTALL_mac.md.
set -e
cd "$(dirname "$0")"

QTDIR=${QTDIR:-$HOME/Qt/6.12.0/macos}
if [ ! -x "$QTDIR/bin/qmake" ]; then
  echo "Qt not found in $QTDIR (set QTDIR, see INSTALL_mac.md)" >&2
  exit 1
fi

version=$(cat VERSION.txt)

# a release tag must match VERSION.txt (uncommitted changes aren't the
# tagged release, so only check a clean tree)
tag=$(git tag --points-at HEAD 'v*' | head -1)
if [ -n "$tag" ] && git diff --quiet HEAD && [ "$tag" != "v$version" ]; then
  echo "HEAD is tagged $tag but VERSION.txt says $version" >&2
  exit 1
fi

echo "== Building Cstitch $version"

# start from a clean tree so qmake -project doesn't pick up generated files
[ -f Makefile ] && make distclean > /dev/null 2>&1 || true
rm -rf Cstitch.app Cstitch.pro Cstitch-*.dmg

./icons/svgToPng.sh
python3 doc/createDocs.py
"$QTDIR/bin/qmake" -project -o Cstitch.pro
python3 progen.py
"$QTDIR/bin/qmake"
make -j"$(sysctl -n hw.ncpu)"

plist=Cstitch.app/Contents/Info.plist
/usr/libexec/PlistBuddy \
  -c "Add :CFBundleName string Cstitch" \
  -c "Add :CFBundleShortVersionString string $version" \
  -c "Add :CFBundleVersion string $version" \
  -c "Add :NSHighResolutionCapable bool true" \
  "$plist"

"$QTDIR/bin/macdeployqt" Cstitch.app
# Qt's frameworks are universal; keep only the arm64 slices
find Cstitch.app -type f | while read -r f; do
  if lipo -archs "$f" 2>/dev/null | grep -q x86_64; then
    lipo -thin arm64 "$f" -output "$f"
  fi
done
# ad-hoc signature (no Apple Developer ID): users approve the first launch
# in System Settings > Privacy & Security
codesign --force --deep -s - Cstitch.app

arch=$(lipo -archs Cstitch.app/Contents/MacOS/Cstitch)
if [ "$arch" != "arm64" ]; then
  echo "expected an arm64 binary, got: $arch" >&2
  exit 1
fi
if otool -L Cstitch.app/Contents/MacOS/Cstitch | grep -qE "/opt/homebrew|$QTDIR"; then
  echo "Cstitch.app links against libraries outside the bundle" >&2
  exit 1
fi

stage=$(mktemp -d)
cp -R Cstitch.app "$stage/"
ln -s /Applications "$stage/Applications"
hdiutil create -volname "Cstitch $version" -srcfolder "$stage" -ov \
  -format UDZO "Cstitch-$version.dmg"
rm -rf "$stage"

echo "== Built Cstitch-$version.dmg"
