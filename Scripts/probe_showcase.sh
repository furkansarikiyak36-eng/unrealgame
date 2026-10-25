#!/bin/sh
# Runs LogShowcase stations in a play session of MechanicsTest (temporary showcase actor, nothing saved), prints the OK/XX
# tally per station and writes Docs/DILIM_SONUC.md (acceptance ID -> PASS / FAIL / BLOCKED, with date and commit).
#   probe_showcase.sh            all 14 stations
#   probe_showcase.sh 3 5 14     only those stations (1-based)
# Editor open; CMDREF = command box ref (find_cmdref.py). Report: Saved/house_run.txt
cd "$(dirname "$0")/.."
export CMDREF="${CMDREF:-tb2}"
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/editor_tools/unmin.ps1 >/dev/null 2>&1
rm -f Saved/house_run.txt Saved/tmp_show_gone.txt Saved/probe_stations.txt
[ $# -gt 0 ] && echo "$*" | tr ' ' ',' > Saved/probe_stations.txt
sh Scripts/editor_tools/pyrun.sh probe_add.py "" none.json 5 >/dev/null
sh "$TEMP/pie.sh" StartPIE '{"options":{}}' >/dev/null; python -c "import time;time.sleep(25)"
sh Scripts/editor_tools/pyrun.sh probe_build.py "" house_run.txt 150 >/dev/null
sh "$TEMP/pie.sh" StopPIE '{}' >/dev/null; python -c "import time;time.sleep(6)"
sh Scripts/editor_tools/pyrun.sh probe_del.py "" tmp_show_gone.txt 20 >/dev/null
python Scripts/record_dilim.py
