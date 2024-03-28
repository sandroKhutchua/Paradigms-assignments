#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

static const int kDefaultAlloc = 10;
void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation) {
    assert(elemSize > 0);
    assert(initialAllocation >= 0);

    v -> elemSize = elemSize;
    v -> allocLen = (initialAllocation != 0) ? initialAllocation : kDefaultAlloc;
    v -> data = malloc(v->elemSize * v->allocLen);
    // v -> reallocInc = v -> allocLen;
    v -> logLen = 0;

    v -> freeFn = freeFn;
}

void VectorDispose(vector *v) {
    assert(v != NULL);
    if(v -> freeFn != NULL) {
        for(int i = 0; i < v->logLen; i++) {
            v->freeFn(VectorNth(v, i));
        }
    }
    free(v->data);
}

int VectorLength(const vector *v) {
    assert(v != NULL);
    return v -> logLen;
}

void *VectorNth(const vector *v, int position) {
    assert(v != NULL);
    assert(position >= 0);
    assert(position < v->logLen);
    void *ptr = (char*)v->data + position * v->elemSize;
    return ptr;
}

void VectorReplace(vector *v, const void *elemAddr, int position) {
    assert(v != NULL);
    assert(position >= 0);
    assert(position < v->logLen);
    assert(elemAddr != NULL);
    void *ptr = VectorNth(v, position);
    if(v->freeFn != NULL) v -> freeFn(ptr);
    memcpy(ptr, elemAddr, v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position) {
    assert(v != NULL);
    assert(elemAddr != NULL);
    assert(position >= 0);
    assert(position <= v->logLen);
    if(v->logLen == v->allocLen) {
        GrowVector(v);
    }
    int totalBytes = v->logLen * v->elemSize;
    int bytesToMove = totalBytes - position * v->elemSize;
    char *ptr;
    if(position < v->logLen) ptr = (char*)VectorNth(v, position);
    else ptr = (char*)v->data + totalBytes;
    memmove(ptr + v->elemSize, ptr, bytesToMove);
    memcpy(ptr, elemAddr, v->elemSize);
    v->logLen++;
}

void VectorAppend(vector *v, const void *elemAddr) {
    VectorInsert(v, elemAddr, v->logLen);
}

void VectorDelete(vector *v, int position) {
    assert(v != NULL);
    assert(position >= 0);
    assert(position < v->logLen);
    if(v -> freeFn != NULL) v -> freeFn(VectorNth(v, position));
    if(position < v->logLen - 1) {
        int totalBytes = v->logLen * v->elemSize;
        int bytesToMove = totalBytes - (position + 1) * v->elemSize;
        char *ptr = (char*)VectorNth(v, position);
        memmove(ptr, ptr + v->elemSize, bytesToMove);
    }
    v->logLen--;
    // if(v->logLen <= v->allocLen - v->reallocInc) {
    //     ShrinkVector(v);
    // }
}

void VectorSort(vector *v, VectorCompareFunction compare) {
    assert(v != NULL);
    assert(compare != NULL);
    qsort(v->data, v->logLen, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData) {
    assert(v != NULL);
    assert(mapFn != NULL);
    for(int i = 0; i < v->logLen; i++) {
        mapFn(VectorNth(v, i), auxData);
    }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted) {
    assert(v != NULL);
    assert(key != NULL);
    assert(searchFn != NULL);
    assert(startIndex >= 0 && startIndex <= v->logLen);
    char *startPtr = (char*)v->data + startIndex * v->elemSize;
    char *resPtr;
    if(isSorted) {
        resPtr = (char*)bsearch(key, startPtr, v->logLen - startIndex, v->elemSize, searchFn);
    } else {
        size_t numOfElems = v->logLen - startIndex;
        resPtr = (char*)lfind(key, startPtr, &numOfElems, v->elemSize, searchFn);
    }
    if(resPtr == NULL) return kNotFound;
    return (resPtr - (char*)v->data) / v->elemSize;
}

void GrowVector(vector *v) {
    v->allocLen *= 2;
    // v->allocLen += v->reallocInc;
    v->data = realloc(v->data, v->allocLen * v->elemSize);
    assert(v->data != NULL);
}