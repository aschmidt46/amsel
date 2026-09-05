# Benötigt strip, uname und ResourceHacker in PATH

New-Item -ItemType Directory -Force -Path "./release" | out-null
New-Item -ItemType Directory -Force -Path "./release/locales" | out-null
New-Item -ItemType Directory -Force -Path "./release/saves" | out-null
New-Item -ItemType Directory -Force -Path "./release/licenses" | out-null

$locales = "./locales/*"
# Get-Item -Path $locales |
Copy-Item  -Path $locales -Destination "./release/locales/" -Recurse -force
Copy-Item  -Path "AMSEL.exe" -Destination "./release/AMSEL.exe" -Recurse -force
Copy-Item  -Path "palette.pal" -Destination "./release/palette.pal" -Recurse -force

strip "./release/AMSEL.exe"

ResourceHacker -open "./release/AMSEL.exe" -save "./release/AMSEL.exe" -resource "../resources/amsel.ico" -mask ICONGROUP,MAINICON,0 -action addskip -log CONSOLE

#Rust-Lizenzen

$current = Get-Location

Set-Location -Path "../src/cgb"

$cargo_licenses = cargo-bundle-licenses --format yaml

Set-Location -Path $current

$cargo_licenses | Out-File "./release/licenses/THIRDPARTY-cargo.yml"

Copy-Item  -Path "../LICENSE" -Destination "./release/licenses/LICENSE.txt" -Recurse -force
Copy-Item  -Path "../scripts/THIRDPARTY.txt" -Destination "./release/licenses/THIRDPARTY.txt" -Recurse -force


$arch = uname -m

$compress = @{
  Path = "./release/*"
  CompressionLevel = "Fastest"
  DestinationPath = "./release_windows_" + $arch + ".zip"
}
Compress-Archive -Force @compress
