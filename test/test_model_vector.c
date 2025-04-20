#include "unity.h"

#include "model_vector.h"
#include <stddef.h>
#include <string.h>

ModelVector vec;

void setUp(void) {
    vec = modelvec_init();
}

void tearDown(void) {
    modelvec_free(&vec);
}

void test_basic_functionality(void) {
    Model model1 = {
        .materialCount = 2,
    };
    Model model2 = {
        .materialCount = 3,
    };
    Model model3 = {
        .materialCount = 4,
    };

    modelvec_append(&vec, model1);
    modelvec_append(&vec, model2);
    modelvec_append(&vec, model3);

    TEST_ASSERT_EQUAL(2, modelvec_get(&vec, 0)->materialCount);
    TEST_ASSERT_EQUAL(3, modelvec_get(&vec, 1)->materialCount);
    TEST_ASSERT_EQUAL(4, modelvec_get(&vec, 2)->materialCount);
}

void test_out_of_bounds_access(void) {
    Model model1 = {
        .materialCount = 2,
    };
    Model model2 = {
        .materialCount = 3,
    };

    modelvec_append(&vec, model1);
    modelvec_append(&vec, model2);

    TEST_ASSERT_EQUAL(2, modelvec_get(&vec, 0)->materialCount);
    TEST_ASSERT_EQUAL(3, modelvec_get(&vec, 1)->materialCount);
    TEST_ASSERT_NULL(modelvec_get(&vec, 2));
    TEST_ASSERT_NULL(modelvec_get(&vec, -1));
}

void test_grow(void) {
    Model model1 = {
        .materialCount = 123,
    };

    for (size_t i = 0; i < 20; i++) {
        modelvec_append(&vec, model1);
    }

    Model *returned1 = modelvec_get(&vec, 0);
    Model *returned2 = modelvec_get(&vec, 15);
    TEST_ASSERT_EQUAL(123, returned1->materialCount);
    TEST_ASSERT_EQUAL(123, returned2->materialCount);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_basic_functionality);
    RUN_TEST(test_out_of_bounds_access);
    RUN_TEST(test_grow);

    return UNITY_END();
}
