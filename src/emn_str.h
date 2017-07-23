#ifndef __EMN_STR_H_
#define __EMN_STR_H_

#include <sys/types.h>
#include <string.h>

/* Describes chunk of memory */
struct emn_str {
	const char *p; /* Memory chunk pointer */
	size_t len;    /* Memory chunk length */
};

static inline void emn_str_init(struct emn_str *str, const char *at, size_t len)
{
	str->p = at;
	str->len = len;
}

static inline void emn_str_increase(struct emn_str *str, size_t len)
{
	str->len += len;
}

int emn_strcmp(const struct emn_str *str1, const struct emn_str *str2);
int emn_strncmp(const struct emn_str *str1, const struct emn_str *str2, size_t n);

int emn_strvcmp(const struct emn_str *str1, const char *str2);
int emn_strvcasecmp(const struct emn_str *str1, const char *str2);


#endif
