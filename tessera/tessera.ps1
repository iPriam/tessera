# Runs the Tessera build from Windows by forwarding to the WSL build host.
#
#     .\tessera.ps1 boot -arch x86_64 -test
#
# Its arguments are `kira build`'s.
#
# The build itself only ever runs inside Linux. This wrapper exists so that
# working from a Windows shell does not mean remembering the wsl.exe incantation
# and the /mnt/c path translation every time.

# No param block: PowerShell would take `-arch` for a parameter of its own.
# A script without one hands every argument, dash or not, to $args.
$ErrorActionPreference = 'Stop'

$Distro = if ($env:TESSERA_WSL_DISTRO) { $env:TESSERA_WSL_DISTRO } else { 'Fedora' }

# The repository's own path, translated to where WSL sees it.
$RepoWindows = $PSScriptRoot
$RepoLinux = (wsl -d $Distro -- wslpath -a ($RepoWindows -replace '\\', '/'))
if ($LASTEXITCODE -ne 0) {
    throw "could not translate $RepoWindows into a WSL path (is the '$Distro' distribution installed?)"
}
$RepoLinux = $RepoLinux.Trim()

wsl -d $Distro --cd "$RepoLinux" -- kira build @args
exit $LASTEXITCODE
