# Linux Driver Skeletons
Skeletons (Templates) to programming Linux device drivers.

Suitable for cross-compiling for embedded devices and/or native-compiling for the host-pc,
depending on the environment variables CROSS_COMPILE, ARCH and KERNEL_SRC_DIR.
If these environment variables are not set so the modules becomes compiles for the host-pc.

Directory ./char_driver contains a C- source file and the corresponding makefile as template for
programming Linux- character- device- drivers for single or multiple instances (depending on macro MAX_INSTANCES).

Directory ./select_poll contains a example how a kernel-space-driver cooperates by the user-space function "select()" respectively "poll()".


**A few words about my coding style.**

Yes, I know my code is not [Linux-style](https://www.kernel.org/doc/html/latest/process/coding-style.html).
For example, I open curly braces in a new line, and don't use tabs except in makefiles.
So the code can never officially become part of the Linux kernel, which I never planned to do.
However, since I am the only author of the code so far and have maintained it alone so far, I keep the code in a form that is most pleasing to my eyes.
I ask you to respect that as well, I respecting the coding style of others as well.

There is a saying in Germany that means: "Einem geschenkten Gaul schaut man nicht ins Maul." ("You don't look a gift horse in the mouth.")

;-)
