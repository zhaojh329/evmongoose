#ifndef __EMN_STR_H_
#define __EMN_STR_H_

/* Describes chunk of memory */
struct emn_str {
	const char *p; /* Memory chunk pointer */
	size_t len;    /* Memory chunk length */
};

static inline void emn_str_init(struct emn_str *str, const char *at, size_t len)
{
	str->p = at;
	str-> len = len;
}

#endif
