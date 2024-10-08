#include <time.h>
#include <ctype.h>
#include "common.h"

#define expect_or_fail(x) do { \
	printf("%s:%d expecting: " #x "\n", __FILE__, __LINE__); \
	if (!(x)) exit(1); \
} while (0)

struct test_list_head {
	const char *name;
	struct list_head head;
};

struct test_list_node {
	struct list_head list;
	int n;
};

void test_list(void)
{
	struct  test_list_head h = {
		.name = "test_list1",
	};
	INIT_LIST_HEAD(&h.head);
	expect_or_fail(list_empty(&h.head));

	struct test_list_node a, b, c, *p;
	a.n = 1;
	list_add(&a.list, &h.head);
	b.n = 2;
	list_add_tail(&b.list, &h.head);
	c.n = 3;
	list_add(&c.list, &h.head);

	int i = 0;
	int nums[] = {3, 1, 2};
	list_for_each_entry(p, &h.head, list) {
		printf("p->n %d, nums[%d] %d\n", p->n, i, nums[i]);
		expect_or_fail(p->n == nums[i]);
		i++;
	}
}

void test_dynamic_array(void)
{
	struct dynamic_array da = DYNAMIC_ARRAY_INIT;

	da_append(&da, 1);
	da_append(&da, 2);
	da_append(&da, 3);
	printf("da->size %d, da->capacity %d\n", da.size, da.capacity);
	printf("da->item(2) %d\n", *(int *) da_item(&da, 2));

	printf("da_pop() %d\n", *(int *) da_pop(&da));
	printf("da_pop() %d\n", *(int *) da_pop(&da));
	printf("da_pop() %d\n", *(int *) da_pop(&da));
	printf("da_pop() %p\n", da_pop(&da));

	da_free(&da);
	printf("freed: da->size %d, da->capacity %d\n", da.size, da.capacity);
	printf("freed: da->item(2) %p\n", da_item(&da, 2));
}



int test_dict_opaque_fprint(void *data, FILE *fp)
{
	if (data)
		return fprintf(fp, "\"%s\"", (char *) data);
	else
		return fprintf(fp, "null");
}

void test_dict(void)
{
	struct dict dict = dict_new();
	struct dynamic_array array = DYNAMIC_ARRAY_INIT;
	struct dict dict2 = dict_new();

	__dict_set_val(&dict, "none", dict_val_none());
	dict_set(&dict, "number", 1);
	dict_set(&dict, "long number", 1L);
	dict_set(&dict, "float", 2.001);
	dict_set(&dict, "literal_string", "3");
	const char *const_char = "4";
	dict_set(&dict, "const_char", const_char);
	char *char_string = malloc(10);
	strcpy(char_string, "5");
	dict_set(&dict, "char_string", char_string);

	da_append(&array, dict_val_from(5));
	da_append(&array, dict_val_from("6"));
	da_append(&array, dict_val_from(7.001));
	dict_set(&dict, "array", array);

	dict_set(&dict2, "\\8 : \\\"weird key\"", "\\\\8: \\weird \"value");
	dict_set(&dict, "dict", dict2);

	char *data = malloc(40);
	strcpy(data, "9");

	struct opaque opaque = {
		.fprint = test_dict_opaque_fprint,
		.discard = free,
		.data = data,
	};
	dict_set(&dict, "opaque value", opaque);

	dict_fprint(&dict, stdout);
	fputc('\n', stdout);
	dict_discard(&dict);
}



void tokenize(struct dynamic_array *da, char *s)
{
	char *tok;

	while (1) {
		while (*s && isspace(*s)) s++;
		if (!*s) return;

		tok = s;
		while (*s && !isspace(*s)) s++;
		if (*s) *s++ = '\0';
		da_append(da, strdup(tok));
	}
}

int compare_freq(const void *_a, const void *_b)
{
	const struct dict_item *a = _a;
	const struct dict_item *b = _b;
	return a->val.number - b->val.number;
}

double timespec_delta(struct timespec *a, struct timespec *b)
{
	double ats = a->tv_sec + a->tv_nsec * 1e-9;
	double bts = b->tv_sec + b->tv_nsec * 1e-9;
	return bts - ats;
}

// t8.shakespeare.txt
int test_dict_hash(const char *file)
{
	static char linebuf[1024];
	FILE *fp;
	char *line;
	struct dynamic_array tokens = DYNAMIC_ARRAY_INIT;
	struct dict freqs = dict_new();

	file = file ?: __FILE__;
	fp = fopen(file, "r");
	while ((line = fgets(linebuf, sizeof(linebuf), fp)) != NULL) {
		tokenize(&tokens, line);
	}

	fclose(fp);

	fprintf(stdout, "%s: %d tokens\n", file, tokens.size);

	int i;
	const char **toks = (void *) tokens.data;
	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC, &begin);

	for (i = 0; i < tokens.size; i++) {
		struct dict_val *val = dict_get(&freqs, toks[i]);
		if (val) {
			val->number++;
		} else {
			dict_set(&freqs, toks[i], 1);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stdout, "Counting freq took %lf secs\n", timespec_delta(&begin, &end));

	qsort(freqs.items.data, freqs.items.size, freqs.items.item_size, compare_freq);
	dict_fprint(&freqs, stdout);
	fputc('\n', stdout);

	return 0;
}

int main(int argc, char *argv[])
{
	test_list();
	test_dynamic_array();
	test_dict();
	if (argc > 1 && argv[1])
		test_dict_hash(argv[1]);
	else
		test_dict_hash(NULL);
	return 0;
}
