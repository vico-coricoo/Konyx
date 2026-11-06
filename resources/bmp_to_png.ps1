param(
    [Parameter(Mandatory=$true)]
    [string]$InputFile,

    [string]$OutputFile = [System.IO.Path]::ChangeExtension($InputFile, ".png"),

    [string]$TransparentColor = "#3B4453"
)

Add-Type -AssemblyName System.Drawing

try {
    # Vérification du fichier source
    if (!(Test-Path $InputFile)) {
        throw "Fichier introuvable : $InputFile"
    }

    # Lecture de la couleur à rendre transparente
    $ColorHex = $TransparentColor.TrimStart('#')

    if ($ColorHex -notmatch '^[0-9A-Fa-f]{6}$') {
        throw "Couleur invalide. Utilise le format #RRGGBB."
    }

    $R = [Convert]::ToInt32($ColorHex.Substring(0,2),16)
    $G = [Convert]::ToInt32($ColorHex.Substring(2,2),16)
    $B = [Convert]::ToInt32($ColorHex.Substring(4,2),16)

    # Chargement de l'image
    $Image = [System.Drawing.Bitmap]::FromFile(
        (Resolve-Path $InputFile).Path
    )

    # Création d'une image avec canal alpha
    $Output = New-Object System.Drawing.Bitmap(
        $Image.Width,
        $Image.Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    )

    # Parcours des pixels
    for ($x = 0; $x -lt $Image.Width; $x++) {
        for ($y = 0; $y -lt $Image.Height; $y++) {

            $Pixel = $Image.GetPixel($x, $y)

            if (
                $Pixel.R -eq $R -and
                $Pixel.G -eq $G -and
                $Pixel.B -eq $B
            ) {
                # Pixel transparent
                $NewPixel = [System.Drawing.Color]::FromArgb(0, $R, $G, $B)
            }
            else {
                # Pixel conservé
                $NewPixel = $Pixel
            }

            $Output.SetPixel($x, $y, $NewPixel)
        }
    }

    # Export PNG
    $Output.Save(
        $OutputFile,
        [System.Drawing.Imaging.ImageFormat]::Png
    )

    Write-Host "Conversion terminee !" -ForegroundColor Green
    Write-Host "Fichier cree : $OutputFile"

}
catch {
    Write-Host "Erreur : $_" -ForegroundColor Red
}
finally {
    if ($Image) { $Image.Dispose() }
    if ($Output) { $Output.Dispose() }
}
