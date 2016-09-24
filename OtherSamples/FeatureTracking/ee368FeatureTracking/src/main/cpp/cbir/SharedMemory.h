#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

class SharedMemory {
public:
	SharedMemory(int aKey, int aMemSize = 1) {
		Construct(aKey, aMemSize);
	}
	void Construct(int aKey, int aMemSize = 1) {
		iStatus = 0;
		iKey = aKey;
		iMemSize = aMemSize;

		iID = shmget(iKey, iMemSize, IPC_CREAT | 0666);
		if (iID < 0) {
			iStatus = 1;
			return;
		}

		iMemoryPointer = shmat(iID, NULL, 0);
		if (iMemoryPointer == (void *)-1) {
			iStatus = 2;
			return;
		}
	}

public:
	int iStatus;
	int iMemSize;
	int iKey;
	int iID;
	void *iMemoryPointer;
};

#endif
