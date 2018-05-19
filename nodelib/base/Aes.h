/*
 * Aes.h
 *
 *  Created on: Dec 27, 2016
 *      Author: zhangyalei
 */

#ifndef AES_H_
#define AES_H_

#include <stdint.h>

//aes128位加密算法(ecb模式)
//ECB模式（电子密码本模式：Electronic codebook）
//ECB是最简单的块密码加密模式，加密前根据加密块大小（如AES为128位）分成若干块，之后将每块使用相同的密钥单独加密，解密同理。
void aes128_ecb_encrypt(uint8_t* input, const uint8_t* key, uint8_t *output);
void aes128_ecb_decrypt(uint8_t* input, const uint8_t* key, uint8_t *output);

//aes128位加密算法(cbc模式)
//CBC模式（密码分组链接：Cipher-block chaining）
//CBC模式对于每个待加密的密码块在加密前会先与前一个密码块的密文异或然后再用加密器加密。第一个明文块与一个叫初始化向量的数据块异或。
void aes128_cbc_encrypt(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void aes128_cbc_decrypt(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

//文件加密解密
int file_encrypt(const char* file_path);
int file_decrypt(const char* file_path);

//文件夹加密解密
int folder_encrypt(const char* folder_path);
int folder_decrypt(const char* folder_path);

void phex(uint8_t* str);
void test_encrypt_ecb(void);
void test_decrypt_ecb(void);
void test_encrypt_ecb_verbose(void);
void test_encrypt_cbc(void);
void test_decrypt_cbc(void);

#endif //AES_H_
