#include "9cc.h"

static char *user_input;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Token *token;
bool equal(Token *tok, char *op) {
    return memcmp(tok->str, op, tok->len) == 0 && op[tok->len] == '\0';
}
Token *consume(Token *tok, char *op) {
    if (!equal(tok, op))
        error_at(tok->str, "'%s'ではありません", op);
    return tok->next;
}

static bool is_ident1(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_ident2(char c) {
    return is_ident1(c) || ('0' <= c && c <= '9');
}

static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

static bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

Token *tokenize(char *p) {
    user_input = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }
        if (strchr("+-*/()<>;=", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }
        if (is_ident1(*p)) {
            int len = 1;
            char *q = p;
            p++;
            while (is_ident2(*p)) {
                len++;
                p++;
            }
            cur = new_token(TK_IDENT, cur, q, len);
            continue;
        }
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }
        error_at(p, "トークナイズできません");
    }
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}