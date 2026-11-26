# qa_test 回归脚本：生成 stickhub 设计块并调用本地 BOM 服务
#
# 前置条件：
#   1. 已完成构建，bin/design_block_json.exe 存在
#   2. 本地 BOM 服务已启动（默认 http://127.0.0.1:8080/get_bom_with_all_component_attrs）
#
# 用法：
#   powershell -ExecutionPolicy Bypass -File scripts\qa_test.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\qa_test.ps1 -DemoDir "E:\kicad\demos\cm5_minima"

param(
    [string]$DemoDir = 'E:\kicad\demos\stickhub',
    [string]$Url     = 'http://127.0.0.1:8080/get_bom_with_all_component_attrs'
)

$ErrorActionPreference = 'Stop'
$env:PYTHONIOENCODING = 'utf-8'

$root = Split-Path -Parent $PSScriptRoot
$exe  = Join-Path $root 'bin\design_block_json.exe'
$qa   = Join-Path $root 'qa_test'

if (-not (Test-Path $exe)) {
    throw "未找到 $exe，请先构建项目"
}

New-Item -ItemType Directory -Force -Path $qa | Out-Null
$jsonPath = Join-Path $qa 'design_block.json'
$b64Path  = Join-Path $qa 'design_block.json.base64'
$bomPath  = Join-Path $qa 'bom_response.json'
$rawPath  = Join-Path $qa 'bom_response.json.base64'
$bomJsonInputPath = Join-Path $qa 'bom_response_from_json.json'
$rawJsonInputPath = Join-Path $qa 'bom_response_from_json.json.base64'

# 1. 生成设计块（JSON + base64）到 qa_test
& $exe $DemoDir $jsonPath $b64Path
if ($LASTEXITCODE -ne 0) {
    throw "design_block_json 执行失败，退出码 $LASTEXITCODE"
}

# 2. BOM 服务联调：base64 输入
python (Join-Path $root 'tools\bom_client.py') `
    -u $Url `
    -i $b64Path `
    -o $bomPath `
    --raw-output $rawPath `
    -t 300
if ($LASTEXITCODE -ne 0) {
    throw "bom_client.py（base64 输入）失败，退出码 $LASTEXITCODE"
}

# 3. 同样内容改用 JSON 输入，脚本会自动转 base64
python (Join-Path $root 'tools\bom_client.py') `
    -u $Url `
    -i $jsonPath `
    -o $bomJsonInputPath `
    --raw-output $rawJsonInputPath `
    -t 300
if ($LASTEXITCODE -ne 0) {
    throw "bom_client.py（JSON 输入）失败，退出码 $LASTEXITCODE"
}

# 4. 校验产物：两种输入结果一致，响应原文可解码且与落盘 JSON 一致
$fromB64  = Get-Content $bomPath -Raw | ConvertFrom-Json
$fromJson = Get-Content $bomJsonInputPath -Raw | ConvertFrom-Json
$decoded  = [System.Text.Encoding]::UTF8.GetString(
        [System.Convert]::FromBase64String((Get-Content $rawPath -Raw).Trim())) |
        ConvertFrom-Json

$refsB64  = ($fromB64.rows  | ForEach-Object { $_.Reference }) -join ','
$refsJson = ($fromJson.rows | ForEach-Object { $_.Reference }) -join ','
$refsRaw  = ($decoded.rows  | ForEach-Object { $_.Reference }) -join ','

if ($refsB64 -ne $refsJson) {
    throw 'JSON 输入与 base64 输入得到的 BOM 不一致'
}
if ($refsB64 -ne $refsRaw) {
    throw '响应原文解码结果与落盘 BOM JSON 不一致'
}

Write-Host "[+] 校验通过：元件行数 $($fromB64.rows.Count)"

# 5. 错误路径（预期失败）：错误地址应返回非 0 退出码
# 临时放宽 $ErrorActionPreference，避免 PowerShell 把原生命令的 stderr 当成终止错误
$savedErrorAction = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
python (Join-Path $root 'tools\bom_client.py') `
    -u "$Url/nonexistent" `
    -i $b64Path `
    -o (Join-Path $qa 'err.json') `
    --raw-output (Join-Path $qa 'err.raw') `
    -t 30 2>&1 | Out-Null
$errorExitCode = $LASTEXITCODE
$ErrorActionPreference = $savedErrorAction

if ($errorExitCode -eq 0) {
    Write-Warning '错误地址未按预期失败'
}
else {
    Write-Host "[+] 错误路径按预期返回退出码 $errorExitCode"
}

Write-Host "[+] qa_test 完成，产物目录: $qa"
