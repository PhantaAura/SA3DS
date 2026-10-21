#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emerald_coast_visual.h"

static unsigned char *read_all(const char *path,size_t *size) {
    FILE *f=fopen(path,"rb");
    if(!f)return NULL;
    assert(fseek(f,0,SEEK_END)==0);
    long n=ftell(f);assert(n>0);*size=(size_t)n;
    assert(fseek(f,0,SEEK_SET)==0);
    unsigned char *p=(unsigned char*)malloc(*size);assert(p);
    assert(fread(p,1,*size,f)==*size);assert(fclose(f)==0);return p;
}

int main(int argc,char **argv) {
    assert(argc==2||argc==3);int synthetic=argc==3;size_t size;unsigned char *data=read_all(argv[1],&size);assert(data);
    SA3DSECVisual v;
    assert(sa3ds_ec_visual_open(&v,data,size)==SA3DS_EC_VISUAL_OK);
    assert(v.model_count==(synthetic?1u:183u));assert(v.mesh_count==(synthetic?1u:578u));assert(v.instance_count==(synthetic?1u:480u));
    assert(v.vertex_count==(synthetic?3u:22237u));assert(v.index_count==(synthetic?3u:58794u));assert(v.material_count==(synthetic?1u:237u));
    assert(v.textures.texture_count==(synthetic?1u:103u));
    SA3DSECModel model;SA3DSECMesh mesh;SA3DSECInstance instance;SA3DSECMaterial material;SA3DSECVertex vertex;uint16_t index;
    assert(sa3ds_ec_model(&v,0,&model));assert(sa3ds_ec_mesh(&v,0,&mesh));
    assert(sa3ds_ec_instance(&v,0,&instance));assert(sa3ds_ec_material(&v,0,&material));
    assert(sa3ds_ec_vertex(&v,0,&vertex));assert(sa3ds_ec_index(&v,0,&index));
    assert(model.mesh_count>0);assert(mesh.index_count%3u==0);assert(material.texture_id<v.textures.texture_count);assert(index<v.vertex_count);
    unsigned char saved=data[size-1];data[size-1]^=0x80;
    assert(sa3ds_ec_visual_open(&v,data,size)==SA3DS_EC_VISUAL_ERR_PACKAGE);
    data[size-1]=saved;free(data);
    puts("Emerald Coast native visual reader: PASS");return 0;
}
