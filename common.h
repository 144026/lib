#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof (arr) / sizeof (arr)[0])
#endif

#ifndef offsetof
#define offsetof(a, b) __builtin_offsetof(a, b)
#endif

#ifndef container_of
#define container_of(p, type, field) ((type *) ((char *) (p) - offsetof(type, field)))
#endif

#ifndef min
#define min(a, b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef max
#define max(a, b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#endif


enum loglevel {
	LOGLEVEL_NONE,
	LOGLEVEL_ERROR,
	LOGLEVEL_WARN,
	LOGLEVEL_INFO,
	LOGLEVEL_DEBUG,
};
extern int logging;

#define debug(fmt, args...) do { if (logging >= LOGLEVEL_DEBUG) fprintf(stderr, "[DEBUG][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define info(fmt, args...)  do { if (logging >= LOGLEVEL_INFO)  fprintf(stderr, "[INFO ][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define warn(fmt, args...)  do { if (logging >= LOGLEVEL_WARN)  fprintf(stderr, "[WARN ][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define error(fmt, args...) do { if (logging >= LOGLEVEL_ERROR) fprintf(stderr, "[ERROR][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)


#ifndef __printf
#define __printf(x, y) __attribute__(( format(printf, x, y) ))
#endif

static inline __printf(3, 0) int vsnprintf_s(char *buf, size_t size, const char *format, va_list ap)
{
	int n = vsnprintf(buf, size, format, ap);

	if (n < 0)
		return 0;

	/* n >= size, no more space left */
	return n < size ? n :size;

}

static inline __printf(3, 4) int snprintf_s(char *buf, size_t size, const char *format, ...)
{
	int n;
	va_list ap;

	va_start(ap, format);
	n = vsnprintf_s(buf, size, format, ap);
	va_end(ap);

	return n;
}



struct dynamic_array {
	int size;
	int capacity;
	size_t item_size;
	char *data;
};

#define DYNAMIC_ARRAY_INIT ((struct dynamic_array) {0, 0, 0, NULL})

static inline void *da_item(struct dynamic_array *da, int n)
{

	return (n >= 0 && n < da->size) ? da->data + n * da->item_size : NULL;
}

static inline void *da_expand(struct dynamic_array *da, size_t item_size, int n)
{
	int cap = da->capacity;

	if (da->item_size == 0)
		da->item_size = item_size;
	else
		assert(da->item_size == item_size && "inconsistent array item size");

	while (da->size + n >= cap) {
		if (cap)
			cap = cap << 1;
		else
			cap = da->size + n > 8 ? da->size + n : 8;
	}

	if (cap > da->capacity) {
		da->capacity = cap;
		if (da->data)
			da->data = realloc(da->data, da->item_size * cap);
		else
			da->data = malloc(da->item_size * cap);
	}

	return da->data + da->item_size * da->size;
}

#define da_append(da, item) do {			\
	typeof(item) *__next;				\
	__next = da_expand(da, sizeof(*__next), 1);	\
	*__next = (item);				\
	(da)->size++;					\
} while (0)


static inline void *da_pop(struct dynamic_array *da)
{
	void *item = da_item(da, da->size - 1);

	if (da->size > 0)
		da->size--;

	return item;
}

static inline void da_free(struct dynamic_array *da)
{
	if (da->capacity > 0)
		free(da->data);

	*da = DYNAMIC_ARRAY_INIT;
}



enum dict_val_type {
	DICT_VAL_NONE,
	DICT_VAL_INT,
	DICT_VAL_FLOAT,
	DICT_VAL_LITERAL_STRING,
	DICT_VAL_STRING,
	DICT_VAL_ARRAY,
	DICT_VAL_DICT,
	DICT_VAL_OPAQUE,
};

struct opaque {
	int (*fprint) (void *, FILE *);
	void (*discard) (void *);
	void *data;
};

struct dict {
	struct dynamic_array items;
};

struct dict_val {
	enum dict_val_type type;
	union {
		long int number;
		double Float;
		const char *literal_string;
		char *string;
		struct dynamic_array array;
		struct dict dict;
		struct opaque opaque;
	};
};

struct dict_item {
	const char *key;
	bool const_key;
	struct dict_val val;
};

#define dict_new() ((struct dict) { DYNAMIC_ARRAY_INIT })

#define dict_freed(d) ((d)->items.capacity == 0)

#define define_dict_val_type(field, T, vtype)			\
static inline struct dict_val dict_val_ ## field(T v)		\
{								\
	struct dict_val val = { .type = vtype, .field = v };	\
	return val;						\
}								\
								\
static inline bool dict_val_is_ ## field(struct dict_val *val)	\
{								\
	return val && val->type == vtype;			\
}


#define dict_val_none() ((struct dict_val) { DICT_VAL_NONE })
#define dict_val_is_none(val) ((val) && (val)->type == DICT_VAL_NONE)

define_dict_val_type(number, long int, DICT_VAL_INT);
define_dict_val_type(Float, float, DICT_VAL_FLOAT);
define_dict_val_type(literal_string, const char *, DICT_VAL_LITERAL_STRING);
define_dict_val_type(string, char *, DICT_VAL_STRING);
define_dict_val_type(array, struct dynamic_array, DICT_VAL_ARRAY);
define_dict_val_type(dict, struct dict, DICT_VAL_DICT);
define_dict_val_type(opaque, struct opaque, DICT_VAL_OPAQUE);

/* https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
 * If exp1 is returned, the return type is the same as exp1's type.
 * Similarly, if exp2 is returned, its return type is the same as exp2.
 * ...
 * Furthermore, the unused expression (...) may still generate syntax errors.
 *
 * By factoring macro args out of type-dependent expression, typeof(args) always matches typeof(const_exp)
 */
#define __match_type(T, v, expr) __builtin_choose_expr(__builtin_types_compatible_p(T, typeof(v)), expr,
#define __match_end(T) )

#define dict_val_from(v) (					\
	__match_type(int, v, dict_val_number)			\
	__match_type(long int, v, dict_val_number)			\
	__match_type(double, v, dict_val_Float)			\
	__match_type(struct dynamic_array, v, dict_val_array)	\
	__match_type(struct dict, v, dict_val_dict)		\
	__match_type(struct opaque, v, dict_val_opaque)		\
	__match_type(char[], v, __builtin_choose_expr(__builtin_constant_p(v), dict_val_literal_string, (void) 0)) \
	__match_type(char *, v, dict_val_string)		\
	__match_type(const char *, v, dict_val_literal_string)	\
	(void) 0						\
	__match_end(const char *)				\
	__match_end(char *)					\
	__match_end(char[])					\
	__match_end(struct opaque)				\
	__match_end(struct dict)				\
	__match_end(struct dynamic_array)			\
	__match_end(double)					\
	__match_end(long int)					\
	__match_end(int)					\
) (v)

#define dict_item_foreach(d, item) \
for (item = da_item(&(d)->items, 0); item && item - (struct dict_item *) (d)->items.data < (d)->items.size; item++)

static inline struct dict_item *dict_item(struct dict *d, const char *k)
{
	struct dict_item *item;

	dict_item_foreach(d, item) {
		if (!strcmp(k, item->key))
			return item;
	}

	return NULL;
}

static inline struct dict_val *dict_get(struct dict *d, const char *k)
{
	struct dict_item *item = dict_item(d, k);

	return item ? &item->val : NULL;
}

static inline void dict_val_discard(struct dict_val *val)
{
	switch (val->type) {
	case DICT_VAL_STRING:
		if (val->string) {
			free(val->string);
			val->string = NULL;
		}
		break;
	case DICT_VAL_ARRAY: {
		struct dict_val *arr_val;

		while ((arr_val = da_pop(&val->array)) != NULL)
			dict_val_discard(arr_val);

		da_free(&val->array);
	}
	case DICT_VAL_DICT: {
		struct dict_item *item;

		while ((item = da_pop(&val->dict.items)) != NULL) {
			dict_val_discard(&item->val);
			if (!item->const_key) {
				free((void *) item->key);
				item->key = NULL;
			}
		}
		da_free(&val->dict.items);
	}
	case DICT_VAL_OPAQUE:
		if (val->opaque.discard)
			val->opaque.discard(val->opaque.data);
	default:
		break;
	}
}

static inline void dict_discard(struct dict *d)
{
	struct dict_val val = dict_val_dict(*d);

	return dict_val_discard(&val);
}

static inline int dict_val_fprint_str(const char *s, FILE *fp)
{
	int n = 0;

	/* properly escape " \ */
	while (*s) {
		if (*s == '"' || *s == '\\')
			n += fputc('\\', fp);

		n += fputc(*s++, fp);
	}
	return n;
}

static inline int dict_val_fprint(struct dict_val *val, FILE *fp)
{
	switch (val->type) {
	case DICT_VAL_NONE:
		return fprintf(fp, "null");
	case DICT_VAL_INT:
		return fprintf(fp, "%ld", val->number);
	case DICT_VAL_FLOAT:
		return fprintf(fp, "%f", val->Float);
	case DICT_VAL_LITERAL_STRING:
	case DICT_VAL_STRING:
		if (val->string) {
			int n = 0;
			n += fputc('"', fp);
			n += dict_val_fprint_str(val->string, fp);
			n += fputc('"', fp);
			return n;
		} else {
			return fprintf(fp, "null");
		}

	case DICT_VAL_ARRAY: {
		int i, n = 0;

		n += fprintf(fp, "[");
		for (i = 0; i < val->array.size; i++) {
			struct dict_val *arr_val = da_item(&val->array, i);
			if (i > 0)
				n += fprintf(fp, ", ");

			n += dict_val_fprint(arr_val, fp);
		}
		n += fprintf(fp, "]");
		return n;
	}
	case DICT_VAL_DICT: {
		int i, n = 0;

		n += fprintf(fp, "{");
		for (i = 0; i < val->dict.items.size; i++) {
			struct dict_item *item = da_item(&val->dict.items, i);
			if (i > 0)
				n += fprintf(fp, ", ");

			n += fputc('"', fp);
			n += dict_val_fprint_str(item->key, fp);
			n += fprintf(fp, "\": ");
			n += dict_val_fprint(&item->val, fp);
		}
		n += fprintf(fp, "}");
		return n;
	}
	case DICT_VAL_OPAQUE:
		if (val->opaque.fprint)
			return val->opaque.fprint(val->opaque.data, fp);
		else
			return 0;
	default:
		return 0;
	}
}

static inline int dict_fprint(struct dict *d, FILE *fp)
{
	struct dict_val val = dict_val_dict(*d);
	return dict_val_fprint(&val, fp);
}

static inline bool dict_val_same(struct dict_val *a, struct dict_val *b)
{
	if (a->type != b->type)
		return false;

	switch (a->type) {
	case DICT_VAL_NONE:
		return true;
	case DICT_VAL_INT:
		return a->number == b->number;
	case DICT_VAL_FLOAT:
		return a->Float == b->Float;
	case DICT_VAL_LITERAL_STRING:
		return a->literal_string == b->literal_string;
	case DICT_VAL_STRING:
		return a->string == b->string;
	case DICT_VAL_ARRAY:
	case DICT_VAL_DICT:
	case DICT_VAL_OPAQUE:
		return a == b;
	default:
		return false;
	}
}

#define __dict_set_val(D, K, V) do {					\
	struct dict_item __new_item = {					\
		.key = (K), .val = (V),					\
		.const_key = __builtin_constant_p(K) || __builtin_types_compatible_p(const char *, typeof(K)), \
	};								\
	struct dict_item *__item = dict_item(D, __new_item.key);	\
	if (__item) {							\
		if (!dict_val_same(&__item->val, &__new_item.val)) {	\
			dict_val_discard(&__item->val);			\
			__item->val = __new_item.val;			\
		}							\
	} else {							\
		da_append(&(D)->items, __new_item);			\
	}								\
} while (0)

#define dict_set(D, K, V) __dict_set_val(D, K, dict_val_from(V))

