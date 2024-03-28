#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	assert(elemSize > 0);
	assert(numBuckets > 0);
	assert(hashfn != NULL);
	assert(comparefn != NULL);

	h -> numBuckets = numBuckets;
	h -> buckets = malloc(h->numBuckets * sizeof(vector));
	for(int i = 0; i < h->numBuckets; i++)
		VectorNew(&h->buckets[i], elemSize, freefn, 0);
	
	h -> hashFn = hashfn;
	h -> cmpFn = comparefn;
	h -> freeFn = freefn;

	h -> numElems = 0;

}

void HashSetDispose(hashset *h) {
	assert(h != NULL);
	for(int i = 0; i < h->numBuckets; i++) {
		VectorDispose(&h->buckets[i]);
	}
    free(h->buckets);
}

int HashSetCount(const hashset *h) {
	assert(h != NULL);
	return h->numElems;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData) {
	assert(h != NULL);
	assert(mapfn != NULL);
	for(int i = 0; i < h->numBuckets; i++) {
		VectorMap(&h->buckets[i], mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr) {
	assert(h != NULL);
	assert(elemAddr != NULL);
	
	int bucketIdx = h->hashFn(elemAddr, h->numBuckets);
	assert(bucketIdx >= 0 && bucketIdx < h->numBuckets);
	
	int elemIdxInVec = VectorSearch(&h->buckets[bucketIdx], elemAddr, h->cmpFn, 0, false);

	if(elemIdxInVec == -1) VectorAppend(&h->buckets[bucketIdx], elemAddr);
	else VectorReplace(&h->buckets[bucketIdx], elemAddr, elemIdxInVec);

	h->numElems++;
}

void *HashSetLookup(const hashset *h, const void *elemAddr) {
	assert(h != NULL);
	assert(elemAddr != NULL);
	int bucketIdx = h->hashFn(elemAddr, h->numBuckets);
	int elemIdxInVec = VectorSearch(&h->buckets[bucketIdx], elemAddr, h->cmpFn, 0, false);
	if(elemIdxInVec == -1) return NULL;
	else return VectorNth(&h->buckets[bucketIdx], elemIdxInVec);
}