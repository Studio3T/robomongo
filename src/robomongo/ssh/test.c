#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "robomongo/ssh/private.h"

static int *elem1;
static int *elem2;
static int *elem3;
static int *elem4;
static int *elem5;

int add_one_element() {
    int **array = NULL;
    int size = 0;

    rbm_array_add((void***)&array, &size, elem1);
    assert(size == 1);
    assert(array[0] == elem1);
    assert(*array[0] == 100);
    free(array);
    return 0;
}

int add_two_elements() {
    int **array = NULL;
    int size = 0;

    rbm_array_add((void***)&array, &size, elem1);
    assert(size == 1);
    rbm_array_add((void***)&array, &size, elem2);
    assert(size == 2);
    assert(array[0] == elem1);
    assert(*array[0] == 100);
    assert(array[1] == elem2);
    assert(*array[1] == 200);
    free(array);
    return 0;
}

int array_remove_when_only_single() {
    int **array = NULL;
    int size = 0;

    rbm_array_add((void***)&array, &size, elem1);
    rbm_array_remove((void***)&array, &size, elem1);
    assert(size == 0);
    assert(*elem1 == 100);
    free(array);
    return 0;
}

int array_remove_when_only_two() {
    int **array = NULL;
    int size = 0;

    rbm_array_add((void***)&array, &size, elem1);
    rbm_array_add((void***)&array, &size, elem2);
    rbm_array_remove((void***)&array, &size, elem1);
    assert(size == 1);
    assert(array[0] == elem2);
    assert(*array[0] == 200);
    free(array);
    return 0;
}

int array_remove_when_only_two_variation() {
    int **array = NULL;
    int size = 0;

    rbm_array_add((void***)&array, &size, elem1);
    rbm_array_add((void***)&array, &size, elem2);
    rbm_array_remove((void***)&array, &size, elem2);
    assert(size == 1);
    assert(array[0] == elem1);
    assert(*array[0] == 100);
    free(array);
    return 0;
}

int array_remove_when_five_elements() {
    int **array = NULL;
    int size = 0;
    rbm_array_add((void***)&array, &size, elem1);
    rbm_array_add((void***)&array, &size, elem2);
    rbm_array_add((void***)&array, &size, elem3);
    rbm_array_add((void***)&array, &size, elem4);
    rbm_array_add((void***)&array, &size, elem5);
    assert(size == 5);

    rbm_array_remove((void***)&array, &size, elem2);
    rbm_array_remove((void***)&array, &size, elem4);
    assert(size == 3);
    assert(array[0] == elem1);
    assert(array[1] == elem3);
    assert(array[2] == elem5);
    free(array);
    return 0;
}

int remove_last_element() {
    int **array = NULL;
    int size = 0;
    rbm_array_add((void***)&array, &size, elem1);
    rbm_array_add((void***)&array, &size, elem2);
    rbm_array_add((void***)&array, &size, elem3);
    assert(size == 3);

    rbm_array_remove((void***)&array, &size, elem3);
    assert(size == 2);
    assert(array[0] == elem1);
    assert(array[1] == elem2);
    free(array);
    return 0;
}

int add_with_incorrect_params() {
    int **array = NULL;
    int size = 1;
    int rc = rbm_array_add((void***)&array, &size, elem1);
    assert(rc == -1);
    assert(size == 1); // should not be modified
    free(array);
    return 0;
}

void init() {
    elem1 = malloc(sizeof(int));
    elem2 = malloc(sizeof(int));
    elem3 = malloc(sizeof(int));
    elem4 = malloc(sizeof(int));
    elem5 = malloc(sizeof(int));
    *elem1 = 100;
    *elem2 = 200;
    *elem3 = 300;
    *elem4 = 400;
    *elem5 = 500;
}

void cleanup() {
    free(elem1);
    free(elem2);
    free(elem3);
    free(elem4);
    free(elem5);
}

int main(int argc, char *argv[]) {
    init();
    add_one_element();
    add_two_elements();
    array_remove_when_only_two();
    array_remove_when_only_single();
    array_remove_when_only_two_variation();
    array_remove_when_five_elements();
    remove_last_element();
    add_with_incorrect_params();
    cleanup();
    printf("All tests completed successfully.\n");
}
