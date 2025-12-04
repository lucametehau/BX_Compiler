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
- Lambdas (They automatically capture everthing)
- Passing functions as variables
- Assignments
- Printing
- Functions/Procedures
- If-Else blocks
- While, Break, Continue
- Translates directly to asm
- Supports comments (`// or /* or #`?)

# Detailed description

## Code organization

There are 5 big parts of the project, each found in its own folder:

- The Lexer is found in `lexer/`, core implementation is in `lexer/lexer.cpp`. Nothing much to describe, this part is pretty straight forward.
- The Parser is found in `parser/` and `ast/`. The parser itself is just an object to iterate over the lexing tokens, while the actual parsing is happening when creating the AST.
- The AST creation is in `ast/`, which contains multiple files specific to each type of grammar defintion. For example `ast/statement.cpp` contains the parsing of many _Statements_. `ast/ast.h` contains the AST node definitions.
- The Type checking is in `typing/type.cpp`, it's similar to munching, but simpler. It contains the definition of `::type_check()` for each AST node.
- The Munching of the AST is in `ast/ast.cpp`, the hardest part of the project. It contains the definition of `::munch()` for each AST node.
- The Assembling is in `asm/`, `assemble_proc` sets some procedure specific stuff, before `assemble_instr` actually assembles every instruction.
- The Optimizations are in `cfg/cfg.cpp` and `cfg/cfg.h`, which includes the CFG definition `make_cfg`, block building `make_blocks` and all the optimizations specified in class.

## Approach to the compiler

I had a nice time building and architecting the compiler, as everything was written from scratch. For the frontend, I monstly followed the provided instructions, although it was a bit tricky to do the Parsing without any prior library, I had to really design the AST to make my life as easy as possible in the future. In the end, I settled for the current scheme, where there are some base nodes built on top of AST: _Expression_, _Statement_ and _Declaration_ on top of which I added the grammar definitions.
Each AST node posseses a `print`, `munch` and `type_check` function, which makes everything very modular.

Munching was slightly annoying in the beginning, because when I was introduced to boolean expressions, I had to have a separate munch function for booleans, as that function should take 2 branches as parameters. I settled on only defining it for _Expression_ derived classes.

Assembling was mostly fine until higher order functions had to be implemented, since that meant I had to detect if each temporary used in a function was actually from this function and if not, go on the static link chain until I find the parent function of the temporary. Overall, it is pretty straight forward casework.

Optimizating and building the CFG was really interesting, kinda sad that we didn't go into more optimization techniques like constant propagation or folding. It was a bit of a mess to write nice, clean code to work with a Block Graph, but in the end it's not that spaghetti. SSA generation was really annoying because I had to rewrite some implementation of the optimizations, which weren't using the SSA representation.

Finally, I really enjoyed building this compiler, I will very likely continue working on it.