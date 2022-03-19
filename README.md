# Brainfuck Interpreter

This project is a brainfuck interpreter capable of running brainfuck code. Also implements some optimizations and support for sudo-code generation.

## Building
```bash
cmake -B bin -S .  # First time only
cmake --build bin
```

## Usage
```
./bf.exe <source-file> [--options]
```

### Options & Flags
* `--opt <0|1|2>` - optimization level (0 = None)
* `--generate` - generates sudo code of the bf program
* `--out <filename>` - output of the generate sudo code (does nothing if `--generate` is not enabled)
  