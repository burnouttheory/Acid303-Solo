# Outil de selection du compilateur Inno Setup lorsque son chemin est non standard.
# Ecrit uniquement le chemin du fichier choisi sur stdout pour CREER_INSTALLATEUR.bat.
try {
    Add-Type -AssemblyName System.Windows.Forms
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Title = 'Acid303 : selectionnez ISCC.exe (compilateur Inno Setup)'
    $dialog.Filter = 'Compilateur Inno Setup (ISCC.exe)|ISCC.exe|Programmes Windows (*.exe)|*.exe'
    $dialog.FileName = 'ISCC.exe'
    $dialog.CheckFileExists = $true
    if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        if ([System.IO.Path]::GetFileName($dialog.FileName) -ieq 'ISCC.exe') {
            [Console]::Out.WriteLine($dialog.FileName)
        }
    }
} catch {
    [Console]::Error.WriteLine('Selection automatique impossible. Ouvrez le projet .iss dans Inno Setup Compiler.')
}
