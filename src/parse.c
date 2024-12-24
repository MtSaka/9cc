#include "9cc.h"

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

static Node *expr(Token **, Token *);
static Node *equality(Token **, Token *);
static Node *relational(Token **, Token *);
static Node *add(Token **, Token *);
static Node *mul(Token **, Token *);
static Node *unary(Token **, Token *);
static Node *primary(Token **, Token *);

static Node *expr(Token **rest, Token *tok) {
    return equality(rest, tok);
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
    if(tok->kind == TK_NUM){
        Node *node = new_node_num(tok->val);
        *rest = tok->next;
        return node;
    }
    error_at(tok->str, "数値でも開きカッコでもないトークンです");
}

Node *parse(Token *tok) {
    Node *node = expr(&tok, tok);
    if (tok->kind != TK_EOF)
        error_at(tok->str, "余分なトークンです");
    return node;
}