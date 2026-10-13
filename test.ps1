# Run from Windows PowerShell 5.1 or PowerShell 7. No temporary input files are needed.
[CmdletBinding()]
param(
    [ValidateSet('25', '75', 'All')]
    [string]$Milestone = 'All',
    [switch]$SkipBuild,
    [ValidateRange(1, 60)]
    [int]$TimeoutSeconds = 5
)

$ErrorActionPreference = 'Stop'

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Milestone $Milestone
    if (-not $?) { throw 'Build failed; tests were not run.' }
}

$script:Passed = 0
$script:Failed = 0
$lexer = Join-Path $PSScriptRoot 'build\lexer_25.exe'
$analyzer = Join-Path $PSScriptRoot 'build\semantic_75.exe'

# ProcessStartInfo.ArgumentList is unavailable in Windows PowerShell 5.1.
# This fallback quotes arguments for the Windows executable API; it never runs a shell.
function ConvertTo-ProcessArgument {
    param([AllowEmptyString()][string]$Argument)
    $escaped = [regex]::Replace($Argument, '(\\*)"', '$1$1\"')
    $escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
    return '"' + $escaped + '"'
}

function Invoke-Program {
    param(
        [string]$Executable,
        [string[]]$Arguments = @(),
        [AllowEmptyString()][string]$InputText = ''
    )
    if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
        throw "Executable not found: $Executable. Run build.ps1 first."
    }
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $Executable
    $startInfo.WorkingDirectory = $PSScriptRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.RedirectStandardInput = $true
    if ($null -ne $startInfo.PSObject.Properties['ArgumentList']) {
        foreach ($argument in $Arguments) { $startInfo.ArgumentList.Add($argument) }
    } else {
        $startInfo.Arguments = (($Arguments | ForEach-Object {
            ConvertTo-ProcessArgument -Argument $_
        }) -join ' ')
    }
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if ($InputText.Length -gt 0) { $process.StandardInput.Write($InputText) }
        $process.StandardInput.Close()
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $process.Kill()
            $process.WaitForExit()
            throw "Timed out after $TimeoutSeconds seconds."
        }
        $process.WaitForExit()
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Output = $stdout.Result + $stderr.Result
        }
    } finally {
        $process.Dispose()
    }
}

function Test-Program {
    param(
        [string]$Name,
        [string]$Executable = $analyzer,
        [string[]]$Arguments = @(),
        [AllowEmptyString()][string]$InputText = '',
        [int]$ExpectedExit = 0,
        [ValidateSet('None', 'LEXICAL', 'SYNTAX', 'SEMANTIC', 'Legacy')]
        [string]$Phase = 'None',
        [int]$DiagnosticCount = 0,
        [string[]]$Contains = @(),
        [string[]]$Excludes = @()
    )
    $problems = New-Object 'System.Collections.Generic.List[string]'
    $result = $null
    try {
        $result = Invoke-Program -Executable $Executable -Arguments $Arguments -InputText $InputText
        if ($result.ExitCode -ne $ExpectedExit) {
            $problems.Add("Expected exit $ExpectedExit; got $($result.ExitCode).")
        }
        if ($Phase -ne 'Legacy') {
            $allErrors = [regex]::Matches($result.Output, '\[(?:LEXICAL|SYNTAX|SEMANTIC) ERROR\]').Count
            if ($allErrors -ne $DiagnosticCount) {
                $problems.Add("Expected $DiagnosticCount total diagnostics; got $allErrors.")
            }
            if ($Phase -ne 'None') {
                $phaseErrors = [regex]::Matches($result.Output, "\[$Phase ERROR\]").Count
                if ($phaseErrors -ne $DiagnosticCount) {
                    $problems.Add("Expected $DiagnosticCount $Phase diagnostics; got $phaseErrors.")
                }
                $reasons = [regex]::Matches($result.Output, '(?m)^\s*Reason:').Count
                $corrections = [regex]::Matches($result.Output, '(?m)^\s*Possible correction:').Count
                if ($reasons -ne $DiagnosticCount -or $corrections -ne $DiagnosticCount) {
                    $problems.Add('Every diagnostic must contain a reason and a possible correction.')
                }
                if ($result.Output -notmatch '(?m)^\s*\^') {
                    $problems.Add('Expected a source caret in the diagnostic output.')
                }
            }
        }
        foreach ($pattern in $Contains) {
            if ($result.Output -notmatch $pattern) { $problems.Add("Missing output pattern: $pattern") }
        }
        foreach ($pattern in $Excludes) {
            if ($result.Output -match $pattern) { $problems.Add("Unexpected output pattern: $pattern") }
        }
    } catch {
        $problems.Add($_.Exception.Message)
    }
    if ($problems.Count -eq 0) {
        $script:Passed++
        Write-Host "PASS $Name" -ForegroundColor Green
    } else {
        $script:Failed++
        Write-Host "FAIL $Name" -ForegroundColor Red
        foreach ($problem in $problems) { Write-Host "  $problem" }
        if ($null -ne $result) { Write-Host $result.Output }
    }
}

if ($Milestone -in @('25', 'All')) {
    Test-Program -Name '25% declarations and character token' -Executable $lexer -Phase Legacy `
        -Arguments @((Join-Path $PSScriptRoot 'tests\test1_declarations.txt')) `
        -Contains @('CHAR_LITERAL', 'Lexical errors: 0', 'KEYWORD_FLOAT')
    Test-Program -Name '25% operators and source positions' -Executable $lexer -Phase Legacy `
        -Arguments @((Join-Path $PSScriptRoot 'tests\test2_operators.txt')) `
        -Contains @('Lexical errors: 0', '\*', '&&', '>=', '!=')
    Test-Program -Name '25% unknown character explanation' -Executable $lexer -Phase Legacy `
        -Arguments @((Join-Path $PSScriptRoot 'tests\test3_unknown_character.txt')) -ExpectedExit 1 `
        -Contains @('Lexical errors: 1', "unrecognized character '@'", 'Reason:', 'Possible correction:', '\^')
}

if ($Milestone -in @('75', 'All')) {
    $fixtures = Join-Path $PSScriptRoot 'tests\semantic'
    Test-Program -Name '75% basic pipeline, tokens, AST, types and scopes' `
        -Arguments @('--all', (Join-Path $fixtures 'test1_valid_basic.txt')) `
        -Contains @('75% milestone', 'KEYWORD_INT', 'EOF', 'Program', 'BinaryExpr\(\* : float\)',
            'BinaryExpr\(&& : bool\)', 'Scope ID:', 'Function main', 'globalCount')
    Test-Program -Name '75% nested scopes and shadowing' `
        -Arguments @('--dump-symbols', (Join-Path $fixtures 'test2_valid_scopes.txt')) `
        -Contains @('Function testScopes', 'Block@L', '(?s)Scope ID:.*Scope ID:.*Scope ID:.*Scope ID:')
    Test-Program -Name '75% functions, arrays and int-to-float widening' `
        -Arguments @('--dump-typed-ast', (Join-Path $fixtures 'test3_valid_functions.txt')) `
        -Contains @('ArrayAccessExpr\(arr : int\)', 'CallExpr\(average : float\)', 'CallExpr\(printStatus : void\)')

    $negativeFixtures = @(
        @{ File = 'test4_error_undeclared.txt'; Count = 3; Patterns = @("Undeclared identifier 'x'", "Undeclared identifier 'z'", "undeclared function 'foo'") },
        @{ File = 'test5_error_redeclared.txt'; Count = 4; Patterns = @("Redeclaration of global variable 'x'", "Duplicate parameter name 'val'", "Redeclaration of identifier 'count'", "Redeclaration of function 'compute'") },
        @{ File = 'test6_error_type_mismatch.txt'; Count = 6; Patterns = @('Type mismatch in assignment', 'Type mismatch in variable initialization', 'requires numeric operands', 'requires integer operands', 'Cannot compare incompatible types') },
        @{ File = 'test7_error_condition.txt'; Count = 2; Patterns = @("Condition in 'if'", "Condition in 'while'", "must evaluate to 'bool'") },
        @{ File = 'test8_error_function_args.txt'; Count = 4; Patterns = @('expects 2 arguments, but 1', 'expects 2 arguments, but 3', "Argument 1 of function 'configure'", "Argument 2 of function 'configure'") },
        @{ File = 'test9_error_return_mismatch.txt'; Count = 4; Patterns = @("Void function 'doWork'", "Non-void function 'getValue'", "function 'calculate'", "function 'checkFlag'") },
        @{ File = 'test10_error_invalid_lvalue.txt'; Count = 5; Patterns = @("Cannot assign to function 'helper'", "entire array 'arr'", "'notArray' is not an array", "index must be of type 'int', got 'float'", "index must be of type 'int', got 'bool'") }
    )
    foreach ($fixture in $negativeFixtures) {
        Test-Program -Name $fixture.File -Arguments @((Join-Path $fixtures $fixture.File)) `
            -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount $fixture.Count -Contains $fixture.Patterns
    }

    Test-Program -Name 'Forward calls and mutual recursion' -Arguments @('--dump-typed-ast') -InputText @'
bool even(int n) { if (n == 0) { return true; } else { return odd(n - 1); } }
bool odd(int n) { if (n == 0) { return false; } else { return even(n - 1); } }
int main() { bool result = even(4); return later(2); }
int later(int n) { return n + 1; }
'@ -Contains @('CallExpr\(odd : bool\)', 'CallExpr\(even : bool\)', 'CallExpr\(later : int\)')

    Test-Program -Name 'Missing non-void return' -InputText 'int f() { int x = 1; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("function 'f' may reach the end", "type 'int'")
    Test-Program -Name 'Return in only one branch' -InputText 'int f(bool flag) { if (flag) { return 1; } }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("function 'f' may reach the end")
    Test-Program -Name 'Return only inside a loop' -InputText 'int f(bool flag) { while (flag) { return 1; } }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("function 'f' may reach the end")
    Test-Program -Name 'All branches return' `
        -InputText 'int f(bool flag) { if (flag) { return 1; } else { return 2; } }'
    Test-Program -Name 'Loop followed by unconditional return' `
        -InputText 'int f(bool flag) { while (flag) { return 1; } return 0; }'
    Test-Program -Name 'Reject void arrays in global and local scopes' `
        -InputText 'void a[2]; int main() { void b[3]; return 0; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 2 `
        -Contains @("Array 'a' cannot have element type 'void'", "Array 'b' cannot have element type 'void'")
    Test-Program -Name 'Reject void variables without duplicate diagnostics' `
        -InputText 'void x; int main() { void y; return 0; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 2 `
        -Contains @("Variable 'x'.*type 'void'", "Variable 'y'.*type 'void'")
    Test-Program -Name 'Reject void parameter without duplicate diagnostics' `
        -InputText 'int f(void x) { return 0; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("Parameter 'x'.*type 'void'")
    Test-Program -Name 'Logical, relational and unary operand rules' `
        -InputText 'int main() { bool a = 1 && true; bool b = 1 < true; int c = -false; bool d = !1; return 0; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 4 `
        -Contains @('Logical operator', 'Relational operator', 'Unary operator', 'Logical NOT operator')
    Test-Program -Name 'Undeclared array index emits one primary error' `
        -InputText 'int main() { int a[2]; int x = a[missing]; return 0; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("Undeclared identifier 'missing'") `
        -Excludes @('Array subscript index must', 'Type mismatch')
    Test-Program -Name 'Undeclared assignment index emits one primary error' `
        -InputText 'int main() { int a[2]; a[missing] = 1; return 0; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("Undeclared identifier 'missing'") `
        -Excludes @('Array subscript index must', 'Type mismatch')
    Test-Program -Name 'Variable leaves scope at closing brace' `
        -InputText 'int main() { { int x = 1; } x = 2; return 0; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("Undeclared identifier 'x'")
    Test-Program -Name 'Function body and parameters share a scope' `
        -InputText 'int f(int x) { int x = 1; return x; }' `
        -ExpectedExit 1 -Phase SEMANTIC -DiagnosticCount 1 -Contains @("Redeclaration of identifier 'x'")
    Test-Program -Name 'Arithmetic precedence is reflected in typed AST' -Arguments @('--dump-typed-ast') `
        -InputText 'int main() { return 1 + 2 * 3; }' `
        -Contains @('BinaryExpr\(\+ : int\)\r?\n\s+LiteralExpr\(1 : int\)\r?\n\s+BinaryExpr\(\* : int\)')
    Test-Program -Name 'Empty stdin is a valid empty program' -Arguments @('--dump-tokens') -InputText '' `
        -Contains @('EOF')
    Test-Program -Name 'Comments-only input is a valid empty program' -InputText "// comment`n/* another */"
    Test-Program -Name 'Character and supported escape literals' `
        -InputText 'int main() { char c = ''A''; char newline = ''\n''; return 0; }'
    Test-Program -Name '75% CLI help' -Arguments @('--help') -Contains @('--dump-typed-ast', '--dump-symbols', '--all')

    $syntaxCases = @(
        @{ Name = 'Reject global return as syntax'; Source = 'return 0;'; Pattern = 'Expected top-level declaration' },
        @{ Name = 'Missing initializer expression'; Source = 'int main() { int x = ; return 0; }'; Pattern = 'Expected expression' },
        @{ Name = 'Missing closing brace at EOF'; Source = 'int main() { return 0;'; Pattern = "Expected '\}' after block" },
        @{ Name = 'Missing if body before closing brace'; Source = 'int main() { if (true) }'; Pattern = 'Expected a statement before' },
        @{ Name = 'Missing semicolon recovers to next declaration'; Source = 'int main() { int x = 1 int y = 2; return 0; }'; Pattern = 'Expected.*;' },
        @{ Name = 'Invalid assignment target'; Source = 'int main() { (1 + 2) = 3; return 0; }'; Pattern = 'assignment target|left-hand side|left side' },
        @{ Name = 'Trailing comma in call'; Source = 'int main() { f(1,); return 0; }'; Pattern = 'Expected expression' }
    )
    foreach ($case in $syntaxCases) {
        Test-Program -Name $case.Name -InputText $case.Source -ExpectedExit 1 -Phase SYNTAX `
            -DiagnosticCount 1 -Contains @($case.Pattern)
    }
    Test-Program -Name 'Unknown single ampersand explains logical AND' `
        -InputText 'int main() { bool x = true & false; return 0; }' `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('&&')
    Test-Program -Name 'Unterminated block comment is lexical' -InputText '/* unfinished' `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('(?i)unterminated|not closed')
    Test-Program -Name 'Empty character literal is lexical' -InputText "char c = '';" `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('(?i)character literal')
    Test-Program -Name 'Multiple characters in character literal are lexical' -InputText "char c = 'ab';" `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('(?i)character literal')
    Test-Program -Name 'Unterminated character literal is lexical' -InputText "char c = 'a" `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('(?i)character literal')
    Test-Program -Name 'Unsupported character escape is lexical' -InputText "char c = '\q';" `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('(?i)escape')
    Test-Program -Name 'Unsupported prefix increment is lexical' -InputText 'int f(int x) { return ++x; }' `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('Unsupported operator.*\+\+')
    Test-Program -Name 'Unsupported postfix decrement is lexical' -InputText 'int f(int x) { x--; return x; }' `
        -ExpectedExit 1 -Phase LEXICAL -DiagnosticCount 1 -Contains @('Unsupported operator.*--')
    Test-Program -Name 'Separated unary operators remain valid' -Arguments @('--dump-typed-ast') `
        -InputText 'int f(int x) { return + +x; }' -Contains @('UnaryExpr\(\+ : int\)')
}

Write-Host "`nTests: $($script:Passed + $script:Failed) | Passed: $script:Passed | Failed: $script:Failed"
if ($script:Failed -gt 0) { exit 1 }
exit 0
