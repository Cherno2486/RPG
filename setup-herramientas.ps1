# =========================================================
# rpg-mazmorras - Setup de herramientas (Windows, corporate-friendly)
# =========================================================
# Como correrlo:
#   1) Abri PowerShell (da igual si es normal o "como administrador",
#      este script detecta cual de las dos sos y se adapta).
#   2) cd hasta la carpeta que tiene este archivo (rpg-mazmorras).
#   3) Ejecutalo con:  powershell -ExecutionPolicy Bypass -File .\setup-herramientas.ps1
#
# Que instala: Scoop (gestor de paquetes de usuario, sin tocar Program Files),
# y con Scoop: mingw (GCC/G++ para Windows), cmake, ninja y git.
#
# Nota sobre el paquete de GCC: se usa "mingw" (baja los binarios desde
# GitHub) en vez de "gcc" (baja desde nuwen.net). En redes corporativas
# nuwen.net suele estar bloqueado por el proxy/firewall (error 403), mientras
# que github.com casi siempre esta permitido.
#
# Nota sobre la politica de ejecucion: en compus corporativas suele haber una
# directiva de PowerShell fijada por GPO (Group Policy) que ni siquiera un
# administrador local puede cambiar con Set-ExecutionPolicy. Por eso este
# script YA NO intenta cambiarla: en vez de eso, corre siempre con el flag
# -ExecutionPolicy Bypass (como en el paso 3 de arriba), que alcanza para
# ejecutar este script puntual sin necesitar permiso para cambiar la politica
# global.

$esAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
Write-Host "Sesion elevada (administrador): $esAdmin" -ForegroundColor DarkGray

Write-Host "=== Paso 1/2: instalando Scoop (si no esta instalado) ===" -ForegroundColor Cyan
if (Get-Command scoop -ErrorAction SilentlyContinue) {
    Write-Host "Scoop ya estaba instalado, sigo." -ForegroundColor Yellow
} else {
    $instalador = Join-Path $env:TEMP "scoop-install.ps1"
    Invoke-RestMethod get.scoop.sh -OutFile $instalador

    if ($esAdmin) {
        # Scoop se niega a instalarse en una consola de administrador salvo que
        # se lo pidas explicitamente con -RunAsAdmin (por diseno: instalar
        # paquetes de usuario no deberia necesitar admin, pero en compus
        # corporativas a veces PowerShell abre elevado igual).
        Write-Host "Detecte sesion de administrador -> instalando con -RunAsAdmin" -ForegroundColor Yellow
        & $instalador -RunAsAdmin
    } else {
        & $instalador
    }

    # Refrescar PATH en esta sesion para que el comando 'scoop' este disponible
    # sin tener que cerrar y volver a abrir PowerShell.
    $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "User") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "Machine")
}

# Si "gcc" quedo instalado de un intento anterior pero roto (por el error de
# nuwen.net), lo sacamos antes de instalar "mingw" para que no choquen (los
# dos ponen gcc.exe/g++.exe en el PATH).
if (Get-Command scoop -ErrorAction SilentlyContinue) {
    $listaApps = scoop list 6>$null | Out-String
    if ($listaApps -match "(?m)^\s*gcc\s") {
        Write-Host "Saco 'gcc' (quedo con la descarga rota de nuwen.net) antes de instalar 'mingw'..." -ForegroundColor Yellow
        scoop uninstall gcc
    }
}

Write-Host "=== Paso 2/2: instalando mingw, cmake, ninja y git (uno por uno) ===" -ForegroundColor Cyan
$paquetes = @("mingw", "cmake", "ninja", "git")
$fallidos = @()
foreach ($pkg in $paquetes) {
    Write-Host "`n--- Instalando $pkg ---" -ForegroundColor Cyan
    scoop install $pkg
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FALLO instalando $pkg -- sigo con los demas paquetes." -ForegroundColor Red
        $fallidos += $pkg
    }
}

Write-Host "`n=== Verificacion ===" -ForegroundColor Green
gcc --version
cmake --version
git --version

if ($fallidos.Count -gt 0) {
    Write-Host "`nOjo: estos paquetes fallaron: $($fallidos -join ', '). Mandame el error de mas arriba de cada uno y lo resolvemos." -ForegroundColor Red
} else {
    Write-Host "`nListo. Si las 3 versiones se imprimieron arriba sin error, las herramientas estan OK." -ForegroundColor Green
}
Write-Host "Si algun comando dice 'no se reconoce como comando', cerra y volve a abrir PowerShell (para que tome el PATH nuevo) y corre este script de nuevo." -ForegroundColor Yellow
Write-Host "`nOjo: para compilar el proyecto (cmake / gcc) no hace falta tocar la politica de ejecucion de PowerShell para nada -- son programas .exe, no scripts. Esa politica solo afecta a archivos .ps1 como este." -ForegroundColor DarkGray
