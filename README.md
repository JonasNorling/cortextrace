# Cortex Trace Utilities

This repository contains software for utilizing the SWO trace
functionality in ARM Cortex-M CPU cores used in popular
microcontrollers. This debug feature allows things such as listening
to a log or event stream from instrumented code, and watching
variables for read and write access without interrupting the program
execution.

A compatible SWD/SWO debug probe is required, with accompanying
gdbserver software. OpenOCD fills this part nicely, and since release
0.9 it has working SWD and SWO support.

## Source tree layout

### libcortextrace/

Libcortextrace is a library for utilizing the SWO trace functionality
in ARM Cortex-M CPU cores. This library can communicate with a debug
probe through OpenOCD to enable, listen to and parse trace messages.

The library is copyright Jonas Norling <jonas.norling@gmail.com> and
distributed under the GNU Lesser General Public Licence Version 3
(LGPL3).

### libcortextrace/tools/

Example programs using the library.

### libcortextrace/3pp/libmi/

This is a third-party C library for communicating with GDB using the
GDB/MI protocol. It is distributed under the Eclipse Public License.

Snapshot taken from the source repository at:
https://github.com/agontarek/libmi.git
