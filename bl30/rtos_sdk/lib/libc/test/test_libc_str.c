#include "unity_fixture.h"
#include "unity.h"
#include <string.h>
#include <stdio.h>

#define TEST_STRING_1   "Hello, Amlogic!"
#define TEST_STRING_2   "Hello, "
#define TEST_STRING_3   "Amlogic!"
#define TEST_STRING_4   "!cigolmA"

#define TEST_STRING_1_LEN   15 //no terminator
#define TEST_STRING_2_LEN   7 //no terminator
#define TEST_STRING_3_LEN   8 //no terminator

/*-----------------------------------------------------------*/
 
TEST_GROUP( Full_LIBC_STR );
 
/*-----------------------------------------------------------*/
 
TEST_SETUP( Full_LIBC_STR )
{
    /* set stuff up here */
}
 
/*-----------------------------------------------------------*/
 
TEST_TEAR_DOWN( Full_LIBC_STR )
{
    /* clean stuff up here */
}
 
/*-----------------------------------------------------------*/
 
TEST_GROUP_RUNNER( Full_LIBC_STR )
{
    RUN_TEST_CASE( Full_LIBC_STR, str_test );
}
 
/*-----------------------------------------------------------*/
 
TEST( Full_LIBC_STR, str_test )
{
    /* strlen test */
    TEST_ASSERT_TRUE(strlen(TEST_STRING_1) == TEST_STRING_1_LEN);

    /* strcmp test */
    TEST_ASSERT_TRUE(strcmp(TEST_STRING_2, TEST_STRING_1) < 0);
    TEST_ASSERT_TRUE(strcmp(TEST_STRING_1, TEST_STRING_2) > 0);
    TEST_ASSERT_TRUE(strcmp(TEST_STRING_1, TEST_STRING_1) == 0);

    /* strcpy test */
    char *dest = malloc(TEST_STRING_1_LEN + 1);

    memset(dest, 0, TEST_STRING_1_LEN + 1);
    strcpy(dest, TEST_STRING_1);
    TEST_ASSERT_TRUE(strcmp(dest, TEST_STRING_1) == 0);

    /* strncpy test */
    memset(dest, 0, TEST_STRING_1_LEN + 1);
    strncpy(dest, TEST_STRING_1, TEST_STRING_2_LEN + 1);
    dest[TEST_STRING_2_LEN] = '\0';
    TEST_ASSERT_TRUE(strcmp(dest, TEST_STRING_2) == 0);

    /* strcat test */
    memset(dest, 0, TEST_STRING_1_LEN + 1);
    strncpy(dest, TEST_STRING_1, TEST_STRING_2_LEN);

    strcat(dest, TEST_STRING_3);
    TEST_ASSERT_TRUE(strcmp(dest, TEST_STRING_1) == 0);

    /* strstr test */
    TEST_ASSERT_TRUE(strstr(TEST_STRING_1, TEST_STRING_3) != NULL);
    TEST_ASSERT_TRUE(strstr(TEST_STRING_1, TEST_STRING_4) == NULL);

    /* strchr test */
     TEST_ASSERT_TRUE(strchr(TEST_STRING_1, 'A') != NULL);
    TEST_ASSERT_TRUE(strchr(TEST_STRING_1, 'a') == NULL);

    /* strtok test */
    strtok(TEST_STRING_1, ",");
    TEST_ASSERT_TRUE(strcmp(strtok(NULL, " "), TEST_STRING_3) == 0);
}






