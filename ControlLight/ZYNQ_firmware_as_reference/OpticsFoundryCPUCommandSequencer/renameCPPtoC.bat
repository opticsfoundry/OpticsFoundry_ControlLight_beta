Get-ChildItem *.cpp | ForEach-Object {
    $newName = $_.BaseName + ".c"
    Rename-Item -Path $_.FullName -NewName $newName
}
REM This script renames all .cpp files in the current directory to .c files.
REM Usage: Run this script in a PowerShell in the directory containing the .cpp files you want to rename.