# Oar
- Shell and scripting language for learning/getting comfortable in C

## TODO:
- [x] PATH resolver for shell
- [ ] Stringigy everything if possible in the language, maybe based on a macro OAR_STRINGIFY_ARGS or something.
    - [x] add more cases in lexer that build strings such as when writing paths: ./, /, etc.

## Syntax idea
```oar
$ program
$ if condition == true { program }; echo "hi" // echo i
$ fn do_something() -> void {
    program
}
$ (program arg1 arg 2 arg) // will run what is inside the parenthesis, does the same as `program arg1 arg2 arg3`
```
* All programs are functions (with variadic unlimited aruments signature: `fn program(...)`)
