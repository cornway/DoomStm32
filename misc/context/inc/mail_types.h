#ifndef MAIL_TYPES_H
#define MAIL_TYPES_H

typedef V_PREPACK struct {
    char *message;
    void *object;
    arch_word_t type;
} MAIL_HANDLE;

#endif /*MAIL_TYPES_H*/
