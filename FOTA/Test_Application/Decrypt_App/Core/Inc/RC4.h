/*
 * RC4.h
 *
 *  Created on: Dec 12, 2024
 *      Author: REEM
 */

#ifndef INC_RC4_H_
#define INC_RC4_H_


#include <stdio.h>
#include <string.h>

#define KEY_SIZE 16  // Key size in bytes (128 bits)

void rc4_crypt(const unsigned char *input, size_t len, const unsigned char *key, size_t key_len, unsigned char *output);
void rc4_key_schedule(const unsigned char *key, size_t key_len, unsigned char *S);


#endif /* INC_RC4_H_ */
