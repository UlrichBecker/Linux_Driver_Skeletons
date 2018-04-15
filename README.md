# Linux Driver Skeletons
Skeletons (Templates) to programming Linux device drivers.

Suitable for cross-compiling for embedded devices and/or native-compiling for the host-pc,
depending on the environment variables CROSS_COMPILE, ARCH and KERNEL_SRC_DIR.
If these environment variables are not set so the modules becomes compiles for the host-pc.

Directory ./char_driver contains a C- source file and the corresponding makefile as template for
programming Linux- character- device- drivers for single or multiple instances (depending on macro MAX_INSTANCES).

Directory ./select_poll contains a example how a kernel-space-driver cooperates by the user-space function "select()" respectively "poll()".


