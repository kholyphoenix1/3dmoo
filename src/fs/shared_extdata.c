/*
* Copyright (C) 2014 - plutoo
* Copyright (C) 2014 - ichfly
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "mem.h"
#include "handles.h"
#include "fs.h"

/* ____ File implementation ____ */

static u32 sharedextdfile_Write(file_type* self, u32 ptr, u32 sz, u64 off, u32 flush_flags, u32* written_out)
{
    FILE* fd = self->type_specific.sysdata.fd;
    *written_out = 0;

    if (off >> 32) {
        ERROR("64-bit offset not supported.\n");
        return -1;
    }

    if (fseek64(fd, off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if (b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    if (mem_Read(b, ptr, sz) != 0) {
        ERROR("mem_Write failed.\n");
        free(b);
        return -1;
    }

    u32 write = fwrite(b, 1, sz, fd);
    if(write != sz) {
        ERROR("fwrite failed\n");
        free(b);
        return -1;
    }

    *written_out = write;
    free(b);

    return 0; // Result
}

static u32 sharedextdfile_Read(file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out)
{
    FILE* fd = self->type_specific.sharedextd.fd;
    *read_out = 0;

    if(off >> 32) {
        ERROR("64-bit offset not supported.\n");
        return -1;
    }

    if(fseek64(fd, off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if(b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    u32 read = fread(b, 1, sz, fd);
    if(read != sz) {
        ERROR("fread failed\n");
        free(b);
        return -1;
    }

    if(mem_Write(b, ptr, read) != 0) {
        ERROR("mem_Write failed.\n");
        free(b);
        return -1;
    }

    *read_out = read;
    free(b);

    return 0; // Result
}

static u64 sharedextdfile_GetSize(file_type* self)
{
    return self->type_specific.sharedextd.sz;
}

static u32 sharedextdfile_CreateFile(archive* self, file_path path, u32 size)
{
    char *p = malloc(256);

    char tmp[256];

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
        self->type_specific.sharedextd.path,
        fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if (!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        free(p);
        return 0;
    }

    int result;

    int fd = open(p, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);

    if (fd == -1)
    {
        result = errno;
        if (result == EEXIST) result = 0x82044BE;
    }
    else {
        result = ftruncate(fd, size);
        if (result != 0) result = errno;
        close(fd);
    }


    if (result == ENOSPC) result = 0x86044D2;
    free(p);
    return result;
}

static u32 sharedextdfile_Close(file_type* self)
{
    // Close file and free yourself
    fclose(self->type_specific.sharedextd.fd);
    free(self);

    return 0;
}



/* ____ FS implementation ____ */

static bool sharedextd_FileExists(archive* self, file_path path)
{
    char p[256], tmp[256];
    struct stat st;

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
             self->type_specific.sharedextd.path,
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return false;
    }

    return stat(p, &st) == 0;
}

static u32 sharedextd_OpenFile(archive* self, file_path path, u32 flags, u32 attr)
{
    char p[256], tmp[256];

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
             self->type_specific.sharedextd.path,
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }

    FILE* fd = fopen(p, "rb");
    if (fd == NULL) {
        if (flags & OPEN_CREATE) {
            fd = fopen(p, "wb");
            if (fd == NULL) {
                ERROR("Failed to open/create sharedexd, path=%s\n", p);
                return 0;
            }
        } else {
            ERROR("Failed to open sharedexd, path=%s\n", p);
            return 0;
        }
    }
    fclose(fd);

    switch (flags& (OPEN_READ | OPEN_WRITE)) {
    case 0:
        ERROR("Error open without write and read fallback to read only, path=%s\n", p);
        fd = fopen(p, "rb");
        break;
    case OPEN_READ:
        fd = fopen(p, "rb");
        break;
    case OPEN_WRITE:
        DEBUG("--todo-- write only, path=%s\n", p);
        fd = fopen(p, "r+b");
        break;
    case OPEN_WRITE | OPEN_READ:
        fd = fopen(p, "r+b");
        break;
    }

    fseek(fd, 0, SEEK_END);
    u32 sz = 0;

    if((sz=ftell(fd)) == -1) {
        ERROR("ftell() failed.\n");
        fclose(fd);
        return 0;
    }

    // Create file object
    file_type* file = calloc(sizeof(file_type), 1);

    if(file == NULL) {
        ERROR("calloc() failed.\n");
        fclose(fd);
        return 0;
    }

    file->type_specific.sharedextd.fd = fd;
    file->type_specific.sharedextd.sz = (u64) sz;

    // Setup function pointers.
    file->fnWrite = &sharedextdfile_Write;
    file->fnRead = &sharedextdfile_Read;
    file->fnGetSize = &sharedextdfile_GetSize;
    file->fnClose = &sharedextdfile_Close;

    file->handle = handle_New(HANDLE_TYPE_FILE, (uintptr_t)file);
    return file->handle;
}


static void sharedextd_Deinitialize(archive* self)
{
    // Free yourself
    free(self);
}

static u32 sharedextd_DeleteFile(archive* self, file_path path)
{
    char p[256], tmp[256];

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
             self->type_specific.sharedextd.path,
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if (!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }

    return remove(p);
}

int sharedextd_DeleteDir(archive* self, file_path path)
{
    char p[256], tmp[256];

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
             self->type_specific.sharedextd.path,
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if (!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }
#ifdef _MSC_VER 
    return _rmdir(p);
#else
    return rmdir(p);
#endif
}

u32 sharedextd_ReadDir(dir_type* self, u32 ptr, u32 entrycount, u32* read_out)
{
    u32 current = 0;
    struct dirent* ent;

    while (current < entrycount) {
        errno = 0;
        if ((ent = readdir(self->dir)) == NULL) {
            if (errno == 0) {
                // Dir empty. Memset region?
                return 0;
            }
            else {
                ERROR("readdir() failed.\n");
                return -1;
            }
        }
        current++;
    }

    // Convert file-name to UTF16 and copy.
    u16 utf16[256];
    for (u32 i = 0; i<256; i++)
        utf16[i] = ent->d_name[i];

    mem_Write((uint8_t*)utf16, ptr, sizeof(utf16));

    // XXX: 8.3 name @ 0x20C
    mem_Write8(ptr + 0x215, 0xA); //unknown
    // XXX: 8.3 ext @ 0x216

    mem_Write8(ptr + 0x21A, 0x1); // Unknown
    mem_Write8(ptr + 0x21B, 0x0); // Unknown

    struct stat st;

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", self->path, ent->d_name);
    if (stat(path,&st) != 0 ) {
        ERROR("Failed to stat: %s\n", path);
        return -1;
    }
    // Is directory flag
    if (S_ISDIR(st.st_mode)) {
        mem_Write8(ptr + 0x21C, 0x1);

        mem_Write8(ptr + 0x216, ' '); // 8.3 file extension
        mem_Write8(ptr + 0x217, ' ');
        mem_Write8(ptr + 0x218, ' ');
    }
    else { // Is directory flag

    // XXX: hidden flag @ 0x21D
    // XXX: archive flag @ 0x21E
    // XXX: readonly flag @ 0x21F

        mem_Write8(ptr + 0x21C, 0x0);
        mem_Write32(ptr + 0x220, st.st_size);

#if defined(ENV64BIT)
        mem_Write32(ptr + 0x224, (st.st_size >> 32));
#else
        mem_Write32(ptr + 0x224, 0);
#endif
    }


    *read_out = current;
    return 0;
}

static u32 sharedextd_OpenDir(archive* self, file_path path)
{
    // Create file object
    dir_type* dir = (dir_type*)malloc(sizeof(dir_type));

    dir->f_path = path;
    dir->self = self;

    // Setup function pointers.
    dir->fnRead = &sharedextd_ReadDir;

    char tmp[256];

    // Generate path on host file system
    snprintf(dir->path, 256, "sys/shared/%s/%s",
        self->type_specific.sharedextd.path,
        fs_PathToString(path.type, path.ptr, path.size, tmp, 256));


    dir->dir = opendir(dir->path);

    if (dir->dir == NULL) {
        ERROR("Dir not found: %s.\n", dir->path);
        free(dir);
        return 0;
    }

    rewinddir(dir->dir);
    return handle_New(HANDLE_TYPE_DIR, (uintptr_t)dir);
}


archive* sharedextd_OpenArchive(file_path path)
{
    // Extdata needs a binary path with a 12-byte id.
    if(path.type != PATH_BINARY || path.size != 12) {
        ERROR("Unknown SharedExtData path.\n");
        return NULL;
    }

    archive* arch = calloc(sizeof(archive), 1);

    if(arch == NULL) {
        ERROR("malloc failed.\n");
        return NULL;
    }

    // Setup function pointers
    arch->fnCreateFile   = &sharedextdfile_CreateFile;
    arch->fnRenameFile   = NULL;
    arch->fnDeleteFile   = &sharedextd_DeleteFile;
    arch->fnCreateDir    = NULL;
    arch->fnDeleteDir    = &sharedextd_DeleteDir;
    arch->fnRenameDir    = NULL;
    arch->fnOpenDir      = &sharedextd_OpenDir;
    arch->fnFileExists   = &sharedextd_FileExists;
    arch->fnOpenFile     = &sharedextd_OpenFile;
    arch->fnDeinitialize = &sharedextd_Deinitialize;
    arch->result = 0;

    u8 buf[12];

    if(mem_Read(buf, path.ptr, 12) != 0) {
        ERROR("Failed to read path.\n");
        free(arch);
        return NULL;
    }

    snprintf(arch->type_specific.sharedextd.path,
        sizeof(arch->type_specific.sharedextd.path),
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
        buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);

    char p[256];

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/",
        arch->type_specific.sharedextd.path);

    DIR* dir = opendir(p);
    if (dir == NULL)
    {
        //sharedextdfile_Close(arch);
        return NULL;
    }
    closedir(dir);
    return arch;
}
