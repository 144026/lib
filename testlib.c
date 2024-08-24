#include "common.h"

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

int main(int argc, char *argv[])
{
	test_dynamic_array();
	test_dict();
	return 0;
}
