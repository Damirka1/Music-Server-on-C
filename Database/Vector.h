#ifndef VECTOR_HEADER
#define VECTOR_HEADER

struct Vector
{
	void* pData;
	int DataSize;
	int Size;
};


struct Vector* CreateVector(int DataSize);

void Release(struct Vector* vector);

int Size(struct Vector* vector);

void Push(struct Vector* vector, void* pData);

void* Get(struct Vector* vector, int index);

void Clear(struct Vector* vector);



#endif
