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

size_t readFAT32(file_desc_t *node, size_t offset, size_t size, char *buf);
size_t writeFAT32(file_desc_t *node, size_t offset, size_t size, char *buf);
int readdirFAT32(DIR *dirstream);
file_desc_t *findfileFAT32(file_desc_t *node, char *name);
int mkfileFAT32(file_desc_t *file);
int rmfileFAT32(file_desc_t *file);
int renameFAT32(file_desc_t *file, file_desc_t *newParent, char *origName);

mountpoint_t *mountFAT32(partition_t *partition);
void unmountFAT32(mountpoint_t *mountpoint);

#endif // _FAT32_H
