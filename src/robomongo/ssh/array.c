#include "robomongo/ssh/private.h"
#include <stdlib.h>

/*
 * Adds pointer to array.
 * You should pass your array (sometype**) and size of it (int) by reference.
 * Array can be NULL, in this case new array will be allocated
 * @array Address of (void **) array
 * @currentsize Address of size
 * @data Pointer that should be added to array
 */
int rbm_array_add(void ***array, int *currentsize, void *data) {
    if (!*array && *currentsize)
        return RBM_ERROR;

    int newsize = *currentsize + 1;

    // Acts like malloc when (*array == NULL)
    void **newarray = realloc(*array, newsize * sizeof(void*));
    if (!newarray)
        return RBM_ERROR;

    newarray[*currentsize] = data;
    *array = newarray;
    *currentsize = newsize;
    return RBM_SUCCESS;
}

int rbm_array_remove(void ***array, int *currentsize, void *data) {
    int newsize = *currentsize - 1;
    void **newarray;

    for (int i = 0; i < *currentsize; i++) {
        if ((*array)[i] != data)
            continue;

        if (*currentsize == 1) {
            newarray = NULL;
        } else {
            newarray = malloc(newsize * sizeof(void*));
            if (!newarray)
                return RBM_ERROR;

            memcpy(newarray, *array, i * sizeof(void*));

            if (i + 1 < *currentsize)
                memcpy(newarray + i, *array + i + 1, (*currentsize - i - 1) * sizeof(void*));
        }

        free(*array);
        *currentsize = newsize;
        *array = newarray;
        return RBM_SUCCESS;
    }

    return RBM_ERROR;
}
