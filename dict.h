#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "dynamic_array.h"

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

struct dict_lookup_bucket {
	struct dynamic_array pitems;
};

struct dict {
	struct dynamic_array items;
	struct dynamic_array buckets;
	int stress;
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

#define dict_new() ((struct dict) { DYNAMIC_ARRAY_INIT, DYNAMIC_ARRAY_INIT, 0 })

#define dict_freed(d) ((d)->items.capacity == 0)

static inline int dict_hashkey(struct dict *d, const char *k)
{
	unsigned long hash = 5381;
	const unsigned char *str = (void *) k;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash % d->buckets.size;
}

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
#define __matchT_type(T, v, expr, else_expr) \
	__builtin_choose_expr(__builtin_types_compatible_p(T, typeof(v)), expr, else_expr)

#define __matchT_1(v, T, expr) __matchT_type(T, v, expr, (void)0)
#define __matchT_2(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_1(v, ##__VA_ARGS__))
#define __matchT_3(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_2(v, ##__VA_ARGS__))
#define __matchT_4(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_3(v, ##__VA_ARGS__))
#define __matchT_5(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_4(v, ##__VA_ARGS__))
#define __matchT_6(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_5(v, ##__VA_ARGS__))
#define __matchT_7(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_6(v, ##__VA_ARGS__))
#define __matchT_8(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_7(v, ##__VA_ARGS__))
#define __matchT_9(v, T, expr, ...) __matchT_type(T, v, expr, __matchT_8(v, ##__VA_ARGS__))

#define __matchT_arg_n(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
	_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#define __matchT_nargs_rseq() 10,10,9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0
#define __matchT_nargs_(...) __matchT_arg_n(__VA_ARGS__)
#define __matchT_nargs(...) __matchT_nargs_(__VA_ARGS__, __matchT_nargs_rseq())
#define __matchT(v, ...) CONCAT(__matchT_, __matchT_nargs(v, __VA_ARGS__)) (v, __VA_ARGS__)

#define dict_val_from(v) (__matchT(v, \
	int, dict_val_number, \
	long int, dict_val_number, \
	double, dict_val_Float, \
	struct dynamic_array, dict_val_array, \
	struct dict, dict_val_dict, \
	struct opaque, dict_val_opaque, \
	char *, dict_val_string, \
	const char *, dict_val_literal_string, \
	char[], __builtin_choose_expr(__builtin_constant_p(v), dict_val_literal_string, (void)0) \
)) (v)

#define dict_item_foreach(d, item) \
for (item = da_item(&(d)->items, 0); item && item - (struct dict_item *) (d)->items.data < (d)->items.size; item++)

static inline struct dict_item *dict_item(struct dict *d, const char *k)
{
	struct dict_item *item;

	if (__builtin_expect((!d || !k), 0))
		return NULL;

	if (d->buckets.size) {
		int i, idx = dict_hashkey(d, k);
		struct dict_lookup_bucket *bucket = da_item(&d->buckets, idx);

		if (!bucket)
			return NULL;

		for (i = 0; i < bucket->pitems.size; i++) {
			item = *(struct dict_item **) da_item(&bucket->pitems, i);
			if (!strcmp(k, item->key))
				return item;
		}
		return NULL;
	}

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
		struct dict_lookup_bucket *bucket;

		while ((item = da_pop(&val->dict.items)) != NULL) {
			dict_val_discard(&item->val);
			if (!item->const_key) {
				free((void *) item->key);
				item->key = NULL;
			}
		}
		da_free(&val->dict.items);

		while ((bucket = da_pop(&val->dict.buckets)) != NULL)
			da_free(&bucket->pitems);

		da_free(&val->dict.buckets);
		val->dict.stress = 0;
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
		return memcmp(&a->array, &b->array, sizeof(struct dynamic_array)) == 0;
	case DICT_VAL_DICT:
		return memcmp(&a->dict, &b->dict, sizeof(struct dict)) == 0;
	case DICT_VAL_OPAQUE:
		return memcmp(&a->opaque, &b->opaque, sizeof(struct opaque)) == 0;
	default:
		return false;
	}
}

static inline void dict_hash_item(struct dict *d, struct dict_item *item)
{
	int h = dict_hashkey(d, item->key);
	struct dict_lookup_bucket *b = da_item(&d->buckets, h);

	if (b->pitems.size == 0)
		d->stress++;

	da_append(&b->pitems, item);
}

#define DICT_HASHING_THRESH 32

static inline void dict_rehash_all(struct dict *d)
{
	struct dict_item *item;
	int ocap = d->buckets.capacity;

	/* stress factor 0.75 */
	if (4 * d->stress < 3 * d->buckets.size)
		return;

	while (d->buckets.capacity <= max(ocap, DICT_HASHING_THRESH))
		da_append(&d->buckets, DYNAMIC_ARRAY_INIT);

	// fprintf(stderr, "--> dict[%p] rehash: items size %d capacity %d, stress %d, bucket capacity %d to %d\n",
	// 	d, d->items.size, d->items.capacity, d->stress, ocap, d->buckets.capacity);
	memset(d->buckets.data, 0, d->buckets.item_size * d->buckets.capacity);
	d->buckets.size = d->buckets.capacity;
	d->stress = 0;

	dict_item_foreach(d, item) {
		dict_hash_item(d, item);
	}
}

static inline void dict_add_item(struct dict *d, struct dict_item *item)
{
	da_append(&d->items, *item);

	if (d->items.size < DICT_HASHING_THRESH)
		return;

	dict_rehash_all(d);

	item = da_item(&d->items, d->items.size - 1);
	dict_hash_item(d, item);
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
		dict_add_item(D, &__new_item);				\
	}								\
} while (0)

#define dict_set(D, K, V) __dict_set_val(D, K, dict_val_from(V))
