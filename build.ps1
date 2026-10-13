[CmdletBinding()]
param(
    [ValidateSet('25', '75', 'All')]
    [string]$Milestone = 'All',
    [string]$Compiler = 'g++'
)

$ErrorActionPreference = 'Stop'
$compilerCommand = Get-Command -Name $Compiler -CommandType Application -ErrorAction Stop
$outputDirectory = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

# Explicit source lists ensure that each executable contains exactly one main.
$lexerSources = @('src/main_1.cpp', 'src/Lexer.cpp')
$semanticSources = @(
    'src/main_2.cpp', 'src/Lexer.cpp', 'src/LexicalValidation.cpp',
    'src/Parser.cpp', 'src/AST.cpp', 'src/Type.cpp',
    'src/SymbolTable.cpp', 'src/SemanticAnalyzer.cpp'
)
$compilerFlags = @('-std=c++17', '-Wall', '-Wextra', '-Wpedantic', '-O2', '-static', '-Isrc')

Push-Location -LiteralPath $PSScriptRoot
try {
    if ($Milestone -eq '25' -or $Milestone -eq 'All') {
        Write-Host 'Building 25% lexer demo...'
        & $compilerCommand.Source @compilerFlags @lexerSources '-o' (Join-Path $outputDirectory 'lexer_25.exe')
        if ($LASTEXITCODE -ne 0) { throw "25% build failed (compiler exit $LASTEXITCODE)." }
        Write-Host 'Built build/lexer_25.exe'
    }
    if ($Milestone -eq '75' -or $Milestone -eq 'All') {
        Write-Host 'Building 75% semantic analyzer...'
        & $compilerCommand.Source @compilerFlags @semanticSources '-o' (Join-Path $outputDirectory 'semantic_75.exe')
        if ($LASTEXITCODE -ne 0) { throw "75% build failed (compiler exit $LASTEXITCODE)." }
        Write-Host 'Built build/semantic_75.exe'
    }
} finally {
    Pop-Location
}
