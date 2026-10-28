## Cstitch on macOS ##

Requires an Apple Silicon Mac (M1 or later) running macOS 14.4 (Sonoma) or
later.  Intel Macs are not supported.

### Installing ###

1.  Download **Cstitch-YY.MM.N.dmg** from the
[Releases page](https://github.com/makiemak/Cstitch/releases).

2.  Open the dmg and drag **Cstitch** onto the **Applications** folder next to
it.

3.  Open Cstitch from Applications.  The app isn't notarized by Apple, so the
first time macOS says it can't verify it.  Click **Done**, then go to
**System Settings > Privacy & Security**, scroll down, and click
**Open Anyway** next to the Cstitch message.  After that it opens normally.

    (Terminal alternative: `xattr -dr com.apple.quarantine /Applications/Cstitch.app`)

### Building ###

1.  Install the build tools:

        xcode-select --install
        brew install librsvg

2.  Install Qt 6.12 from Qt's official binaries.  Homebrew's qt works for
local builds, but its libraries only run on the macOS version they were built
for, so a dmg made with it won't run on older systems.

        python3 -m venv ~/aqtvenv
        ~/aqtvenv/bin/pip install aqtinstall
        ~/aqtvenv/bin/aqt install-qt mac desktop 6.12.0 clang_64 --outputdir ~/Qt

3.  From the cstitch directory run

        ./build_mac.sh

    This renders the icons, generates the docs and the .pro file, compiles,
bundles Qt into **Cstitch.app**, signs it ad hoc, and writes
**Cstitch-<version>.dmg**.  Set `QTDIR` if Qt is somewhere other than
`~/Qt/6.12.0/macos`.

### Versioning and Releasing ###

Releases use calendar versions, **vYY.MM.PATCH**: the year and month of the
release, plus a patch number that starts at 0 and goes up for later releases
in the same month (v26.10.0, v26.10.1, ...).

The version is defined in one place: **VERSION.txt** (no leading v).
progen.py compiles it into the program and build_mac.sh puts it in the app's
Info.plist.  It is also saved into project files, which compare versions
numerically, so it must stay plain numbers.

    # 1. set the new version
    echo 26.11.0 > VERSION.txt

    # 2. commit and tag the release
    git commit -am "Release v26.11.0"
    git tag -a v26.11.0 -m "v26.11.0: <summary>"

    # 3. build (fails if the tag and VERSION.txt disagree)
    ./build_mac.sh

    # 4. publish
    git push origin mac-version
    git push origin v26.11.0
    gh release create v26.11.0 Cstitch-26.11.0.dmg --target mac-version \
      --title "Cstitch 26.11.0 for macOS" --notes "..."

Never re-tag a released version; bump to the next number instead.

Development happens on the **mac-version** branch.  **master** tracks
upstream and is protected against pushes.
