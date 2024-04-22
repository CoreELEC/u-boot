#include "unity_fixture.h"
#include "unity.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/*-----------------------------------------------------------*/
 
TEST_GROUP( Full_LIBC_CHR );
 
/*-----------------------------------------------------------*/
 
TEST_SETUP( Full_LIBC_CHR )
{
    /* set stuff up here */
}
 
/*-----------------------------------------------------------*/
 
TEST_TEAR_DOWN( Full_LIBC_CHR )
{
    /* clean stuff up here */
}
 
/*-----------------------------------------------------------*/
 
TEST_GROUP_RUNNER( Full_LIBC_CHR )
{
    RUN_TEST_CASE( Full_LIBC_CHR, chr_test );
}
 
/*-----------------------------------------------------------*/
 
TEST( Full_LIBC_CHR, chr_test )
{   
    // Test isalpha
    TEST_ASSERT_TRUE(isalpha('a') != 0);
    TEST_ASSERT_TRUE(isalpha('1') == 0);

    // Test isdigit
    TEST_ASSERT_TRUE(isdigit('9') != 0);
    TEST_ASSERT_TRUE(isdigit('a') == 0);

    // Test isalnum
    TEST_ASSERT_TRUE(isalnum('b') != 0);
    TEST_ASSERT_TRUE(isalnum('!') == 0);

    // Test isupper and islower
    TEST_ASSERT_TRUE(isupper('Z') != 0);
    TEST_ASSERT_TRUE(islower('z') != 0);
    TEST_ASSERT_TRUE(isupper('a') == 0);
    TEST_ASSERT_TRUE(islower('A') == 0);

    // Test tolower and toupper
    TEST_ASSERT_TRUE(tolower('A') == 'a');
    TEST_ASSERT_TRUE(toupper('a') == 'A');

    // Test isspace
    TEST_ASSERT_TRUE(isspace(' ') != 0);
    TEST_ASSERT_TRUE(isspace('\t') != 0);
    TEST_ASSERT_TRUE(isspace('x') == 0);
}