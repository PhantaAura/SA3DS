#ifndef SA3DS_REFERENCE_PROFILE_H
#define SA3DS_REFERENCE_PROFILE_H
#define WIN32_LEAN_AND_MEAN
#define PSAPI_VERSION 1
#include <windows.h>
#include <bcrypt.h>
#include <psapi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PROFILE_SHA "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
static int profile_file(const wchar_t *path) {
 BCRYPT_ALG_HANDLE algorithm=NULL; BCRYPT_HASH_HANDLE hash=NULL;
 HANDLE file=INVALID_HANDLE_VALUE; unsigned char bytes[65536],digest[32]; char hex[65]; DWORD count=0; int ok=0;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
 file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(file==INVALID_HANDLE_VALUE)goto done;
 for(;;){if(!ReadFile(file,bytes,sizeof(bytes),&count,NULL))goto done;if(!count)break;
  if(BCryptHashData(hash,bytes,count,0)<0)goto done;}
 if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)goto done;
 for(unsigned i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
 ok=!strcmp(hex,PROFILE_SHA);
 done: if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);return ok;
}
#endif
