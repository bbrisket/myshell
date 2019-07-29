This is an implementation of a simple linux shell. To run, enter the following:
```
$ make
$ ./myshell
```

Supports:

- Batch mode: specifying a file as an additional argument to `./myshell` will
make the program attempt to run each line of the file as a command.
- Navigational built-in commands: `exit`, `cd`, and `pwd` are supported.
- Redirection: `[command] > file.txt` will make the program attempt to write the
output of `[command]` to `file.txt`. This is only supported for non-built-in commands.
- Multiple commands: several jobs can be run on a single line by separating their
commands with a semicolon. For example, `ls; ps; who` will result in each of the 
three jobs being run from left to right, in order.
