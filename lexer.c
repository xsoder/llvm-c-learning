#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* NOTE:
 * For now keeping a global var is fine but when I get to parse files it is a
 * problem.
 */

bool
is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool
is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool
is_op(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/';
}

token_s*
peek(tokarr_t *token, size_t *tok_index)
{
    if (*tok_index < token->count) {
        return &token->items[*tok_index];
    }

    return NULL;
}

void
advance(size_t *tok_index)
{
    (*tok_index)++;
}

void
alloc(tokarr_t *t)
{
    if (t->capacity == 0) {
        t->capacity = 256;
        t->items = malloc(sizeof(*t->items) * t->capacity);
    }
    if (t->count >= t->capacity) {
        t->capacity *= 2;
        t->items = realloc(t->items, sizeof(*t->items) * t->capacity);
    }
}

void
parse_input(const char *line, tokarr_t *token)
{
    for (int i = 0; i < strlen(line); i++) {
        if (is_whitespace(line[i])) continue;
        else if (is_op(line[i])) {
            char p = line[i];
            items_append_op(token, p);
            continue;
        }
        else if (is_digit(line[i])) {
            int val = 0;
            while((i < strlen(line) && is_digit(line[i])))
            {
                val = val * 10 + (line[i] - '0');
                i++;
            }
            i--;
            items_append_v(token, val);
            continue;
        }
    }
}

void
items_append_v(tokarr_t *t, int val)
{
    alloc(t);
    t->items[t->count].value  = val;
    t->items[t->count].type = T_VAL;
    t->count++;
}

void
items_append_op(tokarr_t *t, char p)
{
    alloc(t);
    t->items[t->count].op = p;
    switch(p) {
        case '+': t->items[t->count].type = T_PLUS; break;
        case '-': t->items[t->count].type = T_MINUS; break;
        case '*': t->items[t->count].type = T_ASTERIK; break;
        case '/': t->items[t->count].type = T_SLASH; break;
        default: {
            printf("[ERROR]:Unknown token '%c'", p);
            t->items[t->count].type = T_END; break;
        } break;
    }
    t->count++;
}
