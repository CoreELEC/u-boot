How to import shell functions    {#shell_functions}
==========================

If you want import shell functions at current shell environment, which could help you to develop on RTOS SDK more convenient.You need to execute: **source scripts/shell_functions.sh** at the rtos sdk's root dir first.Then you can type aml prefix and type tab key to auto-complete the command.

# shell functions list
***
## common command
* **aml_help:** list the shell functions supported.
***
## git command
* **aml_reset:** Reset all repositories to remote branch among rtos sdk.
* **aml_checkout:** Checkout trunk branch if track needed among rtos sdk.
* **aml_git:** Execute git command for all repositories among rtos sdk.
* **aml_pull:** Pull rebase all repositories among rtos sdk.
***
## grep command
* **aml_cgrep:** Greps on all local project C/C++ files.
* **aml_kgrep:** Greps on all local project Kconfig files.
* **aml_cmgrep:** Greps on all local project CMakelists.txt files.
* **aml_shgrep:** Greps on all local project shell files.

# function notes
* **git command:** Used at the rtos sdk is a part of other project which managed by repo,we use the git command only on rtos sdk's repository.
