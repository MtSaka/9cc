#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2)
        error("%s: 引数の個数が正しくありません", argv[0]);
    Token *token = tokenize(argv[1]);
    Function *code = parse(token);

    codegen(code);
    return 0;
}