# Symbolic Differentiator
Command line tool for computing differentials of single variable polynomial expression inputs.

## Compiling
Clone the repository, navigate to the project directory and run the following command on the terminal:
```
$ g++ main.cpp -o symb-diff
```
Run the executable to begin computing differentials:
```
$ ./symb-diff
$ SymbDiff ('q'/'exit'/'quit'/CTRL-C to exit)
$ >
```
## Includes
- Implicit multiplication support. For example, `4x` is interpreted as `4*x` and vice-versa.
- Implicit parse precedence evaluation (thanks to recursive descent!). For now, explicit parse precedence specification via parentheses will result in a parse error.
  I have nothing against parentheses, I just felt they were unnecessary for this base implementation.

Regards 🫰
