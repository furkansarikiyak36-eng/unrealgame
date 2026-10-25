"""Saves dirty maps and content packages, then closes the editor. py ".../save_and_quit_editor.py" """
import unreal
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.SystemLibrary.quit_editor()
