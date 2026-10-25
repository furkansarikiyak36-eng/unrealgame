$p = Get-Process UnrealEditor; $o = Get-CimInstance Win32_OperatingSystem
$c1 = $p.TotalProcessorTime.TotalMilliseconds; Start-Sleep -Seconds 5; $p.Refresh(); $c2 = $p.TotalProcessorTime.TotalMilliseconds
$cpu = ($c2 - $c1) / 5000 / [Environment]::ProcessorCount * 100
$g = (& nvidia-smi --query-gpu=memory.used,utilization.gpu --format=csv,noheader,nounits).Split(',')
'EDITOR_PRIVATE_GB {0:N2} EDITOR_RAM_GB {1:N2} SYS_FREE_GB {2:N2} EDITOR_CPU_PCT {3:N1} GPU_VRAM_TOTAL_MB {4} GPU_UTIL_PCT {5}' -f ($p.PrivateMemorySize64/1GB), ($p.WorkingSet64/1GB), ($o.FreePhysicalMemory/1MB), $cpu, $g[0].Trim(), $g[1].Trim()
