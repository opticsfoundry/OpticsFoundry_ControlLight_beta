Get-ChildItem *.c | ForEach-Object {
    $newName = $_.BaseName + ".cpp"
    Rename-Item -Path $_.FullName -NewName $newName
}
REM This script renames all .c files in the current directory to .cpp files.
REM Usage: Run this script in a PowerShell in the directory containing the .c files you want to rename.