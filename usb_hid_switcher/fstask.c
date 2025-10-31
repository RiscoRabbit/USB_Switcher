#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "lfs.h"

// LittleFS configuration for Pico flash
#define FLASH_TARGET_OFFSET (1024 * 1024)  // 1MB from start of flash
#define SAMPLE_FILENAME "sample.txt"

// Variables used by the filesystem
static lfs_t lfs;
static struct lfs_config cfg;
static bool fs_mounted = false;

// Read a region in a block. Negative error codes are propagated to user.
static int block_device_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    uint32_t fs_start = XIP_BASE + FLASH_TARGET_OFFSET;
    memcpy(buffer, (unsigned char *)(fs_start + (block * c->block_size) + off), size);
    return 0;
}

// Program a region in a block. The block must have previously been erased.
// Negative error codes are propagated to user.
static int block_device_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t flash_offs = FLASH_TARGET_OFFSET + (block * c->block_size) + off;
    
    multicore_lockout_start_blocking();
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(flash_offs, buffer, size);
    restore_interrupts (ints);
    multicore_lockout_end_blocking();
    return 0;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined.
static int block_device_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t flash_offs = FLASH_TARGET_OFFSET + (block * c->block_size);
    
    multicore_lockout_start_blocking();
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_offs, c->block_size);
    restore_interrupts (ints);
    multicore_lockout_end_blocking();
    return 0;
}

// Sync the state of the underlying block device. Negative error codes
// are propagated to user.
static int block_device_sync(const struct lfs_config *c) {
    (void)c;
    return 0;
}

/**
 * Mount and initialization function called at power-on
 * @return 0: Success, negative value: Error
 */
int fstask_mount_and_init(void) {
    printf("Initializing LittleFS...\n");
    
    // Configuration of the filesystem
    cfg.read  = block_device_read;
    cfg.prog  = block_device_prog;
    cfg.erase = block_device_erase;
    cfg.sync  = block_device_sync;

    cfg.read_size = 1;
    cfg.prog_size = FLASH_PAGE_SIZE;
    cfg.block_size = FLASH_SECTOR_SIZE;
    cfg.block_count = 128;  // 512KB filesystem
    cfg.cache_size = FLASH_SECTOR_SIZE;
    cfg.lookahead_size = 16;
    cfg.block_cycles = 500;

    // Mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // Reformat if we can't mount the filesystem
    if (err) {
        printf("Formatting filesystem...\n");
        err = lfs_format(&lfs, &cfg);
        if (err < 0) {
            printf("Failed to format filesystem: %d\n", err);
            return err;
        }
        
        err = lfs_mount(&lfs, &cfg);
        if (err < 0) {
            printf("Failed to mount filesystem after format: %d\n", err);
            return err;
        }
    }
    
    fs_mounted = true;
    printf("LittleFS mounted successfully\n");
    return 0;
}

/**
 * Function to read specified file
 * @param filename Name of file to read
 * @param buffer Buffer to store read data
 * @param buffer_size Size of buffer
 * @return Number of bytes read, negative value on error
 */
int fstask_read_file(const char *filename, char *buffer, size_t buffer_size) {
    if (!fs_mounted) {
        printf("Filesystem not mounted\n");
        return -1;
    }
    
    if (!filename || !buffer || buffer_size == 0) {
        printf("Invalid parameters\n");
        return -1;
    }
    
    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY);
    if (err < 0) {
        printf("Failed to open file '%s' for reading: %d\n", filename, err);
        return err;
    }
    
    // Read the file content
    lfs_ssize_t bytes_read = lfs_file_read(&lfs, &file, buffer, buffer_size - 1);
    lfs_file_close(&lfs, &file);
    
    if (bytes_read < 0) {
        printf("Failed to read file '%s': %ld\n", filename, bytes_read);
        return (int)bytes_read;
    }
    
    // Null terminate the string
    buffer[bytes_read] = '\0';
    
    // printf("Read %ld bytes from file '%s'\n", bytes_read, filename);
    return (int)bytes_read;
}



/**
 * Function to list directory contents
 * @param callback Callback function to process file/directory information
 * @param user_data User data to pass to callback function
 * @return 0: Success, negative value: Error
 */
int fstask_list_directory(void (*callback)(const char* name, int type, lfs_size_t size, void* user_data), void* user_data) {
    if (!fs_mounted) {
        printf("Filesystem not mounted\n");
        return -1;
    }
    
    if (!callback) {
        printf("Invalid callback parameter\n");
        return -1;
    }
    
    lfs_dir_t dir;
    int err = lfs_dir_open(&lfs, &dir, "/");
    if (err < 0) {
        printf("Failed to open root directory: %d\n", err);
        return err;
    }
    
    struct lfs_info info;
    while (true) {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            printf("Failed to read directory: %d\n", res);
            lfs_dir_close(&lfs, &dir);
            return res;
        }
        
        if (res == 0) {
            break;  // End of directory
        }
        
        // Skip "." and ".." entries
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) {
            continue;
        }
        
        // Call the callback function with file information
        callback(info.name, info.type, info.size, user_data);
    }
    
    lfs_dir_close(&lfs, &dir);
    return 0;
}

/**
 * ファイルを削除する関数
 * @param filename 削除するファイル名
 * @return 0: 成功, 負の値: エラー
 */
int fstask_remove_file(const char *filename) {
    if (!fs_mounted) {
        printf("Filesystem not mounted\n");
        return -1;
    }
    
    if (!filename || strlen(filename) == 0) {
        printf("Invalid filename parameter\n");
        return -1;
    }
    
    // Check if file exists first
    struct lfs_info info;
    int err = lfs_stat(&lfs, filename, &info);
    if (err < 0) {
        printf("File '%s' not found: %d\n", filename, err);
        return err;
    }
    
    // Check if it's a regular file (not a directory)
    if (info.type != LFS_TYPE_REG) {
        printf("'%s' is not a regular file (type: %d)\n", filename, info.type);
        return -1;
    }
    
    // Remove the file
    err = lfs_remove(&lfs, filename);
    if (err < 0) {
        printf("Failed to remove file '%s': %d\n", filename, err);
        return err;
    }
    
    printf("Successfully removed file '%s'\n", filename);
    return 0;
}

/**
 * 指定されたファイルにバイナリデータを書き込む関数
 * @param filename 書き込むファイル名
 * @param data 書き込むデータ
 * @param data_size 書き込むデータのサイズ
 * @return 書き込んだバイト数、エラーの場合は負の値
 */
int fstask_write_file(const char *filename, const void *data, size_t data_size) {
    if (!fs_mounted) {
        printf("Filesystem not mounted\n");
        return -1;
    }
    
    if (!filename || !data || data_size == 0) {
        printf("Invalid parameters\n");
        return -1;
    }
    
    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err < 0) {
        printf("Failed to open file '%s' for writing: %d\n", filename, err);
        return err;
    }
    
    // Write the data
    lfs_ssize_t bytes_written = lfs_file_write(&lfs, &file, data, data_size);
    lfs_file_close(&lfs, &file);
    
    if (bytes_written < 0) {
        printf("Failed to write file '%s': %ld\n", filename, bytes_written);
        return (int)bytes_written;
    }
    
    printf("Successfully wrote %ld bytes to file '%s'\n", bytes_written, filename);
    return (int)bytes_written;
}

/**
 * サンプルファイルを読み取る関数
 * @param buffer 読み取ったデータを格納するバッファ
 * @param buffer_size バッファのサイズ
 * @return 読み取ったバイト数、エラーの場合は負の値
 */
int fstask_read_sample_file(char *buffer, size_t buffer_size) {
    return fstask_read_file(SAMPLE_FILENAME, buffer, buffer_size);
}

/**
 * サンプルファイルに書き込む関数
 * @param data 書き込むデータ
 * @param data_size 書き込むデータのサイズ
 * @return 書き込んだバイト数、エラーの場合は負の値
 */
int fstask_write_sample_file(const char *data, size_t data_size) {
    return fstask_write_file(SAMPLE_FILENAME, data, data_size);
}

/**
 * ファイルのサイズを取得する関数
 * @param filename サイズを取得するファイル名
 * @return ファイルサイズ（バイト）、エラーの場合は負の値
 */
lfs_ssize_t fstask_get_file_size(const char *filename) {
    if (!fs_mounted) {
        printf("Filesystem not mounted\n");
        return -1;
    }
    
    if (!filename) {
        printf("Invalid filename parameter\n");
        return -1;
    }
    
    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY);
    if (err < 0) {
        printf("Failed to open file '%s' for size check: %d\n", filename, err);
        return err;
    }
    
    // Get file size
    lfs_ssize_t size = lfs_file_size(&lfs, &file);
    lfs_file_close(&lfs, &file);
    
    if (size < 0) {
        printf("Failed to get size of file '%s': %ld\n", filename, size);
        return size;
    }
    
    printf("File '%s' size: %ld bytes\n", filename, size);
    return size;
}

/**
 * ファイルシステムをアンマウントする関数
 * @return 0: 成功, 負の値: エラー
 */
int fstask_unmount(void) {
    if (!fs_mounted) {
        return 0;  // Already unmounted
    }
    
    int err = lfs_unmount(&lfs);
    if (err < 0) {
        printf("Failed to unmount filesystem: %d\n", err);
        return err;
    }
    
    fs_mounted = false;
    printf("Filesystem unmounted successfully\n");
    return 0;
}