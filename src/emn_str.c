#include "emn_str.h"

int emn_strcmp(const struct emn_str *str1, const struct emn_str *str2)
{
	size_t n1 = str1->len;
	size_t n2 = str2->len;
	int r = memcmp(str1->p, str2->p, (n1 < n2) ? n1 : n2);

	return r ? r : n1 - n2;
}

int emn_strncmp(const struct emn_str *str1, const struct emn_str *str2, size_t n)
{
	size_t n1 = str1->len > n ? n : str1->len;
	size_t n2 = str2->len > n ? n : str2->len;
	int r = memcmp(str1->p, str2->p, (n1 < n2) ? n1 : n2);

	return r ? r : n1 - n2;
}

int emn_strvcmp(const struct emn_str *str1, const char *str2)
{
	size_t n1 = str1->len;
	size_t n2 = strlen(str2);
	int r = memcmp(str1->p, str2, (n1 < n2) ? n1 : n2);

	return r ? r : (n1 - n2);
}

int emn_strvcasecmp(const struct emn_str *str1, const char *str2)
{
	size_t n1 = str1->len;
	size_t n2 = strlen(str2);
	int r = strncasecmp(str1->p, str2, (n1 < n2) ? n1 : n2);

	return r ? r : (n1 - n2);
}

