////////////////////////////////////////////////////////////
//  horodateur source file
//  implements timestamp functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xOsHorodateur.h"
#include "xAssert.h"
#include <time.h>
#include <string.h>

#define XOS_HORODATEUR_BUFFER_SIZE 48

static char s_cTimeBuffer[XOS_HORODATEUR_BUFFER_SIZE];

const char* xHorodateurGetString(void)
{
    time_t l_tNow;
    struct tm* l_ptTm;

    // Obtenir le temps actuel
    time(&l_tNow);
    l_ptTm = localtime(&l_tNow);

    if (l_ptTm == NULL)
    {
        return NULL;
    }

    // Formater dans le buffer statique
    if (strftime(s_cTimeBuffer, XOS_HORODATEUR_BUFFER_SIZE,
        "%Y-%m-%d %H:%M:%S", l_ptTm) == 0)
    {
        return NULL;
    }

    return s_cTimeBuffer;
}


uint32_t xHorodateurGet(void)
{
    time_t l_tNow;
    struct tm* l_ptTm;

    // Obtenir le temps actuel
    time(&l_tNow);
    l_ptTm = localtime(&l_tNow);

    X_ASSERT(l_ptTm != NULL);
   
	return (uint32_t)l_tNow;
}
