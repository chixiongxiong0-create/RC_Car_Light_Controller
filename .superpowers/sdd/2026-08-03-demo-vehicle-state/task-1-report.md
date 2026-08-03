# Task 1 report: demo vehicle state and source selector

## Changes

- Added `demo_vehicle_state_sample()`, a deterministic, bounded sample generator.
  It initializes every `VehicleState` field, keeps the link in `LINK_STARTING`,
  forces `aux_page` to zero and `armed` false, and uses a nominal per-cell
  voltage safely above the low-battery threshold.
- Added `VehicleStateSource`, which starts in demo mode, permanently latches
  after `vehicle_state_source_note_real()`, copies discrete real values
  immediately, and blends only the specified numeric values over 300 ms.
  Heading interpolation follows the shortest wrapped arc.
- Added focused host tests and CTest targets for both components.

## RED evidence

1. Demo generator:

   ```powershell
   cmake -S tests -B build-host-demo
   cmake --build build-host-demo
   ```

   The build failed as intended with `fatal error C1083: cannot open include
   file: 'demo_vehicle_state.h'`.

2. Selector:

   ```powershell
   cmake -S tests -B build-host-demo
   cmake --build build-host-demo
   ```

   The build failed as intended with `fatal error C1083: cannot open include
   file: 'vehicle_state_source.h'`.

## GREEN evidence

```powershell
cmake -S tests -B build-host-demo
cmake --build build-host-demo
ctest --test-dir build-host-demo -C Debug --output-on-failure
```

The final run exited successfully: 6/6 CTest entries passed, including the
existing unit and manifest tests plus both new component test executables.

## Self-review

- Demo output is deterministic for repeated timestamps; all fields are zeroed
  before explicit values are assigned.
- Controls, attitude, heading, and battery meet their defined bounds. The
  timestamp arithmetic uses unsigned values where elapsed-time wrapping is
  relevant.
- Real ownership never reverts to demo, including when the real link is lost.
- The selector copies `aux_page`, RSSI, mode flags, satellites, armed/link,
  and timestamps directly from the real sample while blending only the six
  allowed numeric fields.
- `git diff --check` found no whitespace errors.

## Concerns

None within Task 1 scope. The local CMake generator is Visual Studio
(multi-config), so CTest needs `-C Debug`; the task brief's CTest command
without that option reports tests as unavailable in this environment.
