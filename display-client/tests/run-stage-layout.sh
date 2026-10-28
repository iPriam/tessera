#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
probe=$(mktemp -d /tmp/tessera-stage-layout.XXXXXX)
trap 'rm -rf "$probe"' EXIT HUP INT TERM
mkdir "$probe/app"
cp "$root/app/Channel/Stage.kira" "$probe/app/Stage.kira"
cp "$root/tests/StageLayout.kira" "$probe/app/main.kira"
cat > "$probe/package.kira" <<'KIRA'
construct StageLayoutTests extends Package {
    let kind: PackageKind = .App
}
KIRA
cd "$probe"
kira run .
