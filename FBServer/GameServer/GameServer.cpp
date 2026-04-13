#include "pch.h"

#include <iostream>
#include "CorePch.h"
#include "ThreadManager.h"



int main()
{
	for (int32 i = 0; i < 5; ++i) {
		GthreadManager->Launch(ThreadMain);
	}

	GthreadManager->Join();
}
