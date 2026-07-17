$root = Join-Path $PSScriptRoot "..\Middlewares\Third_Party\lvgl\src"
Get-ChildItem -LiteralPath $root -Recurse -Filter *.c |
    ForEach-Object {
        $_.FullName.Replace('\', '/')
    }
