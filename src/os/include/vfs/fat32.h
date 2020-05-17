#ifndef _FAT32_H
#define _FAT32_H

//------------------------------------------------------------------------------------------
//				Includes
//------------------------------------------------------------------------------------------

#include <vfs/vfs.h>

#include <stdint.h>
#include <stddef.h>

//------------------------------------------------------------------------------------------
//				Constants
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Types
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Variables
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
//				Public Function
//------------------------------------------------------------------------------------------

int readFAT32(file_desc_t *node, size_t offset, int size, char *buf);
int writeFAT32(file_desc_t *node, size_t offset, int size, char *buf);
dirent *readdirFAT32(file_desc_t *node, int index);
file_desc_t *findfileFAT32(file_desc_t *node, char *name);
file_desc_t *mountFAT32(partition_t *partition);
file_desc_t *mkdirFAT32(file_desc_t *node, char *name);
file_desc_t *mkfileFAT32(file_desc_t *node, char *name);

#endif // _FAT32_H
