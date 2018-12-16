#ifndef PARSE_H
#define PARSE_H

#include <string.h>
#include <ctype.h>
#include <stdint.h>

int touppers (char *str);
int touppers (char *dest, char *src);
int touppers (char *dest, char *src, int size);

int remove_char (char *str, char del);
int remove_chars (char *str, char *del);

int parse_hex8 (char *token, uint8_t *byte);
int parse_hex16 (char *token, uint16_t *word);
int parse_hex32 (char *token, uint32_t *dword);

int parse_uint32 (char *token, uint32_t *dword);
int parse_int32 (char *token, int32_t *dword);
int parse_long (char *token, int64_t *qword);
int parse_float (char *token, float *_float);
int parse_double (char *token, double *_double);
int parse_bool (char *token, bool *_bool);

template <typename L>
int parse_struct (char *token, int size, L l)
{
    if (*(token++) != '{') {
        return -1;
    }
    int index = 0;
    int token_num = 0;
    char buf[24];
    while ((*token != '}') && (index < size)) {
        if (*token == ',') {
            buf[index] = '\0';
            if (l(buf, token_num) < 0) {
                return -1;
            }
            index = 0;
            token++;
            token_num++;
        } 
        buf[index++] = *token;
        token++;
    }
    if (index == size) {
        return -1;
    } else {
        buf[index] = '\0';
        l(buf, token_num);
    }
    return token_num;
}

template <typename L>
int parse_int_enum (char *token, int size, char del, L l)
{
    if (*(token) == 0) {
        return -1;
    }
    int index = 0;
    int token_num = 0;
    char buf[24];
    while ((*token != 0) && (index < size)) {
        if (*token == del) {
            buf[index] = '\0';
            if (l(buf, token_num) < 0) {
                return -1;
            }
            index = 0;
            token++;
            token_num++;
        } 
        buf[index++] = *token;
        token++;
    }
    if (index == size) {
        return -1;
    } else {
        buf[index] = '\0';
        if (l(buf, token_num) < 0) { /*todo*/
            return -1;
        }
    }
    return token_num;
}

#endif  /*PARSE_H*/
