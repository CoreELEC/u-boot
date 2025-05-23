Coding Style {#coding_style}
==========

Amlogic RTOS SDK uses <a href="https://kernel.org/doc/html/latest/process/coding-style.html">Linux kernel coding style</a> to ensure that your development complies with the project’s style and naming conventions.

The Linux kernel GPL-licensed tool <a href="https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/plain/scripts/checkpatch.pl">checkpatch</a> is used to check coding style conformity.

#### To check patches, use following command. ####

	scripts/checkpatch.pl --strict *.patch

#### To check source code, use following command. ####

	scripts/checkpatch.pl --strict -f *.c

#### To reformat your code, use *indent* or *Rindent.sh*. ####

	./scripts/Rindent.sh <path>

Note: ***indent*** can't cover all cases.
