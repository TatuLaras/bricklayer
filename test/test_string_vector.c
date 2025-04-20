#include "string_vector.h"
#include "unity.h"
#include <stddef.h>
#include <string.h>

StringVector vec;

void setUp(void) {
    vec = stringvec_init();
}

void tearDown(void) {
    stringvec_free(&vec);
}

void test_basic_functionality(void) {
    char *string1 = "Hello this is string";
    char *string2 = "Another one";
    char *string3 = "Third one";

    stringvec_append(&vec, string1, strlen(string1) + 1);
    stringvec_append(&vec, string2, strlen(string2) + 1);
    stringvec_append(&vec, string3, strlen(string3) + 1);

    char *returned2 = stringvec_get(&vec, 1);
    char *returned1 = stringvec_get(&vec, 0);
    char *returned3 = stringvec_get(&vec, 2);
    TEST_ASSERT_EQUAL_STRING("Hello this is string", returned1);
    TEST_ASSERT_EQUAL_STRING("Another one", returned2);
    TEST_ASSERT_EQUAL_STRING("Third one", returned3);
}

void test_forgotten_null_terminator(void) {
    char *string1 = "Hello this is string";
    char *string2 = "Another one";
    char *string3 = "Third one";

    stringvec_append(&vec, string1, strlen(string1));
    stringvec_append(&vec, string2, strlen(string2));
    stringvec_append(&vec, string3, strlen(string3));

    char *returned2 = stringvec_get(&vec, 1);
    char *returned1 = stringvec_get(&vec, 0);
    char *returned3 = stringvec_get(&vec, 2);
    TEST_ASSERT_EQUAL_STRING("Hello this is string", returned1);
    TEST_ASSERT_EQUAL_STRING("Another one", returned2);
    TEST_ASSERT_EQUAL_STRING("Third one", returned3);
}

void test_out_of_bounds_access(void) {
    char *string1 = "Hello this is string";
    char *string2 = "Another one";
    char *string3 = "Third one";
    stringvec_append(&vec, string1, strlen(string1));
    stringvec_append(&vec, string2, strlen(string2));
    stringvec_append(&vec, string3, strlen(string3));

    char *returned1 = stringvec_get(&vec, 0);
    char *returned2 = stringvec_get(&vec, 3);
    char *returned3 = stringvec_get(&vec, -1);
    TEST_ASSERT_EQUAL_STRING("Hello this is string", returned1);
    TEST_ASSERT_NULL(returned2);
    TEST_ASSERT_NULL(returned3);
}

void test_grow_indices(void) {
    char *string1 = "Hello this is string";
    for (size_t i = 0; i < 20; i++) {
        stringvec_append(&vec, string1, strlen(string1));
    }

    char *returned1 = stringvec_get(&vec, 0);
    char *returned2 = stringvec_get(&vec, 15);
    TEST_ASSERT_EQUAL_STRING("Hello this is string", returned1);
    TEST_ASSERT_EQUAL_STRING("Hello this is string", returned2);
}

void test_grow_data(void) {
    char *shakespeare = "1609 \
        1 From fairest creatures we desire increase, \
         That thereby beauty's rose might never die, But as the riper should by \
             time decease, \
         His tender heir might bear his memory \
        : But thou contracted to thine own bright eyes, \
         Feed 'st thy light' s flame with self - substantial fuel, \
         Making a famine where abundance lies, \
         Thy self thy foe, \
         to thy sweet self too cruel \
        : Thou that art now the world's fresh ornament, And only herald to the \
              gaudy spring, \
         Within thine own bud buriest thy content, \
         And tender churl mak'st waste in niggarding: Pity the world, \
         or else this glutton be, \
         To eat the world's due, by the grave and thee. \
 \
          2 When forty winters shall besiege thy brow, \
         And dig deep trenches in thy \
              beauty's field, Thy youth's proud livery so gazed on now, Will be \
                  a tattered weed of small worth held : Then being asked, \
         where all thy beauty lies, \
         Where all the treasure of thy lusty days; \
    To say within thine own deep sunken eyes, Were an all - eating shame, \
        and thriftless praise.How much more praise deserved thy beauty's use, If \
            thou couldst answer 'This fair child of mine Shall sum my count, \
        and make my old excuse' Proving his beauty by succession thine.This were \
            to be new made when thou art old, \
        And see thy blood warm when thou feel'st it cold. \
 \
        3 Look in thy glass and tell the face thou viewest, \
        Now is the time that face should form another, \
        Whose fresh repair if now thou not renewest, \
        Thou dost beguile the world, \
        unbless some mother.For where is she so fair whose uneared womb Disdains \
            the tillage of thy husbandry \
        ? Or who is he so fond will be the tomb, \
        Of his self - love to stop posterity \
        ? Thou art thy mother's glass and she in thee Calls back the lovely \
              April of her prime, \
        So thou through windows of thine age shalt see, \
        Despite of wrinkles this thy golden time.But if thou live remembered not \
            to be, \
        Die single and thine image dies with thee. \
 \
        4 Unthrifty loveliness why dost thou spend, \
        Upon thy self thy \
            beauty's legacy? Nature's bequest gives nothing but doth lend, And \
                being frank she lends to those are free \
        : Then beauteous niggard why dost thou abuse, \
        The bounteous largess given thee to give \
        ? Profitless usurer why dost thou use So great a sum of sums yet canst \
              not live \
        ? For having traffic with thy self alone, \
        Thou of thy self thy sweet self dost deceive, \
        Then how when nature calls thee to be gone, \
        What acceptable audit canst thou leave \
        ? Thy unused beauty must be tombed with thee, \
        Which used lives th' executor to be. \
 \
        5 Those hours that with gentle work did frame The lovely gaze where \
            every eye doth dwell Will play the tyrants to the very same, \
        And that unfair which fairly doth excel \
        : For never - resting time leads summer on To hideous winter and \
                          confounds him there, \
        Sap checked with frost and lusty leaves quite gone, \
        Beauty o'er-snowed and bareness every where: Then were not \
            summer's distillation left A liquid prisoner pent in walls of glass, \
        Beauty's effect with beauty were bereft, Nor it nor no remembrance what \
            it was.But flowers distilled though they with winter meet, \
        Leese but their show, \
        their substance still lives sweet. \
 \
        6 Then let not winter's ragged hand deface, In thee thy summer ere thou \
            be distilled \
        : Make sweet some vial; \
    treasure thou some place, \
        With beauty's treasure ere it be self-killed: That use is not forbidden \
            usury, \
        Which happies those that pay the willing loan;";

    char *string1 = "Hello this is string";
    char *string2 = "Another one";
    char *string3 = "Third one";
    stringvec_append(&vec, string1, strlen(string1));
    stringvec_append(&vec, string2, strlen(string2));
    stringvec_append(&vec, string3, strlen(string3));
    stringvec_append(&vec, shakespeare, strlen(shakespeare));

    char *returned1 = stringvec_get(&vec, 0);
    char *returned2 = stringvec_get(&vec, 1);
    char *returned3 = stringvec_get(&vec, 2);
    char *returned_shakespeare = stringvec_get(&vec, 3);
    TEST_ASSERT_EQUAL_STRING("Hello this is string", returned1);
    TEST_ASSERT_EQUAL_STRING("Another one", returned2);
    TEST_ASSERT_EQUAL_STRING("Third one", returned3);
    TEST_ASSERT_EQUAL_STRING(shakespeare, returned_shakespeare);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_basic_functionality);
    RUN_TEST(test_forgotten_null_terminator);
    RUN_TEST(test_out_of_bounds_access);
    RUN_TEST(test_grow_data);
    RUN_TEST(test_grow_indices);

    return UNITY_END();
}
