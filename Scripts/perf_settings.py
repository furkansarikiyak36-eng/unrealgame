"""Editor throttling for the PerfTest measurements: py ".../perf_settings.py" off|restore.
'off' remembers the current 'Use Less CPU when in Background' value in Saved/perf_settings.json and turns it off
(the editor drops to ~3 fps in the background otherwise); 'restore' puts the remembered value back."""
import json, os, sys
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'perf_settings.json')
s = unreal.get_default_object(unreal.EditorPerformanceSettings)
mode = sys.argv[1] if len(sys.argv) > 1 else 'off'
if mode == 'off':
    before = s.get_editor_property('throttle_cpu_when_not_foreground')
    if not os.path.exists(OUT):
        json.dump({'throttle_cpu_when_not_foreground': before}, open(OUT, 'w'))
    s.set_editor_property('throttle_cpu_when_not_foreground', False)
else:
    before = json.load(open(OUT))['throttle_cpu_when_not_foreground']
    s.set_editor_property('throttle_cpu_when_not_foreground', before)
    os.remove(OUT)
unreal.log('[PerfSettings] {} -> {}'.format(mode, s.get_editor_property('throttle_cpu_when_not_foreground')))
