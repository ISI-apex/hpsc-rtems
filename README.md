HPSC RTEMS Applications
-----------------------

This project contains RTEMS applications for the HPSC Chiplet.
Specifically, applications are intended for the TRCH (Cortex-M4) and RTPS (Cortex-R52) subsystems.

Common code includes drivers, library routines, and shared platform configurations.
The project layout is:

* `drivers`: produces `libhpsc-drivers.a` containing low-level driver code.
This code must not have dependencies on anything else in this project.
Ideally, this code can be merged into upstream RTEMS.
* `hpsc`: produces `libhpsc.a` containing library routines.
This code may only depend on `libhpsc-drivers`.
* `rtps-r52`: produces `rtps-r52.img` containing the reference/test RTEMS application for the R52s in the RTPS subsystem.


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


Drivers
-------

Current drivers are:

* `hpsc-mbox`: HPSC Mailbox
* `hpsc-wdt`: HPSC Watchdog Timers


Library Code
------------

Current library tools are:

* `command`: A simple framework for command processing.
* `link`: A two-way messaging channel that abstracts the exchange mechanism.
  * `mailbox-link`: An implementation of `link` using HPSC Mailboxes.
  * `shmem-link`: An implementation of `link` using shared memory.
* `server`: An interface to be implemented by applications to handle requests.
* `shmem`: A shared memory messaging interface, compatible with mailbox-style messages.


Developer Notes
---------------

A few lessons learned for getting started with RTEMS:

* Building
  * The RTEMS build system uses only `directory` and `leaf` Makefiles - mixing them is problematic.
  * The template Makefiles provided by RTEMS are a good starting point.
  They don't manage dependencies for incremental builds particularly well (e.g., on libraries, headers, and the Makefile itself) and must be extended.
  * See: https://devel.rtems.org/wiki/Developer/Makefile.
* Interrupt Handling
  * You may NOT use `printf` in an interrupt context - you will not receive an error, things just won't work!
  Use `printk` instead.
  * RTEMS documentation does not currently describe more advanced interrupt handler functionality like `rtems_interrupt_handler_install`/`rtems_interrupt_handler_remove`.
  * See: https://docs.rtems.org/branches/master/c-user/interrupt_manager.html and refer to our existing drivers.

See https://docs.rtems.org for more help.
