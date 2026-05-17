# Benötigt strip und ResourceHacker in PATH

New-Item -ItemType Directory -Force -Path "./release"
New-Item -ItemType Directory -Force -Path "./release/locales"
New-Item -ItemType Directory -Force -Path "./release/saves"

$locales = "./locales/*"
# Get-Item -Path $locales |
Copy-Item  -Path $locales -Destination "./release/locales/" -Recurse -force
Copy-Item  -Path "glfw3.dll" -Destination "./release/glfw3.dll" -Recurse -force
Copy-Item  -Path "glew32.dll" -Destination "./release/glew32.dll" -Recurse -force
Copy-Item  -Path "AMSEL.exe" -Destination "./release/AMSEL.exe" -Recurse -force
Copy-Item  -Path "palette.pal" -Destination "./release/palette.pal" -Recurse -force

strip "./release/AMSEL.exe"

ResourceHacker -open "./release/AMSEL.exe" -save "./release/AMSEL.exe" -resource "../resources/amsel.ico" -mask ICONGROUP,MAINICON,0 -action addskip -log CONSOLE
