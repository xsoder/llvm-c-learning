/* date = January 23rd 2026 4:29 am */

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>

typedef enum {
    T_VAL,
    T_PLUS,
    T_MINUS,
    T_ASTERIK,
    T_SLASH,
    T_END
} token_t;

typedef struct {
    int  value;
    int  type;
    char op;
} token_s;

typedef struct {
    size_t count;
    size_t capacity;
    token_s *items;
} tokarr_t;

bool is_digit(char c);
bool is_whitespace(char c);
bool is_op(char c);

token_s* peek(tokarr_t *token, size_t *tok_index);
void advance(size_t *tok_index);

void alloc(tokarr_t *t);
void items_append_op(tokarr_t *t, char p);
void items_append_v(tokarr_t *t, int val);

void parse_input(const char *line, tokarr_t *token);

bool is_digit(char c);
bool is_whitespace(char c);
bool is_op(char c);

#endif //LEXER_H
