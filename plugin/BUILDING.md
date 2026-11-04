# Building the plugin (RDR-AP.red)

The plugin is a RedHook plugin written against the RedHook SDK by K3rhos. The SDK is not included in this repository.

1. Get the RedHook SDK (linked from the [RedHook page](https://www.nexusmods.com/reddeadredemption/mods/192), or
   [K3rhos/RedHookSDK](https://github.com/K3rhos/RedHookSDK)) and open its solution in Visual Studio 2022 (C++ desktop workload).
2. Replace `Source\Application\Application.cpp` with `plugin/Application.cpp` from this repository.
3. In the project properties (Configuration Properties > General) set **Target Name** to `RDR-AP` for Release | x64
   (the SDK already sets the extension to `.red`).
4. Build **Release | x64**. The output is `Build\x64\Release\RDR-AP.red`.
   Use Release for anything you give to other people: a Debug build needs Visual Studio's debug runtime.
5. Copy `RDR-AP.red`, `gamedata/rdr1_ap_hashmap.txt` and `gamedata/rdr1_ap_scriptmap.txt` into the game folder.

Notes
- MSVC: write `(std::max)(a, b)` / `(std::min)(a, b)`, the Windows headers define `max`/`min` macros.
- The plugin writes `rdr1_ap_test.log` in the game folder; attach it to bug reports.
- Test commands can be typed into `rdr1_ap_in.txt` in the game folder (see the comments at the top of Application.cpp).
