# Benötigt strip, uname und ResourceHacker in PATH

New-Item -ItemType Directory -Force -Path "./release" | out-null
New-Item -ItemType Directory -Force -Path "./release/locales" | out-null
New-Item -ItemType Directory -Force -Path "./release/saves" | out-null

$locales = "./locales/*"
# Get-Item -Path $locales |
Copy-Item  -Path $locales -Destination "./release/locales/" -Recurse -force
Copy-Item  -Path "AMSEL.exe" -Destination "./release/AMSEL.exe" -Recurse -force
Copy-Item  -Path "palette.pal" -Destination "./release/palette.pal" -Recurse -force

strip "./release/AMSEL.exe"

ResourceHacker -open "./release/AMSEL.exe" -save "./release/AMSEL.exe" -resource "../resources/amsel.ico" -mask ICONGROUP,MAINICON,0 -action addskip -log CONSOLE

$arch = uname -m

$compress = @{
  Path = "./release/*"
  CompressionLevel = "Fastest"
  DestinationPath = "./release_windows_" + $arch + ".zip"
}
Compress-Archive -Force @compress
