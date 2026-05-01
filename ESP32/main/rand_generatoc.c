

#include "mbedtls/aes.h"

#include "esp_random.h"



mbedtls_aes_context generate_aes128(){
	mbedtls_aes_context aes;
	
	
	
	mbedtls_aes_init(&aes);

    // 2. Ustawienie klucza (dla AES-128 klucz ma 16 bajtów / 128 bitów)
    mbedtls_aes_setkey_enc(&aes, key, 128);

	
}