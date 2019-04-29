HPSC RTEMS Applications
-----------------------

This project contains RTEMS libraries and applications for the HPSC Chiplet.
Applications are for the TRCH (Cortex-M4) and RTPS (Cortex-R52) subsystems.

Common code includes drivers, library routines, and platform configurations.
The project layout is:

* `lib`: top-level directory for libraries
  * `drivers`: produces `libhpsc-drivers.a` containing low-level driver code.
  This code must not have dependencies on anything else in this project.
  Ideally, this code can be merged into upstream RTEMS.
  * `drivers-selftest`: produces `libhpsc-drivers-selftest.a`.
  These library routines accept parameters for testing drivers and hardware.
  This library may only depend on `libhpsc-drivers`.
  * `hpsc`: produces `libhpsc.a` containing common library routines.
  This library may only depend on `libhpsc-drivers`.
  * `hpsc-test`: produces `libhpsc-test.a` containing tests for `libhpsc`.
  This library may only depend on `libhpsc`.
* `plat`: shared platform configurations to be used by applications.
* `rtps-r52`: produces `rtps-r52.img` containing the reference/test RTEMS
  application for the R52s in the RTPS subsystem.

As a general rule of thumb, libraries should be independent of the RTEMS BSP,
HPSC platform, and application (e.g., rtps-r52) configuration.
This includes the creation of RTEMS `tasks` -- create tasks in the application
and pass task IDs to library routines to start/use when needed.


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

Optionally install the libraries, headers, and applications with:

    make install

For each library `libfoo` (not a real library), the static lib is installed
to `${PROJECT_RELEASE}/lib/libfoo.a` and its headers to
`${PROJECT_RELEASE}/lib/libfoo/`.
`PROJECT_RELEASE` is configured by the Makefile at `RTEMS_MAKEFILE_PATH`, and
may be the same value.
Out-of-tree libraries and applications may include these headers and link
against the libraries produced by this project (described above).

Applications are installed to `${PROJECT_BIN}`.

Drivers
-------

Current drivers are:

* `hpsc-mbox`: HPSC Mailbox
* `hpsc-rti-timer`: HPSC Real-Time Interrupt Timers
* `hpsc-wdt`: HPSC Watchdog Timers


Library Code
------------

Current library tools are:

* `affinity`: A simple wrapper around RTEMS CPU affinity implementation.
* `command`: A framework for command processing.
* `hpsc-msg`: Utility functions for constructing HPSC messages.
* `link`: A two-way messaging channel that abstracts the exchange mechanism.
  * `link-mbox`: An implementation of `link` using HPSC Mailboxes.
  * `link-shmem`: An implementation of `link` using shared memory.
  * `link-store`: A common location to store open links for easy access.
* `shmem`: A shared memory messaging interface, compatible with HPSC messages.
  * `shmem-poll`: Tasks to poll shared memory for HPSC message statuses and
                  issue callbacks which mimic ISRs.


Developer Notes
---------------

A few lessons learned for getting started with RTEMS:

* Building
  * The RTEMS build system uses only `directory` and `leaf` Makefiles - mixing
  them is problematic.
  * The template Makefiles provided by RTEMS are a good starting point.
  They don't manage dependencies for incremental builds particularly well (e.g.,
  on libraries, headers, and the Makefile itself) and must be extended.
  * See: https://devel.rtems.org/wiki/Developer/Makefile.
* Interrupt Handling
  * You may NOT use `printf` in an interrupt context - you will not receive an
  error, things just won't work!
  Use `printk` instead.
  * RTEMS documentation does not currently describe more advanced interrupt
  handler functionality like
  `rtems_interrupt_handler_install`/`rtems_interrupt_handler_remove`.
  * See: https://docs.rtems.org/branches/master/c-user/interrupt_manager.html
  and refer to our existing drivers.
* Testing
  * Keep test routines in separate libraries when possible.
  Applications may opt to not link against test code when unneeded.
  * Tests should be reusable across applications with only minor configuration
  (given as parameters).
  * It may be reasonable to exclude test libraries from the task creation rule.
  This couples them more tightly with the BSP and application configuration, but
  they should still be independent of platform configuration.
  This exclusion is strictly for convenience.

See https://docs.rtems.org for more help.
