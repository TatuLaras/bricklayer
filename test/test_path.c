#include "path.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_get_corresponding_works_correctly(void) {
    char *model_path = "/path/to/somewhere/amodelname.obj";
    char *expected_path = "/path/to/somewhere/amodelname.aseprite";
    char *actual_path = path_get_corresponding_texture_file(model_path);
    printf("%s should be %s\n", expected_path, actual_path);
    TEST_ASSERT_FALSE(strcmp(expected_path, actual_path));
    free(actual_path);
}

void test_get_corresponding_only_three_chars(void) {
    char *model_path = "obj";
    char *expected_path = "aseprite";
    char *actual_path = path_get_corresponding_texture_file(model_path);
    printf("%s should be %s\n", expected_path, actual_path);
    TEST_ASSERT_FALSE(strcmp(expected_path, actual_path));
    free(actual_path);
}

void test_get_corresponding_less_than_three_chars(void) {
    char *model_path = "oj";
    char *actual_path = path_get_corresponding_texture_file(model_path);
    TEST_ASSERT_FALSE(actual_path);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_corresponding_works_correctly);
    RUN_TEST(test_get_corresponding_less_than_three_chars);
    RUN_TEST(test_get_corresponding_only_three_chars);

    return UNITY_END();
}
