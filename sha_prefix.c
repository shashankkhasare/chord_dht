#include<openssl/sha.h>
#include<stdio.h>
#include<string.h>

int main(int argc, char *argv[])
{
	unsigned char hash[SHA_DIGEST_LENGTH + 1];
	SHA1(argv[1] , strlen(argv[1]), hash);
	hash[20] =0;
	printf("%s", hash);

	return 0; 
}
