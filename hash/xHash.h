////////////////////////////////////////////////////////////
//  hash header file
//  defines the hash types and functions
//
// general discloser: copy or share the file is forbidden
// Written : 11/01/2025
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_HASH_H_
#define XOS_HASH_H_

#include <stdint.h>
#include <stddef.h>

// Hash error codes
#define XOS_HASH_OK           0
#define XOS_HASH_ERROR       -1
#define XOS_HASH_INVALID     -2

// Hash output sizes (in bytes)
#define XOS_HASH_SHA256_SIZE    32
#define XOS_HASH_SHA384_SIZE    48
#define XOS_HASH_SHA512_SIZE    64
#define XOS_HASH_SHA3_256_SIZE  32
#define XOS_HASH_SHA3_384_SIZE  48
#define XOS_HASH_SHA3_512_SIZE  64
#define XOS_HASH_KECCAK_256_SIZE 32

// Hash types
typedef enum {
    XOS_HASH_TYPE_SHA256,
    XOS_HASH_TYPE_SHA384,
    XOS_HASH_TYPE_SHA512,
    XOS_HASH_TYPE_SHA3_256,
    XOS_HASH_TYPE_SHA3_384,
    XOS_HASH_TYPE_SHA3_512,
    XOS_HASH_TYPE_KECCAK_256
} t_hashAlgorithm;

//////////////////////////////////
/// @brief Calculate hash of data
/// @param p_eType : hash type
/// @param p_ptData : input data
/// @param p_ulSize : input size
/// @param p_ptHash : output hash buffer
/// @param p_pulHashSize : size of hash buffer
/// @return : success or error code
//////////////////////////////////
int xHashCalculate(t_hashAlgorithm p_eType, 
                   const void* p_ptData, 
                   size_t p_ulSize,
                   uint8_t* p_ptHash, 
                   size_t* p_pulHashSize);

//////////////////////////////////
/// @brief Initialize hash context
/// @param p_eType : hash type
/// @param p_ptContext : hash context
/// @return : success or error code
//////////////////////////////////
int xHashInit(t_hashAlgorithm p_eType, void* p_ptContext);

//////////////////////////////////
/// @brief Update hash with data
/// @param p_eType : hash type
/// @param p_ptContext : hash context
/// @param p_ptData : input data
/// @param p_ulSize : input size
/// @return : success or error code
//////////////////////////////////
int xHashUpdate(t_hashAlgorithm p_eType, 
                void* p_ptContext,
                const void* p_ptData, 
                size_t p_ulSize);

//////////////////////////////////
/// @brief Finalize hash calculation
/// @param p_eType : hash type
/// @param p_ptContext : hash context
/// @param p_ptHash : output hash buffer
/// @param p_pulHashSize : size of hash buffer
/// @return : success or error code
//////////////////////////////////
int xHashFinalize(t_hashAlgorithm p_eType, 
                  void* p_ptContext,
                  uint8_t* p_ptHash, 
                  size_t* p_pulHashSize);

#endif // XOS_HASH_H_
