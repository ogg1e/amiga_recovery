/*

Amiga Recovery - Recover files from an Amiga AFFS disk image
Copyright 2009-2015 - Michael Kohn (mike@mikekohn.net)
http://www.mikekohn.net/

Released under GPL

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "affs.h"
#include "fileio.h"

#define ST_FILE -3
#define ST_ROOT 1 
#define ST_USERDIR 2 
#define ST_SOFTLINK 3 
#define ST_LINKDIR 4 

/* hash_name function copied from http://lclevy.free.fr/adflib/adf_info.html */
static unsigned int hash_name(unsigned char *name)
{
uint32_t hash, l; /* sizeof(int)>=2 */
int i;

  l = hash = strlen((char *)name);

  for(i = 0; i < l; i++)
  {
    hash = hash * 13;
    hash = hash + toupper(name[i]); /* not case sensitive */
    hash = hash & 0x7ff;
  }

  /* 0 < hash < 71 in the case of 512 byte blocks */
  hash = hash % ((BSIZE / 4) - 56);

  return(hash);
}

void read_fileheader(FILE * in, struct _amiga_bootblock *bootblock, struct _amiga_partition *partition, struct _amiga_fileheader *fileheader, unsigned int block)
{
int namelen;
int t;

  fseek(in, partition->start + (block * bootblock->blksz), SEEK_SET);

  fileheader->type = read_int(in);
  fileheader->header_key = read_int(in);
  fileheader->high_seq = read_int(in);
  fileheader->data_size = read_int(in);
  fileheader->first_data = read_int(in);
  fileheader->checksum = read_int(in);
  //unsigned int datablocks[512/4-56];  //FIXME - wrong size?
  for (t = 0; t < (512 / 4 - 56); t++)
  {
    fileheader->datablocks[t] = read_int(in);
  }
  fileheader->unused1 = read_int(in);
  fileheader->uid = read_short(in);
  fileheader->gid = read_short(in);
  fileheader->protect = read_int(in);
  fileheader->byte_size = read_int(in);
  //unsigned char comment[80]; // first char is len
  namelen = getc(in);
  for (t = 0; t < 79; t++)
  {
    fileheader->comment[t] = getc(in);
  }
  fileheader->comment[namelen] = 0;
  //unsigned char unused2[12];
  fseek(in, 12, SEEK_CUR);
  fileheader->days = read_int(in);
  fileheader->mins = read_int(in);
  fileheader->ticks = read_int(in);
  //unsigned char filename[32]; // first char is len, last char is unused
  namelen = getc(in);
  for (t = 0; t < 31; t++)
  {
    fileheader->filename[t] = getc(in);
  }
  fileheader->filename[namelen] = 0;
  fileheader->unused3 = read_int(in);
  fileheader->read_entry = read_int(in);
  fileheader->next_link = read_int(in);
  //unsigned int unused4[5];
  fseek(in, 5 * 4, SEEK_CUR);
  fileheader->hash_chain = read_int(in);
  fileheader->parent = read_int(in);
  fileheader->extension = read_int(in);
  fileheader->sec_type = read_int(in);
}

void read_file_ext(FILE * in, struct _amiga_bootblock *bootblock, struct _amiga_partition *partition, struct _amiga_file_ext *file_ext, unsigned int block)
{
int t;

  fseek(in, partition->start + (block * bootblock->blksz), SEEK_SET);

  file_ext->type = read_int(in);
  file_ext->header_key = read_int(in);
  file_ext->high_seq = read_int(in);
  //unsigned int unused1;
  //unsigned int unused2;
  fseek(in, 2 * 4, SEEK_CUR);
  file_ext->checksum = read_int(in);
  //unsigned int datablocks[BSIZE/4-56]
  for (t = 0; t<(512 / 4 - 56); t++)
  {
    file_ext->datablocks[t] = read_int(in);
  }
  //unsigned int info[46];
  fseek(in, 46 * 4, SEEK_CUR);
  file_ext->unused3 = read_int(in);
  file_ext->parent = read_int(in);
  file_ext->extension = read_int(in);
  file_ext->sec_type = read_int(in);
}

void read_directory(FILE * in, struct _amiga_bootblock *bootblock, struct _amiga_partition *partition, struct _amiga_directory *directory, unsigned int block)
{
int namelen;
int t;

  fseek(in, partition->start + (block * bootblock->blksz), SEEK_SET);

  directory->type = read_int(in);
  directory->header_key = read_int(in);
  //unsigned int unused1[3];
  fseek(in, 3 * 4, SEEK_CUR);
  directory->checksum = read_int(in);
  //unsigned int hash_table[512/4-56];
  for (t = 0; t < (512 / 4 - 56); t++)
  {
    directory->hash_table[t] = read_int(in);
  }
  //unsigned int unused2[2];
  fseek(in, 2 * 4, SEEK_CUR);
  directory->uid = read_short(in);
  directory->gid = read_short(in);
  directory->protect = read_int(in);
  // FIXME - WTF?
  //directory->unused3 = read_int(in);
  //unsigned char comment[80]; // first char is len
  namelen = getc(in);
  for (t = 0; t < 79; t++)
  {
    directory->comment[t] = getc(in);
  }
  directory->comment[namelen]  =0;
  //unsigned char unused4[12];
  fseek(in, 12, SEEK_CUR);
  directory->days= read_int(in);
  directory->mins = read_int(in);
  directory->ticks = read_int(in);
  //unsigned char dirname[32]; // first char is len, last char is unused
  namelen = getc(in);
  for (t = 0; t < 31; t++)
  {
    directory->dirname[t] = getc(in);
  }
  directory->dirname[namelen] = 0;
  //unsigned int unused5[2];
  fseek(in, 2 * 4, SEEK_CUR);
  directory->next_link = read_int(in);
  //unsigned int unused6[5];
  fseek(in, 5 * 4, SEEK_CUR);
  directory->hash_chain = read_int(in);
  directory->parent = read_int(in);
  directory->extension = read_int(in);
  directory->sec_type = read_int(in);
}

void read_datablock(FILE * in, struct _amiga_bootblock *bootblock, struct _amiga_partition *partition, struct _amiga_datablock *datablock, unsigned int block)
{
int t;

  fseek(in, partition->start + (block * bootblock->blksz), SEEK_SET);

  datablock->type = read_int(in);
  datablock->header_key = read_int(in);
  datablock->seq_num = read_int(in);
  datablock->data_size = read_int(in);
  datablock->next_data = read_int(in);
  datablock->checksum = read_int(in);
  for (t = 0; t < 512 - 24; t++)
  {
    datablock->data[t] = getc(in);
  }
}

static void print_hash_info(FILE *in, struct _amiga_rootblock *rootblock, struct _amiga_bootblock *bootblock, struct _amiga_partition *partition, unsigned int block)
{
struct _amiga_directory directory;
struct _amiga_fileheader fileheader;
int sec_type;

  sec_type = get_sec_type(in, bootblock, partition,block);

  if (sec_type == ST_USERDIR)
  {
    read_directory(in, bootblock, partition, &directory, block);
    //print_directory(&directory);
    printf("    (Dir) %s\n", directory.dirname);

    if (directory.hash_chain != 0)
    {
      print_hash_info(in, rootblock, bootblock, partition, directory.hash_chain);
    }
  }
    else
  if (sec_type == ST_FILE)
  {
    read_fileheader(in, bootblock, partition, &fileheader, block);
    //print_fileheader(&fileheader);
    printf("%9d %s\n", fileheader.byte_size, fileheader.filename);

    if (fileheader.hash_chain != 0)
    {
      print_hash_info(in, rootblock, bootblock, partition, fileheader.hash_chain);
    }
  }
    else
  {
    printf("Unknown sec_type %d\n", sec_type);
  }
}

void list_directory(FILE *in, struct _amiga_bootblock *bootblock, struct _pwd *pwd)
{
//struct _amiga_rootblock rootblock;
int t;

  //read_rootblock(in,bootblock,partition,&rootblock);

  for (t = 0; t < 72; t++)
  {
    if (pwd->dir_hash[t] != 0)
    {
      print_hash_info(in, &pwd->rootblock, bootblock, &pwd->partition, pwd->dir_hash[t]);
    }
  }
}

static void print_file_at_block(FILE *in, struct _amiga_bootblock *bootblock, struct _amiga_partition *partition, struct _amiga_fileheader *fileheader, FILE *out)
{
//struct _amiga_datablock datablock;
struct _amiga_file_ext file_ext;
unsigned int *datablocks;
unsigned int bytes_left;
unsigned char buffer[bootblock->blksz];
unsigned int next_block;
int curr;
int len;
int t;

  datablocks = fileheader->datablocks;
  bytes_left = fileheader->byte_size;
  next_block = fileheader->extension;

  // print_fileheader(fileheader);

  while(bytes_left > 0)
  {
    for (curr = 71; curr >= 0; curr--)
    {
      if (datablocks[curr] == 0) break;

      fseek(in, partition->start + (datablocks[curr] * bootblock->blksz), SEEK_SET); 
      if (bytes_left>bootblock->blksz)
      {
        len=fread(buffer, 1, bootblock->blksz, in);
        bytes_left -= bootblock->blksz;
      }
        else
      {
        len=fread(buffer, 1, bytes_left, in);
        bytes_left -= bytes_left;
      }

      if (out == NULL)
      {
        for (t = 0; t < len; t++) { putchar(buffer[t]); }
      }
        else
      {
        fwrite(buffer, 1, len, out);
      }
    }

    if (next_block==0) break;

    read_file_ext(in, bootblock, partition, &file_ext, next_block);
    if (file_ext.type != 16)
    {
      printf("Error: File extension wasn't read right?  Bug?\n");
      break;
    }
    //print_file_ext(&file_ext);
    next_block = file_ext.extension;
    datablocks = file_ext.datablocks;
  }

  if (out == NULL) { printf("\n"); }
}

void print_file(FILE *in, struct _amiga_bootblock *bootblock, struct _pwd *pwd, char *filename, FILE *out)
{
struct _amiga_directory directory;
struct _amiga_fileheader fileheader;
unsigned int block;
int sec_type;

  block = pwd->dir_hash[hash_name((unsigned char*)filename)];

  while(block != 0)
  {
    sec_type = get_sec_type(in, bootblock, &pwd->partition, block);

    if (sec_type == ST_USERDIR)
    {
      read_directory(in, bootblock, &pwd->partition, &directory, block);

      if (directory.hash_chain == 0) break;

      block = directory.hash_chain;
    }
      else
    if (sec_type == ST_FILE)
    {
      read_fileheader(in, bootblock, &pwd->partition, &fileheader, block);

      if (strcmp((char *)fileheader.filename, filename) == 0)
      {
        print_file_at_block(in, bootblock, &pwd->partition, &fileheader, out);
        return;
      }

      if (fileheader.hash_chain == 0) break;

      block = fileheader.hash_chain;
    }
      else
    {
      printf("Unknown sec_type %d\n", sec_type);
      break;
    }
  }

  printf("Error: file '%s' not found\n", filename);
}

void print_fileheader(struct _amiga_fileheader *fileheader)
{
int t;

  printf("================== File Header ===================\n");
  printf("             type: %d\n", fileheader->type);
  printf("       header_key: %d\n", fileheader->header_key);
  printf("         high_seq: %d\n", fileheader->high_seq);
  printf("        data_size: %d\n", fileheader->data_size);
  printf("       first_data: %d\n", fileheader->first_data);
  printf("         checksum: %d\n", fileheader->checksum);
  printf("       datablocks:");
  for (t = 0; t < (512 / 4 - 56); t++)
  {
    if (t != 0) printf(" ");
    printf("%d", fileheader->datablocks[t]);
  }
  printf("\n");
  printf("              uid: %d\n", fileheader->uid);
  printf("              gid: %d\n", fileheader->gid);
  printf("          protect: %d\n", fileheader->protect);
  printf("        byte_size: %d\n", fileheader->byte_size);
  printf("          comment: %s\n", fileheader->comment);
  printf("             days: %d\n", fileheader->days);
  printf("             mins: %d\n", fileheader->mins);
  printf("            ticks: %d\n", fileheader->ticks);
  printf("         filename: %s\n", fileheader->filename);
  printf("       read_entry: %d\n", fileheader->read_entry);
  printf("        next_link: %d\n", fileheader->next_link);
  printf("       hash_chain: %d\n", fileheader->hash_chain);
  printf("           parent: %d\n", fileheader->parent);
  printf("        extension: %d\n", fileheader->extension);
  printf("         sec_type: %d\n", fileheader->sec_type);
}

void print_file_ext(struct _amiga_file_ext *file_ext)
{
int t;

  printf("================== File Extension ===================\n");
  printf("             type: %d\n", file_ext->type);
  printf("       header_key: %d\n", file_ext->header_key);
  printf("         high_seq: %d\n", file_ext->high_seq);
  printf("         checksum: %d\n", file_ext->checksum);
  printf("       datablocks: ");
  for (t = 0; t < (512 / 4 - 56); t++)
  {
    if (t != 0) { printf(" "); }
    printf("%d", file_ext->datablocks[t]);
  }
  printf("\n");
  printf("           parent: %d\n", file_ext->parent);
  printf("        extension: %d\n", file_ext->extension);
  printf("         sec_type: %d\n", file_ext->sec_type);
}

void print_directory(struct _amiga_directory *directory)
{
int t;

  printf("================== Directory ===================\n");
  printf("             type: %d\n", directory->type);
  printf("       header_key: %d\n", directory->header_key);
  printf("         checksum: %d\n", directory->checksum);
  printf("       hash_table:");
  for (t = 0; t < (512 / 4 - 56); t++)
  {
    if (t != 0) { printf(" "); }
    printf("%d", directory->hash_table[t]);
  }
  printf("\n");
  printf("              uid: %d\n", directory->uid);
  printf("              gid: %d\n", directory->gid);
  printf("          protect: %d\n", directory->protect);
  printf("          comment: %s\n", directory->comment);
  printf("             days: %d\n", directory->days);
  printf("             mins: %d\n", directory->mins);
  printf("            ticks: %d\n", directory->ticks);
  printf("          dirname: %s\n", directory->dirname);
  printf("        next_link: %d\n", directory->next_link);
  printf("       hash_chain: %d\n", directory->hash_chain);
  printf("           parent: %d\n", directory->parent);
  printf("        extension: %d\n", directory->extension);
  printf("         sec_type: %d\n", directory->sec_type);
}

void print_datablock(struct _amiga_datablock *datablock)
{
char ascii[17];
int ptr;
int t;

  printf("================== Datablock ===================\n");
  printf("             type: %d\n", datablock->type);
  printf("       header_key: %d\n", datablock->header_key);
  printf("          seq_num: %d\n", datablock->seq_num);
  printf("        data_size: %d\n", datablock->data_size);
  printf("        next_data: %d\n", datablock->next_data);
  printf("         checksum: %d\n", datablock->checksum);
  printf("             data:\n");

  if (datablock->type != 8) return;
  ptr = 0;

  for (t = 0; t < datablock->data_size; t++)
  {
    if ((t % 16) == 0)
    {
      ascii[ptr] = 0;
      if (ptr != 0) printf("  %s",ascii);
      printf("\n");
      ptr = 0;
    }

    if ((t % 16) != 0) printf(" ");
    printf("%02x", datablock->data[t]);

    if (datablock->data[t] >= 32 && datablock->data[t] < 127)
    { ascii[ptr++] = datablock->data[t]; }
      else
    { ascii[ptr++] = '.'; }
  }

  //FIXME - stuff missing at bottom..
}

int ch_dir(FILE *in, struct _amiga_bootblock *bootblock, struct _pwd *pwd, char *dirname)
{
struct _amiga_directory directory;
struct _amiga_fileheader fileheader;

  if (strcmp(dirname, "..")==0)
  {
    if (pwd->parent_dir == 0) { return -1; }

    if (pwd->parent_dir==pwd->rootblock.header_key)
    {
      memcpy(pwd->dir_hash, pwd->rootblock.hash_table, (BSIZE / 4 - 56) * 4);
      pwd->cwd[0] = 0;
      pwd->parent_dir = 0;
    }
      else
    {
      read_directory(in, bootblock, &pwd->partition, &directory, pwd->parent_dir);
      memcpy(pwd->dir_hash, directory.hash_table, (BSIZE / 4 - 56) * 4);
      int l = strlen(pwd->cwd) - 2;
      while(l >= 0)
      {
        if (pwd->cwd[l] == '/') break;
        pwd->cwd[l--] = 0;
      }
    }
  }
    else
  {
    int block = pwd->dir_hash[hash_name((unsigned char*)dirname)];

    while(block != 0)
    {
      unsigned int sec_type = get_sec_type(in,bootblock,&pwd->partition,block);

      if (sec_type == ST_USERDIR)
      {
        read_directory(in,bootblock,&pwd->partition,&directory,block);

        if (strcmp((char *)directory.dirname, dirname)==0)
        {
          memcpy(pwd->dir_hash, directory.hash_table, (BSIZE / 4 - 56) * 4);
          pwd->parent_dir = directory.parent;
          strcat(pwd->cwd, dirname);
          strcat(pwd->cwd, "/");
          return 0;
        }

        if (directory.hash_chain == 0) break;
        block = directory.hash_chain;
      }
        else
      if (sec_type == ST_FILE)
      {
        read_fileheader(in, bootblock, &pwd->partition, &fileheader, block);

        if (fileheader.hash_chain == 0) break;

        block = fileheader.hash_chain;
      }
        else
      {
        printf("Unknown sec_type %d\n", sec_type);
        break;
      }
    }

    printf("Error: No such directory\n");
    return -1;
  }

  return 0;
}

int get_sec_type(FILE * in, struct _amiga_bootblock *bootblock, struct _amiga_partition *partition, unsigned int block)
{
  fseek(in, partition->start + (block * bootblock->blksz + (bootblock->blksz - 4)), SEEK_SET);

  return read_int(in);
}


