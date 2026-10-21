Add-Type -Name W3 -Namespace U3 -MemberDefinition '[DllImport("user32.dll")] public static extern bool IsIconic(System.IntPtr h); [DllImport("user32.dll")] public static extern bool ShowWindowAsync(System.IntPtr h, int n);'
$h = (Get-Process UnrealEditor).MainWindowHandle
if ([U3.W3]::IsIconic($h)) { [U3.W3]::ShowWindowAsync($h, 4) | Out-Null; Start-Sleep -Seconds 4; "restored" } else { "visible" }
