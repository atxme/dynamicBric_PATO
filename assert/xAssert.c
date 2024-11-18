////////////////////////////////////////////////////////////
//  assert scource file
//  
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xAssert.h"


////////////////////////////////////////////////////////
/// loadLogFilePath
////////////////////////////////////////////////////////
bool loadLogFilePath(char* p_ptcLogFilePath)
{
	if (p_ptcLogFilePath == NULL)
	{
		printf("ASSERT Error: invalid log file path\n");
		return 0;
	}

	s_ptcLogFilePath = p_ptcLogFilePath;

	bool l_bIsValid = false;

	FILE* l_ptFile = fopen(s_ptcLogFilePath, "w");

	if (l_ptFile != NULL)
	{
		l_bIsValid = true;
		fclose(l_ptFile);
	}

	else
	{
		printf("ASSERT Error: invalid log file path or impossible to open the file\n");
	}

	return l_bIsValid;
	
}

////////////////////////////////////////////////////////
/// assert
////////////////////////////////////////////////////////
void assert(uint8_t* p_pFileName, uint32_t p_uiLine, void* p_ptLog)
{


	if (s_bIsLogToFile)
	{
		//open the log file 
		FILE* l_ptFile = fopen(s_ptcLogFilePath, "a");

		if (l_ptFile == NULL)
		{
			printf("ASSERT Error: impossible to open the log file\n");
			printf("ASSERT Error: %s, %d\n", p_pFileName, p_uiLine);
			printf("ASSERT log message: %s\n", (char*)p_ptLog);
			return;
		}

		else
		{
			//write the log message to the file
			fprintf(l_ptFile, "ASSERT Error: %s, %d\n", p_pFileName, p_uiLine);

			if (p_ptLog != NULL)
			{
				fprintf(l_ptFile, "ASSERT log message: %s\n", (char*)p_ptLog);
			}

		}
	}

	else
	{
		printf("ASSERT Error: %s, %d\n", p_pFileName, p_uiLine);

		if (p_ptLog != NULL)
		{
			printf("ASSERT log message: %s\n", (char*)p_ptLog);
		}

	}

#ifdef ASSERT_LOOP
	printf("Start infinite loop\n");
	while (1);
#elif ASSERT_EXIT
	exit(1);
#elif ASSERT_CONTINUE
	return;
#else
#error "No assert option is defined"
#endif // ASSERT_CONTINUE


}

