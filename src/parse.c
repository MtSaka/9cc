#include "9cc.h"

LVar *locals;

static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}
static Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

static Node *program(Token **, Token *);
static Node *stmt(Token **, Token *);
static Node *expr(Token **, Token *);
static Node *assign(Token **, Token *);
static Node *equality(Token **, Token *);
static Node *relational(Token **, Token *);
static Node *add(Token **, Token *);
static Node *mul(Token **, Token *);
static Node *unary(Token **, Token *);
static Node *primary(Token **, Token *);

static Node *program(Token **rest, Token *tok) {
    Node *code = calloc(1, sizeof(Node));
    Node *ret = code;
    while (tok->kind != TK_EOF) {
        code->next = stmt(&tok, tok);
        code = code->next;
    }
    *rest = tok;
    return ret->next;
}

static Node *stmt(Token **rest, Token *tok) {
    Node *node;
    if (equal(tok, "return")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr(&tok, tok->next);
        *rest = consume(tok, ";");
    } else if (equal(tok, "if")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        tok = consume(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = consume(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else")) {
            node->els = stmt(&tok, tok->next);
        }
        *rest = tok;
    } else if (equal(tok, "while")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        tok = consume(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = consume(tok, ")");
        node->then = stmt(rest, tok);
    } else if (equal(tok, "for")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        tok = consume(tok->next, "(");
        if (!equal(tok, ";")) {
            node->init = expr(&tok, tok);
        }
        tok = consume(tok, ";");
        if (!equal(tok, ";")) {
            node->cond = expr(&tok, tok);
        }
        tok = consume(tok, ";");
        if (!equal(tok, ")")) {
            node->inc = expr(&tok, tok);
        }
        tok = consume(tok, ")");
        node->then = stmt(rest, tok);
    } else {
        node = expr(&tok, tok);
        *rest = consume(tok, ";");
    }
    return node;
}

static Node *expr(Token **rest, Token *tok) {
    return assign(rest, tok);
}

static Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    if (equal(tok, "=")) {
        node = new_node(ND_ASSIGN, node, assign(&tok, tok->next));
    }
    *rest = tok;
    return node;
}

static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);
    for (;;) {
        if (equal(tok, "==")) {
            node = new_node(ND_EQ, node, relational(&tok, tok->next));
            continue;
        }
        if (equal(tok, "!=")) {
            node = new_node(ND_NE, node, relational(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);
    for (;;) {
        if (equal(tok, "<")) {
            node = new_node(ND_LT, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, "<=")) {
            node = new_node(ND_LE, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, ">")) {
            node = new_node(ND_LT, add(&tok, tok->next), node);
            continue;
        }
        if (equal(tok, ">=")) {
            node = new_node(ND_LE, add(&tok, tok->next), node);
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);
    for (;;) {
        if (equal(tok, "+")) {
            node = new_node(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }
        if (equal(tok, "-")) {
            node = new_node(ND_SUB, node, mul(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);
    for (;;) {
        if (equal(tok, "*")) {
            node = new_node(ND_MUL, node, unary(&tok, tok->next));
            continue;
        }
        if (equal(tok, "/")) {
            node = new_node(ND_DIV, node, unary(&tok, tok->next));
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+"))
        return unary(rest, tok->next);
    if (equal(tok, "-"))
        return new_node(ND_SUB, new_node_num(0), unary(rest, tok->next));
    return primary(rest, tok);
}

static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = consume(tok, ")");
        return node;
    }
    if (tok->kind == TK_IDENT) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;
        LVar *lvar = find_lvar(tok);
        if (lvar) {
            node->var = lvar;
        } else {
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->offset = locals->offset + 8;
            node->var = lvar;
            locals = lvar;
        }
        *rest = tok->next;
        return node;
    }
    if (tok->kind == TK_NUM) {
        Node *node = new_node_num(tok->val);
        *rest = tok->next;
        return node;
    }
    error_at(tok->str, "数値でも開きカッコでもないトークンです");
}

Function *parse(Token *tok) {
    locals = calloc(1, sizeof(LVar));
    Node *node = program(&tok, tok);
    if (tok->kind != TK_EOF)
        error_at(tok->str, "余分なトークンです");
    Function *prog = calloc(1, sizeof(Function));
    prog->node = node;
    prog->locals = locals;
    return prog;
}