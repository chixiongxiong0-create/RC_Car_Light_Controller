# STM32F469I-DISCO UI target

This target is being brought up separately from the Wio H725 target. It uses
ST's STM32CubeF4 HAL package and the board's on-board display and touch panel.
The current build is a safe, lamp-off scaffold; it is not a finished UI image.

Configure from the repository root with `BUILD_F469_UI=ON`,
`STM32CUBE_F4_ROOT` pointing to STM32Cube_FW_F4, and the repository's
`cmake/gcc-arm-none-eabi.cmake` toolchain. The exact external wiring is held
until the reported B-01 board marking is verified against a matching schematic.
