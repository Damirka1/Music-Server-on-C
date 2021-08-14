#include "Vector.h"
#include <stdlib.h>
#include <string.h>

struct Vector* CreateVector(int DataSize)
{
	struct Vector* vec = malloc(sizeof(vec));
	
	vec->pData = NULL;
	vec->DataSize = DataSize;
	vec->Size = 0;

	return vec;
}


void Release(struct Vector* vec)
{
	Clear(vec);
	free(vec);
}

void Push(struct Vector* vec, void* pData)
{
	void* pDataTemp = NULL;
	int bSize = 0;
	if(vec->Size)
	{
		bSize = vec->Size * vec->DataSize;

		pDataTemp = malloc(bSize);

		memcpy(pDataTemp, vec->pData, bSize);

		free(vec->pData);
	}

	vec->Size += 1;
	vec->pData = malloc(bSize + vec->DataSize);
	
	if(pDataTemp)
		memcpy(vec->pData, pDataTemp, bSize);

	memcpy(vec->pData + bSize, pData, vec->DataSize);

}

void* Get(struct Vector* vec, int index)
{
	return vec->pData + (index * vec->DataSize);
}

void Clear(struct Vector* vec)
{
	vec->Size = 0;
	free(vec->pData);
}

