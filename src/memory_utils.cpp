#include "memory_utils.h"

#if defined _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <cstdio>
#include <string.h>
#endif

namespace memory_utils {

double GetWorkingSetSize()
{
#if defined _WIN32
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
		return (double)pmc.WorkingSetSize;
    }
	return -1;
#else
    FILE* file = fopen("/proc/self/status", "r");
    if (file == 0)
    	return -1;
    	
    double result = -1;
    char* line = new char[128];

    while (fgets(line, 128, file))
	{
		if (strncmp(line, "VmSize:", 7) == 0) 
		{
			sscanf(line, "VmSize: %lf", &result);
			break;
		}
    }
    
    fclose(file);
    delete[] line;
	return (result==-1.0 ? -1.0 : result*1024.0);
#endif
}

double GetWorkingSetSizeMB()
{
	double workingSetSize = GetWorkingSetSize();

	if (workingSetSize != -1)
	{
		workingSetSize /= 1024.0;
		workingSetSize /= 1024.0;
	}

	return workingSetSize;
}

} // namespace memory_utils