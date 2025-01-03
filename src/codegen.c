#include "9cc.h"

static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static Function *current_fn;
static int counter = 0;
static int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

static void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->lvar->offset);
    printf("  push rax\n");
}

static int offset_of(Var *var) {
    if (!var)
        return 0;
    if (var->next) {
        var->offset = offset_of(var->next) + 8;
    } else {
        var->offset = 8;
    }
    // fprintf(stderr, "%s: %d\n", var->name, var->offset);
    return var->offset;
}

static void assign_lvar_offsets(Function *prog) {
    int offset = offset_of(prog->locals);
    prog->stack_size = align_to(offset, 16);
}

static void gen_expr(Node *node) {
    switch (node->kind) {
    case ND_FUNCALL:
        int nargs = 0;
        for (Node *now = node->args; now; now = now->next) {
            gen_expr(now);
            nargs++;
        }
        for (int i = nargs - 1; i >= 0; --i) {
            printf("  pop %s\n", argreg[i]);
        }
        printf("  call %s\n", node->funcname);
        printf("  push rax\n");
        return;
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;
    case ND_LVAR:
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen_expr(node->rhs);
        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    case ND_ADDR:
        gen_lval(node->lhs);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    }
    printf("  push rax\n");
}

static void gen_stmt(Node *node) {
    switch (node->kind) {
    case ND_BLOCK:
        for (Node *now = node->body; now; now = now->next) {
            gen_stmt(now);
        }
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        printf("  pop rax\n");
        printf("  jmp .L.return.%s\n", current_fn->name);
        return;
    case ND_IF:
        gen_expr(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        int c_if = counter++;
        printf("  je .L.else.%d\n", c_if);
        gen_stmt(node->then);
        printf("  jmp .L.end.%d\n", c_if);
        printf(".L.else.%d:\n", c_if);
        if (node->els) {
            gen_stmt(node->els);
        }
        printf(".L.end.%d:\n", c_if);
        return;
    case ND_FOR:
        int c_for = counter++;
        if (node->init)
            gen_stmt(node->init);
        printf(".L.begin.%d:\n", c_for);
        if (node->cond) {
            gen_expr(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .L.end.%d\n", c_for);
        }
        gen_stmt(node->then);
        if (node->inc)
            gen_expr(node->inc);
        printf("  jmp .L.begin.%d\n", c_for);
        printf(".L.end.%d:\n", c_for);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    }
}

void codegen(Function *prog) {
    printf(".intel_syntax noprefix\n");
    for (Function *fn = prog; fn; fn = fn->next) {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);
        assign_lvar_offsets(fn);

        current_fn = fn;
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", fn->stack_size);
        int i = 0;

        for (Var *var = fn->params; var; var = var->next) {
            printf("  mov [rbp-%d], %s\n", var->offset, argreg[i++]);
        }

        gen_stmt(fn->node);

        printf(".L.return.%s:\n", fn->name);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}