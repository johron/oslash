#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists("build")) {
        return 1;
    }

    Nob_Cmd cmd = {0};

    nob_cmd_append(
        &cmd,
        "cc",
        "-Wall",
        "-Wextra",
        "-std=c11",
        "src/main.c",
        "src/lang/lexer.c",
        "src/lang/parser.c",
        "src/lang/evaluator.c",
        "-o",
        "build/ø"
    );

    if (!nob_cmd_run_sync(cmd)) {
        return 1;
    }

    return 0;
}
