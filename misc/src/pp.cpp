#include "pp.h"

int touppers (char *str)
{
    char c;
    while (*str != '\0') {
        c = toupper(*str);
        *str = c;
        str++;
    }
    return 0;
}

int touppers (char *dest, char *src)
{
    char c;
    while (*src != '\0') {
        c = toupper(*src);
        *dest = c;
        src++;
        dest++;
    }
    return 0;
}

int touppers (char *dest, char *src, int size)
{
    char c;
    int i = 0;
    while ((*src != '\0') && (i++ < size)) {
        c = toupper(*src);
        *dest = c;
        src++;
        dest++;
    }
    return 0;
}

int remove_char (char *str, char del)
{
    int dest = 0, src = 0;
    while (str[src] != 0) {
        if (str[src] != del) {
            str[dest++] = str[src];
        }
        src++;
    }
    return 0;
}

int remove_chars (char *str, char *del)
{
    int dest = 0, src = 0;
    int index = 0;
    bool check = false;
    while (str[src] != 0) {
        index = 0;
        check = true;
        while (del[index] != 0) {
            if (str[src] == del[index++]) {
                check = false;
                break;
            }
        }
        if (check == true) {
            str[dest++] = str[src];
        }
        src++;
    }
    return 0;
}

int parse_hex8 (char *token, uint8_t *byte)
{
    if (token[0] != '0') {
        return -1;
    }
    if ((token[1] != 'x') && (token[1] != 'X')) {
        return -1;
    }
    uint8_t n1, n0;
    char c = toupper(token[2]);
    
    if (c >= 'A') {
        n1 = c - '0' - 7;
    } else {
        n1 = c - '0';
    }
    c = toupper(token[3]);
    
    if (c >= 'A') {
        n0 = c - '0' - 7;
    } else {
        n0 = c - '0';
    }
    *byte = (n1 << 4) + (n0 << 0);
    return 0;
}

int parse_hex16 (char *token, uint16_t *word)
{
    if (*(token++) != '0') {
        return -1;
    }
    if ((*token != 'x') && (*token != 'X')) {
        return -1;
    }
    token++;
    uint8_t nibbles[4];
    char c;
    for (int i = 0; i < 4; i++) {
        c = toupper(token[i]);
        if ((c >= '0') && (c <= 'F')) {
            if (c >= 'A') {
                nibbles[i] = c - '0' - 7;
            } else {
                nibbles[i] = c - '0';
            }
        } else {
            nibbles[i] = 0;
        }
    }
    *word = (nibbles[0] << 12) + (nibbles[1] << 8) + (nibbles[2] << 4) + (nibbles[3] << 0);
    return 0;
}

int parse_hex32 (char *token, uint32_t *dword)
{
    if (*(token++) != '0') {
        return -1;
    }
    if ((*token != 'x') && (*token != 'X')) {
        return -1;
    }
    token++;
    uint8_t nibbles[8];
    char c;
    for (int i = 0; i < 8; i++) {
        c = toupper(token[i]);
        if ((c >= '0') && (c <= 'F')) {
            if (c >= 'A') {
                nibbles[i] = c - '0' - 7;
            } else {
                nibbles[i] = c - '0';
            }
        } else {
            nibbles[i] = 0;
        }
    }
    *dword = 
               (nibbles[0] << 28) + 
               (nibbles[1] << 24) + 
               (nibbles[2] << 20) +
               (nibbles[3] << 16) +
               (nibbles[4] << 12) +
               (nibbles[5] << 8) +
               (nibbles[6] << 4) +
               (nibbles[7] << 0);
    return 0;
}

int parse_uint32 (char *token, uint32_t *dword)
{
    int str_len = strlen(token);
    if (str_len == 0) {
        return -1;
    }
    int weight = 1;
    uint32_t res = 0;
    char c = 0;
    for (int i = 0; i < str_len; i++) {
        c = token[str_len - i - 1];
        if ((c >= '0') && (c <= '9')) {
            res += (c - '0') * weight;
        } else {
            return -1;
        }
        weight *= 10;
    }
    *dword = res;
    return str_len;
}

int parse_int32 (char *token, int32_t *dword)
{
    int str_len = strlen(token);
    if (str_len == 0) {
        return -1;
    }
    int weight = 1;
    int32_t res = 0;
    char c = 0;
    for (int i = 0; i < str_len; i++) {
        c = token[str_len - i - 1];
        if ((c >= '0') && (c <= '9')) {
            res += (c - '0') * weight;
        } else if (c == '-') {
            res = -res;
            break;
        } else {
            return -1;
        }
        weight *= 10;
    }
    *dword = res;
    return str_len;
}

int parse_long (char *token, int64_t *qword)
{
    int str_len = strlen(token);
    if (str_len == 0) {
        return -1;
    }
    int64_t weight = 1;
    int64_t res = 0;
    char c = 0;
    for (int i = 0; i < str_len; i++) {
        c = token[str_len - i - 1];
        if ((c >= '0') && (c <= '9')) {
            res += (c - '0') * weight;
        } else if (c == '-') {
            res = -res;
            break;
        } else {
            return -1;
        }
        weight *= 10;
    }
    *qword = res;
    return str_len;
}

int parse_float (char *token, float *_float)
{
    int64_t minor = 0;
    int64_t major = 0;
    int indexOf = 0;
    char buf[24];
    while ((token[indexOf] != '.') && (indexOf < 24)) {
        buf[indexOf] = token[indexOf];
        indexOf++;
    }
    if ((indexOf == 24) || (indexOf == 0)) {
        return -1;
    }
    buf[indexOf] = '\0';
    if (parse_long(buf, &major) < 0) {
        return -1;
    }
    indexOf = parse_long(token + indexOf + 1, &minor);
    if (indexOf < 0) {
        return -1;
    }
    float _minor = (float)minor;
    while (indexOf--) {
        _minor /= 10.0f;
    }
    if (major < 0) {
        _minor = -_minor;
    }
    *_float = (float)major + _minor;
    return 0;
}

int parse_double (char *token, double *_double)
{
    int64_t minor = 0;
    int64_t major = 0;
    int indexOf = 0;
    char buf[24];
    while ((token[indexOf] != '.') && (indexOf < 24)) {
        buf[indexOf] = token[indexOf];
        indexOf++;
    }
    if ((indexOf == 24) || (indexOf == 0)) {
        return -1;
    }
    buf[indexOf] = '\0';
    if (parse_long(buf, &major) < 0) {
        return -1;
    }
    indexOf = parse_long(token + indexOf + 1, &minor);
    if (indexOf < 0) {
        return -1;
    }
    double _minor = (double)minor;
    while (indexOf--) {
        _minor /= 10.0f;
    }
    if (major < 0) {
        _minor = -_minor;
    }
    *_double = (double)major + _minor;
    return 0;
}

int parse_bool (char *token, bool *_bool)
{
    if (*token == '\0') {
        return -1;
    }
    char buf[5];
    touppers(buf, token, 5);
    if (strncmp("TRUE", buf, 4) == 0) {
        *_bool = true;
        return 0;
    }
    
    if (strncmp("FALSE", buf, 5) == 0) {
        *_bool = false;
        return 0;
    }
    return -1;
}

