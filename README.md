# BX Compiler

This is a WIP C++ Compiler implementation for the made up language BX. As of now, it doesn't directly translate a file into assembly or an executable.

# Usage

Assuming you have a file called `file.bx`, to compile one can do:

```
make
./main.exe file.bx
```

This will create a `file.tac.json` file which then has to be translated to ASM via:
```
python utils/tac2x64_jmp.py file.tac.json
```

Finally, to make a binary
```
gcc -o file.exe file.s
./file.exe
```

# Features

There aren't many features available, some are WIP:
- Integers
- Booleans (`true`, `false` in conditions only)
- Variables (only integers)
- Assignments (only integers)
- Printing (only integers)
- If-Else blocks

## TODO:
- While, Break, Continue
- Translate directly to ASM
- Support comments (`// or /* or #`?)