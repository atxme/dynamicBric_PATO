/**
 * @file error.h
 *
 * Error management for the MRPiZ programming API.
 *
 * @version 0.1
 * @author Matthias Brun (matthias.brun@eseo.fr)
 *
 */

#ifndef MRPIZ_ERROR_H_
#define MRPIZ_ERROR_H_

/**
 * @defgroup mrpiz_error Error management for MRPiZ robot programming.
 *
 * @brief
 * Functions and macros useful for error management when programming an MRPiZ.
 *
 * @{
 */

/**
 * @enum mrpiz_error_t
 *
 * @brief List of MRPiZ errors
 *
 * Error codes used to set errno during library function execution.
 *
 * Note: The enumeration starts at 1 (errno is never set to zero when an error occurs (man errno)).
 *
 */
typedef enum
{
	// Errors specific to Intox:

	MRPIZ_INTOX_E_SYSTEM				= 1,		/**< Internal problem using a system call (see previous logs from perror). */

	MRPIZ_INTOX_E_SOCKET,						/**< Error during the creation of the simulator connection socket. */
	MRPIZ_INTOX_E_CONNECT,						/**< Error during connection to the Intox simulator (default error). */
	MRPIZ_INTOX_E_CONNECT_REFUSED,				/**< Error during connection to the Intox simulator: connection refused. */
	MRPIZ_INTOX_E_CONNECT_NET,					/**< Error during connection to the Intox simulator: network unreachable. */
	MRPIZ_INTOX_E_CONNECT_HOST,					/**< Error during connection to the Intox simulator: server unreachable. */
	MRPIZ_INTOX_E_CONNECT_TIMEOUT,				/**< Error during connection to the Intox simulator: connection timeout. */

	MRPIZ_INTOX_E_ACCESS, 						/**< Access error to the Intox simulator. */
	MRPIZ_INTOX_E_LOST, 						/**< Lost access to the Intox simulator. */
	MRPIZ_INTOX_E_CMD,	 						/**< Invalid command to the Intox simulator. */

	// MRPiZ errors:

	MRPIZ_E_INIT,							/**< MRPiZ is not initialized. */
	MRPIZ_E_MOTOR_CMD,						/**< Invalid motor command. */
	MRPIZ_E_MOTOR_ID,						/**< Invalid motor identifier. */
	MRPIZ_E_PROXY_SENSOR_ID,				/**< Invalid proximity sensor identifier. */

	MRPIZ_E_SYSTEM,							/**< Internal problem using a system call (see previous logs from perror). */

	// Errors specific to MRPiZ:
	MRPIZ_E_UART,							/**< Error during UART communication between PiZ and STM32. */

	//  MRPIZ_E_UNDEFINED						/**< End of the error list. */

} mrpiz_error_t;


/**
 * @fn char const * mrpiz_error_msg()
 *
 * @brief Returns the error message (associated with the errno value of the mrpiz lib).
 *
 * @return the error message
 */
char const * mrpiz_error_msg();

/**
 * @fn void mrpiz_error_print(char * msg)
 *
 * @brief Displays the error message (associated with the errno value of the mrpiz lib).
 *
 *
 * @param msg the prefix of the message to display (NULL if no prefix)
 */
void mrpiz_error_print(char * msg);

/** @} */
// end of mrpiz_error group

#endif /* MRPIZ_ERROR_H_ */
