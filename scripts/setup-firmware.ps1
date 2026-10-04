# Prepare firmware/snapclient for building:
#   1. Initialize the upstream submodule (and its nested submodules) at the pinned commit.
#   2. Apply firmware/patches/upstream-changes.patch, unless it is already applied.
#   3. Copy the overlay files (status_led component, speaker-stake sdkconfigs) into the submodule.
# Safe to run more than once.

$ErrorActionPreference = "Continue"
$root = Split-Path -Parent $PSScriptRoot
$fw = Join-Path $root "firmware\snapclient"
$patch = Join-Path $root "firmware\patches\upstream-changes.patch"
$overlay = Join-Path $root "firmware\overlay"

git -C $root submodule update --init --recursive
if ($LASTEXITCODE -ne 0) { throw "submodule init failed" }

git -C $fw apply --check $patch 2>$null
if ($LASTEXITCODE -eq 0) {
    git -C $fw apply $patch
    if ($LASTEXITCODE -ne 0) { throw "failed to apply patch" }
    Write-Output "Patch applied."
} else {
    git -C $fw apply --reverse --check $patch 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Output "Patch already applied."
    } else {
        throw "Patch does not apply cleanly. The submodule may have been moved off 5cda3a7."
    }
}

Copy-Item -Recurse -Force (Join-Path $overlay "components\status_led") (Join-Path $fw "components\")
Copy-Item -Force (Join-Path $overlay "sdkconfig.*") $fw
Write-Output "Overlay copied. Firmware is ready in firmware\snapclient."
