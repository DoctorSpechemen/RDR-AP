# RDR-AP: where we are

## Beta 3.0 - plugin v7.0, APWorld 0.4.0, PopTracker 0.5.0 - 2026-11-05
Source of truth for the plugin: `RedHookSDK\Source\Application\Application.cpp` (same as `redhook_test/Application.cpp`).
Build: Visual Studio, Release | x64, `Build\x64\Release\RDR-AP.red`. Game must be closed before the .red is swapped.

### Confirmed working in play (2026-11-04 / 05 playthrough)
Story missions through Fort Mercer and into Mexico, strangers, gang kills (incl. Reyes' Rebels), hideouts, hat tricks,
duels, poker, Five Finger Fillet, Horseshoes, Liar's Dice, Arm Wrestling wins, bounties, shops incl. tailors and scraps,
outfit take-back, cash, consumables, all 11 traps incl. the duel trap, F9 tally colours, Landon lock + reminder,
PopTracker Mexico / West Elizabeth gating.

### Needs a test with the v7.0 build
- Fame / honor rank checks (Gunslinger 8000, Legend 10000, Hero 3500, outlaw -1500/-2500/-3500 are guesses).
- Law uniforms in the pause menu (UniformMenuTick re-unlocks them after pause.c locks them; log shows "uniforms: ..."). F10 stays as a fallback test key until this is confirmed.
- Arm wrestling loss no longer counted as a win.
- West Elizabeth, Remember My Family goal marker. (DeathLink confirmed with Gungeon; TrapLink removed in 3.0.)
- Jenny's Faith: sent after part 1. Does she have a second part?

## Release 1.0 plan
- Hide the test-only commands (#define RDR_AP_DEV), Application.cpp split into files (items, traps, shops, tracker, checks).
- Confirm or correct the guessed rank cut-offs.

## Working notes
- Device link quirk: always write commands and re-sent files under a NEW staged file name (a reused name can send an old copy).
- Visual Studio can overwrite the source with its buffer: re-stage and compare after every commit, then "Reload" in VS.
- Ids are append-only (removed items/locations keep their ids): Herbert Moon Trap, Ricketts trap, clusters, milestones (ranks have new ids),
  random encounters, challenges.
