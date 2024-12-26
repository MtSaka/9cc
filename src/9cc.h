#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// Token

typedef enum {
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_KEYWORD,
    TK_FOR,
    TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

bool equal(Token *tok, char *op);
Token *consume(Token *tok, char *op);
Token *tokenize(char *p);

// Variable

typedef struct Var Var;

struct Var {
    Var *next;
    char *name;
    int len;
    int offset;
};

// Node

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_ASSIGN,
    ND_LVAR,
    ND_IF,
    ND_FOR,
    ND_RETURN,
    ND_BLOCK,
    ND_FUNCALL,
    ND_EXPR_STMT,
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;

    Node *next;

    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    Node *body;

    int val;

    char *funcname;
    Node *args;

    Var *lvar;
};

// Function

typedef struct Function Function;

struct Function {
    Function *next;
    char *name;
    Var *params;

    Node *node;
    Var *locals;
    int stack_size;
};

Function *parse(Token *tok);
void codegen(Function *prog);
