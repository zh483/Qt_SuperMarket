$word = New-Object -ComObject Word.Application
$word.Visible = $false
$docPath = Join-Path $PWD "需求分析和数据库设计格式参考2026.doc"
$outPath = Join-Path $PWD "reference_doc_ps.txt"
$doc = $word.Documents.Open($docPath)
$text = $doc.Content.Text
$doc.Close()
$word.Quit()
[System.IO.File]::WriteAllText($outPath, $text, [System.Text.Encoding]::UTF8)
Write-Output "Extracted to $outPath"
