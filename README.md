HPSC RTEMS Applications
-----------------------

This project contains RTEMS applications for the HPSC Chiplet.
Specifically, applications are intended for the TRCH (Cortex-M4) and RTPS (Cortex-R52) subsystems.

Common code includes drivers and shared platform configurations.


Building
--------

To compile, first add the RTEMS Source Builder `bin` directory to PATH.
For example, if the `--prefix` used was `/opt/hpsc/RSB-5/`, then:

    export PATH=/opt/hpsc/RSB-5/bin:$PATH

Then set `RTEMS_MAKEFILE_PATH` to be of the form `<prefix>/<target>/<bsp>`.
The `prefix` depends on your install location.
The `target` should be `arm-rtems5`.
The `bsp` should be `gen_r52_qemu` (no option for M4 yet).

For example:

    export RTEMS_MAKEFILE_PATH=/opt/hpsc/RTEMS-5-RTPS-R52/arm-rtems5/gen_r52_qemu

Then finally:

    make
