#include <unity.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fleur.h>
#include <time.h>
// Some global file descriptor
FILE* inheader;
FILE* infull;
struct header my_header;

typedef struct tester {
    BloomFilter *bf;
    char *buf[];
}tester;

char* GenerateTestValue(uint64_t length){
    unsigned char* buf = calloc(length, sizeof(char));
    for (uint64_t i = 0; i < length; i++){
		buf[i] = rand() % 256;
    }

	return (char*)buf;
}

tester * GenerateExampleFilter(uint64_t capacity, double p, uint64_t samples) {
    // seeding the prng
    srand((unsigned) time(0));
    BloomFilter * bf = Initialize(capacity, p);
    struct tester *test = malloc(sizeof(BloomFilter*) + (samples * sizeof(char*)));
    test->bf = bf;
    unsigned char* str = "foobar";
	test->bf->Data = str; 
    test->bf->datasize = strlen(str);
	for (uint64_t i = 0; i < samples; i++) {
        test->buf[i] = GenerateTestValue(100);
        Add(test->bf, test->buf[i], 100);
	}
	return test;
}

void test_reading_header(void)
{
    header my_header;
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
    BloomFilter * bf;
    bf = BloomFilterFromFile(infull);
    TEST_ASSERT_EQUAL_UINT64(450, bf->M);
    // TEST_ASSERT_EQUAL_STRING("toto\n", my_bloom->Data);
    print_filter(bf);
    free(bf->v);
    free(bf->Data);
}

void test_writing(void){
    const char* path = "./writing-test.bloom";
    FILE* f = fopen(path, "wb");
    struct tester *test = GenerateExampleFilter(1000, 0.001, 1000);
    BloomFilterToFile(test->bf, f);
    fclose(f);
}

void test_fingerprint(void){
    char str[80];
    BloomFilter * bf = Initialize(100000, 0.01);
    uint64_t* fp = calloc(7, sizeof(uint64_t));
	uint64_t expected[7] = {20311, 36825, 412501, 835777, 658914, 853361, 307361};
	Fingerprint(bf, "bar", strlen("bar"), &fp);
    for (uint64_t i = 0; i < bf->h->k ; i++ ){
		if (fp[i] != expected[i]) {
            sprintf(str, "Wrong fingerprint: %ld vs. %ld", fp[i], expected[i]);
		    TEST_FAIL_MESSAGE(str);
			break;
		}
	}
    free(bf->v);
    free(fp);
}

void test_initialize(void){
    BloomFilter *bf = Initialize(10000, 0.001);
    char str[80];
    print_filter(bf);
	if (bf->h->k != 10) {
		TEST_FAIL_MESSAGE("k does not match expectation!\n");
        (strerror(errno));
	}
	if (bf->h->m != 143775 ){
        sprintf(str, "m does not match expectation: %lu\n", bf->h->m);
		TEST_FAIL_MESSAGE(str);
	}
	if (bf->M != (uint64_t)ceil((double)bf->h->m/64)){
        sprintf(str, "M does not match expectation: %lu\n", bf->M);
		TEST_FAIL_MESSAGE(str);
	}
	for (uint64_t i = 0 ; i < bf->M; i++) {
		if (bf->v[i] != 0) {
			TEST_FAIL_MESSAGE("Filter value is not initialized to zero!\n");
		}
	}
    free(bf->v);
}

//This tests the checking of values against a given filter
void test_checking(void) {
	uint64_t capacity = 100000;
	double p = 0.001;
	uint64_t samples = 100000;
    struct tester *test = GenerateExampleFilter(capacity, p, samples);
    print_filter(test->bf);

    for (uint64_t i = 0; i < samples; i ++){
        if (Check(test->bf, test->buf[i], 100) == 0) {
            TEST_FAIL_MESSAGE("Did not find test value in filter!");
		}
    }
    char* str = "this is not in the filter";
    if (Check(test->bf, str, strlen(str)) == 1) {
        TEST_FAIL_MESSAGE("This value is not in the filter!");
    }
    free(test->bf->v);
    for (int i = 0; i < samples; i++){
        free(test->buf[i]);
    }
    free(test);
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
    RUN_TEST(test_checking);
    // TODO make complete serialization test
    RUN_TEST(test_writing);

    return UNITY_END();
}