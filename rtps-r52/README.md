HPSC RTEMS RTPS-R52 Reference Application
-----------------------------------------

This directory contains the reference RTEMS application for the R-52 cores in
the RTPS subsystem.

The reference application expects to own both R52 CPUs, in lockstep or SMP.
Resources are not yet allocated at the platform level to support split mode,
which requires coordination among all subsystems.
Attempting to run two instances in split mode will result in resource clashes.
