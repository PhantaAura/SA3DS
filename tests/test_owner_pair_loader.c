#define _XOPEN_SOURCE 700
/* macOS headers hide mkdtemp() in the restricted POSIX namespace. */
/* Request Darwin interfaces *before* any system header is included. */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif
#include "owner_assets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void write_file(const char *dir, const char *name, const char *text) {
    char path[512];
    assert(snprintf(path,sizeof(path),"%s/%s",dir,name) > 0);
    FILE *f=fopen(path,"wb"); assert(f);
    assert(fwrite(text,1,strlen(text),f)==strlen(text));
    assert(fclose(f)==0);
}
static void remove_file(const char *dir, const char *name) {
    char path[512];
    assert(snprintf(path,sizeof(path),"%s/%s",dir,name) > 0);
    unlink(path);
}
static void check(const char *romfs, const char *sd, int expected,
                  SA3DSOwnerSource source, char first, char second) {
    uint8_t *c=(uint8_t*)1, *t=(uint8_t*)1;
    size_t cs=777,ts=777;
    SA3DSOwnerSource found=SA3DS_OWNER_SOURCE_SD;
    int ok=sa3ds_load_owner_pair(romfs,sd,&c,&cs,&t,&ts,&found);
    assert(ok==expected && found==source);
    if (ok) {
        assert(c && t && cs && ts);
        assert(c[0]==(uint8_t)first && t[0]==(uint8_t)second);
    } else assert(!c && !t && !cs && !ts);
    sa3ds_free_file(c); sa3ds_free_file(t);
}
int main(void) {
    char tmp[]="/tmp/sa3ds-owner-XXXXXX";
    assert(mkdtemp(tmp));
    char r[512],s[512];
    snprintf(r,sizeof(r),"%s/rom",tmp);
    snprintf(s,sizeof(s),"%s/sd",tmp);
    assert(mkdir(r,0700)==0 && mkdir(s,0700)==0);
    check(r,s,0,SA3DS_OWNER_SOURCE_NONE,0,0);
    write_file(s,"sonic.sa3c","C-sd");
    write_file(s,"sonic.sa3t","T-sd");
    check(r,s,1,SA3DS_OWNER_SOURCE_SD,'C','T');
    write_file(r,"sonic.sa3c","R-rom");
    /* Partial RomFS MUST NOT mix with SD textures. */
    check(r,s,1,SA3DS_OWNER_SOURCE_SD,'C','T');
    write_file(r,"sonic.sa3t","V-rom");
    check(r,s,1,SA3DS_OWNER_SOURCE_ROMFS,'R','V');
    write_file(r,"sonic.sa3t","");
    check(r,s,1,SA3DS_OWNER_SOURCE_SD,'C','T');
    remove_file(s,"sonic.sa3t");
    check(r,s,0,SA3DS_OWNER_SOURCE_NONE,0,0);
    /* A sparse oversized file must fail BEFORE allocating Old 3DS memory. */
    char big[1024]; assert(snprintf(big,sizeof(big),"%s/oversized",s)>0);
    FILE *f=fopen(big,"wb"); assert(f);
    assert(ftruncate(fileno(f),(off_t)SA3DS_OWNER_ASSET_MAX_BYTES+1)==0);
    assert(fclose(f)==0);
    uint8_t *data=(uint8_t*)1; size_t size=123;
    assert(!sa3ds_read_file(big,&data,&size) && !data && !size);
    assert(!sa3ds_read_file(NULL,&data,&size) && !data && !size);
    assert(strcmp(sa3ds_owner_source_name(SA3DS_OWNER_SOURCE_SD),"SD")==0);
    remove_file(r,"sonic.sa3c"); remove_file(r,"sonic.sa3t");
    remove_file(s,"sonic.sa3c"); remove_file(s,"oversized");
    assert(rmdir(r)==0 && rmdir(s)==0 && rmdir(tmp)==0);
    puts("owner pair loader: PASS (fallback, pair atomicity, precedence, cap, cleanup)");
    return 0;
}
