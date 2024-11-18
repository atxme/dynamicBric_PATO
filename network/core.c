////////////////////////////////////////////////////////////
//  assert scource file
//  
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xAssert.h"
#include "core.h"

///////////////////////////////////////////////////////
/// createSocket
///////////////////////////////////////////////////////
int createSocket(NetworkSocketCreation* p_tSocketParameters)
{
	
	X_ASSERT(p_tSocketParameters != NULL);
	X_ASSERT(p_tSocketParameters->t_iDomain == AF_INET || p_tSocketParameters->t_iDomain == AF_INET6);
	X_ASSERT(p_tSocketParameters->t_iType == SOCK_STREAM || p_tSocketParameters->t_iType == SOCK_DGRAM);
	X_ASSERT(p_tSocketParameters->t_iProtocol == 0);

	int l_iSocket = socket(p_tSocketParameters->t_iDomain, p_tSocketParameters->t_iType, p_tSocketParameters->t_iProtocol);

	X_ASSERT(l_iSocket == NETWORK_OK);

	return l_iSocket;
}

///////////////////////////////////////////////////////
/// bindSocket
///////////////////////////////////////////////////////
int bindSocket(int p_iSocket, NetworkAddress* p_pttAddress, struct sockaddr* p_pttSockaddr)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_pttAddress != NULL);
	X_ASSERT(p_pttSockaddr != NULL);

	// convert to local structure
	NetworkAddress l_ttAddress = *p_pttAddress;




}