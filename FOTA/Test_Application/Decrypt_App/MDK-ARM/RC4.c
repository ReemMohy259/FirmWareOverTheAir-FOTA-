#include "RC4.h"

// RC4 key scheduling algorithm (KSA)
void rc4_key_schedule(const unsigned char *key, size_t key_len, unsigned char *S) {
    unsigned char temp;
    unsigned char j = 0;

    // Initialize the state array S
    for (int i = 0; i < 256; i++) {
        S[i] = i;
    }

    // Perform the key scheduling algorithm
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % key_len]) % 256;
        // Swap S[i] and S[j]
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

// RC4 stream cipher (encryption and decryption)
void rc4_crypt(const unsigned char *input, size_t len, const unsigned char *key, size_t key_len, unsigned char *output) {
    unsigned char S[256];
    unsigned char i = 0, j = 0, k;
    unsigned char temp;

    // Key scheduling
    rc4_key_schedule(key, key_len, S);

    // Encryption / Decryption
    for (size_t n = 0; n < len; n++) {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        // Swap S[i] and S[j]
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;

        // Generate keystream byte
        k = S[(S[i] + S[j]) % 256];

        // XOR keystream with input to produce output
        output[n] = input[n] ^ k;
    }
}
