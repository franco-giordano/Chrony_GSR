from os.path import join, isfile

Import("env")

FRAMEWORK_DIR = join(".")
patchflag_path = join(FRAMEWORK_DIR, ".pio", "libdeps", "watchy", "GxEPD2", "src", "epd", ".patching-done")

PATCH_TUPLES = [("GxEPD2_154_D67.h", "1-patch-h.patch"), ("GxEPD2_154_D67.cpp", "2-patch-cpp.patch")]

# patch file only if we didn't do it before
if not isfile(patchflag_path):
    for original_name, patch_name in PATCH_TUPLES:
        original_file = join(FRAMEWORK_DIR, ".pio", "libdeps", "watchy", "GxEPD2", "src", "epd", original_name)
        patched_file = join("patches", patch_name)

        assert isfile(original_file) and isfile(patched_file)

        env.Execute("patch %s %s" % (original_file, patched_file))
        # env.Execute("touch " + patchflag_path)

    def _touch(path):
        with open(path, "w") as fp:
            fp.write("")

    env.Execute(lambda *args, **kwargs: _touch(patchflag_path))

print("-- Patching completed")