#include <unity.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <bloom.h>
// Some global file descriptor
FILE* inheader;
FILE* infull;
struct header my_header;


void test_reading_header(void)
{
    size_t elements_read = fread(&my_header, sizeof(my_header), 1, inheader);
    if(elements_read == 0){
        TEST_FAIL_MESSAGE(strerror(errno));
    }
    print_header(my_header);
    TEST_ASSERT_EQUAL_UINT64 (1, my_header.version);
    TEST_ASSERT_EQUAL_UINT64 (354253067, my_header.n);
    // Jean-Michel Apeupré
    TEST_ASSERT_EQUAL_FLOAT (0.0001, my_header.p);
    TEST_ASSERT_EQUAL_UINT64 (14, my_header.k);
    TEST_ASSERT_EQUAL_UINT64 (6791072655, my_header.m);
    TEST_ASSERT_EQUAL_UINT64 (354249652, my_header.N);
}

void test_reading_full(void)
{
    // Get header
    size_t elements_read = fread(&my_header, sizeof(my_header), 1, infull);
    if(elements_read == 0){
        TEST_FAIL_MESSAGE(strerror(errno));
    }

    TEST_ASSERT_EQUAL_UINT64 (1, my_header.version);
    TEST_ASSERT_EQUAL_UINT64 (1000, my_header.n);
    // Jean-Michel Apeupré
    TEST_ASSERT_EQUAL_FLOAT (0.000001, my_header.p);
    TEST_ASSERT_EQUAL_UINT64 (20, my_header.k);
    TEST_ASSERT_EQUAL_UINT64 (28755, my_header.m);
    TEST_ASSERT_EQUAL_UINT64 (3, my_header.N);

    struct BloomFilter * my_bloom;
    my_bloom = BloomFilterFromFile(&my_header, infull);

    TEST_ASSERT_EQUAL_UINT64 (450, my_bloom->M);
    TEST_ASSERT_EQUAL_STRING("toto\n", my_bloom->Data);
    print_filter(my_bloom);
}

void test_fingerprint(void){
    char str[80];
    BloomFilter * filter = Initialize(100000, 0.01);
    uint64_t* fp = calloc(7, sizeof(uint64_t));
	uint64_t expected[7] = {20311, 36825, 412501, 835777, 658914, 853361, 307361};
	Fingerprint("bar", strlen("bar"), &fp, filter);
    for (uint64_t i = 0; i < filter->k ; i++ ){
		if (fp[i] != expected[i]) {
            sprintf(str, "Wrong fingerprint: %ld vs. %ld", fp[i], expected[i]);
		    TEST_FAIL_MESSAGE(str);
			break;
		}
	}
}

void test_initialize(void){
    BloomFilter *bf = Initialize(10000, 0.001);
    char str[80];
    print_filter(bf);
	if (bf->k != 10) {
		TEST_FAIL_MESSAGE("k does not match expectation!\n");
        (strerror(errno));
	}
	if (bf->m != 143775 ){
        sprintf(str, "m does not match expectation: %lu\n", bf->m);
		TEST_FAIL_MESSAGE(str);
	}
	if (bf->M != (uint64_t)ceil((double)bf->m/64)){
        sprintf(str, "M does not match expectation: %lu\n", bf->M);
		TEST_FAIL_MESSAGE(str);
	}
	for (uint64_t i = 0 ; i < bf->M; i++) {
		if (bf->v[i] != 0) {
			TEST_FAIL_MESSAGE("Filter value is not initialized to zero!\n");
		}
	}

}

void setUp() {
    inheader = fopen("header.bin", "rb");
    if (inheader==NULL){
        TEST_FAIL_MESSAGE(strerror(errno));
    }
    infull = fopen("datatest.bloom", "rb");
    if (infull==NULL){
        TEST_FAIL_MESSAGE(strerror(errno));
    }
 }

void tearDown() {
    fclose(inheader);
    fclose(infull);
 }

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_reading_header);
    RUN_TEST(test_reading_full);
    RUN_TEST(test_initialize);
    RUN_TEST(test_fingerprint);

    return UNITY_END();
}