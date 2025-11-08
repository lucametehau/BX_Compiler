# BX Compiler

This is a WIP C++ Compiler implementation for the made up language BX. As of now, it directly translates a file into assembly.

# Usage

Assuming you have a file called `file.bx`, to compile one can do:

```
make
./main.exe file.bx
```

Finally, to make a binary
```
gcc -o file.exe file.s utils/bx_runtime.c
./file.exe
```

Since `bx_runtime.c` contains the print functions we are gonna use.

# Features

There aren't many features available, some are WIP:
- Integers
- Booleans
- Variables
- Assignments
- Printing
- Functions/Procedures
- If-Else blocks
- While, Break, Continue
- Translates directly to asm
- Supports comments (`// or /* or #`?)