/*
 * PhotoRec API - Comprehensive File Recovery Library Interface
 * 
 * This header provides a complete API for implementing custom interfaces
 * to PhotoRec's file recovery functionality while preserving all core
 * recovery capabilities.
 * 
 * Copyright (C) 1998-2024 Christophe GRENIER <grenier@cgsecurity.org>
 * 
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _PHOTOREC_API_H
#define _PHOTOREC_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* ============================================================================
 * CONSTANTS AND LIMITS
 * ============================================================================ */

#define MAX_FILES_PER_DIR           500
#define DEFAULT_RECUP_DIR          "recup_dir"
#define PHOTOREC_MAX_FILE_SIZE     (((uint64_t)1<<41)-1)
#define PHOTOREC_MAX_BLOCKSIZE     (32*1024*1024)
#define PH_INVALID_OFFSET          0xffffffffffffffff

/* Access mode flags */
#define TESTDISK_O_RDONLY          0x00000001
#define TESTDISK_O_READAHEAD_32K   0x00000010

/* ============================================================================
 * ENUMS AND STATUS TYPES
 * ============================================================================ */

/**
 * @brief PhotoRec recovery status phases
 */
typedef enum
{
    STATUS_FIND_OFFSET, /**< Finding optimal block alignment */
    STATUS_UNFORMAT, /**< FAT unformat recovery */
    STATUS_EXT2_ON, /**< Main recovery with filesystem optimization */
    STATUS_EXT2_ON_BF, /**< Brute force with filesystem optimization */
    STATUS_EXT2_OFF, /**< Main recovery without filesystem optimization */
    STATUS_EXT2_OFF_BF, /**< Brute force without filesystem optimization */
    STATUS_EXT2_ON_SAVE_EVERYTHING, /**< Save everything mode with optimization */
    STATUS_EXT2_OFF_SAVE_EVERYTHING, /**< Save everything mode without optimization */
    STATUS_QUIT /**< Recovery completed */
} photorec_status_t;

/**
 * @brief Process status codes
 */
typedef enum
{
    PSTATUS_OK = 0, /**< Normal operation */
    PSTATUS_STOP = 1, /**< User requested stop */
    PSTATUS_EACCES = 2, /**< Access denied */
    PSTATUS_ENOSPC = 3 /**< No space left on device */
} pstatus_t;

/**
 * @brief File recovery status codes
 */
typedef enum
{
    PFSTATUS_BAD = 0, /**< File recovery failed */
    PFSTATUS_OK = 1, /**< File recovered successfully */
    PFSTATUS_OK_TRUNCATED = 2 /**< File recovered but truncated */
} pfstatus_t;

/**
 * @brief Data validation results
 */
typedef enum
{
    DC_SCAN = 0, /**< Continue scanning */
    DC_CONTINUE = 1, /**< Continue with current file */
    DC_STOP = 2, /**< Stop processing current file */
    DC_ERROR = 3 /**< Error occurred */
} data_check_t;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

/**
 * @brief Linked list structure for internal use
 */
struct td_list_head
{
    struct td_list_head* next;
    struct td_list_head* prev;
};

/**
 * @brief Disk structure for device information
 */
typedef struct disk_struct disk_t;
struct disk_struct
{
    char description_txt[1024];    /**< Human-readable disk description */
    char *device;                  /**< Device path (e.g., /dev/sda) */
    uint64_t disk_size;            /**< Disk size in bytes */
    unsigned int sector_size;      /**< Sector size in bytes */
    /* Note: This is a simplified view - full structure contains many more fields */
};

/* Additional required types */
typedef enum upart_type {
    UP_UNK=0, UP_APFS, UP_BEOS, UP_BTRFS, UP_CRAMFS, UP_EXFAT, UP_EXT2, UP_EXT3, UP_EXT4,
    UP_EXTENDED, UP_FAT12, UP_FAT16, UP_FAT32, UP_FATX, UP_FREEBSD, UP_F2FS, UP_GFS2,
    UP_HFS, UP_HFSP, UP_HFSX, UP_HPFS, UP_ISO, UP_JFS, UP_LINSWAP, UP_LINSWAP2,
    UP_LINSWAP_8K, UP_LINSWAP2_8K, UP_LINSWAP2_8KBE, UP_LUKS, UP_LVM, UP_LVM2,
    UP_MD, UP_MD1, UP_NETWARE, UP_NTFS, UP_OPENBSD, UP_OS2MB, UP_ReFS, UP_RFS,
    UP_RFS2, UP_RFS3, UP_RFS4, UP_SUN, UP_SYSV4, UP_UFS, UP_UFS2, UP_UFS_LE,
    UP_UFS2_LE, UP_VMFS, UP_WBFS, UP_XFS, UP_XFS2, UP_XFS3, UP_XFS4, UP_XFS5, UP_ZFS
} upart_type_t;

typedef enum status_type { 
    STATUS_DELETED, STATUS_PRIM, STATUS_PRIM_BOOT, STATUS_LOG, STATUS_EXT, STATUS_EXT_IN_EXT
} status_type_t;

typedef enum errcode_type {
    BAD_NOERR, BAD_SS, BAD_ES, BAD_SH, BAD_EH, BAD_EBS, BAD_RS, BAD_SC, BAD_EC, BAD_SCOUNT
} errcode_type_t;

/* EFI GUID structure */
struct efi_guid_s {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t  clock_seq_hi_and_reserved;
    uint8_t  clock_seq_low;
    uint8_t  node[6];
};
typedef struct efi_guid_s efi_guid_t;

/* Forward declaration for arch functions */
typedef struct arch_fnct_struct arch_fnct_t;

/**
 * @brief Partition structure for filesystem information
 */
typedef struct partition_struct partition_t;
struct partition_struct
{
    char          fsname[128];        /**< Filesystem name */
    char          partname[128];      /**< Partition name */
    char          info[128];          /**< Additional information */
    uint64_t      part_offset;        /**< Partition offset in bytes */
    uint64_t      part_size;          /**< Partition size in bytes */
    uint64_t      sborg_offset;       /**< Superblock origin offset */
    uint64_t      sb_offset;          /**< Superblock offset */
    unsigned int  sb_size;            /**< Superblock size */
    unsigned int  blocksize;          /**< Block size */
    efi_guid_t    part_uuid;          /**< Partition UUID (GPT) */
    efi_guid_t    part_type_gpt;      /**< Partition type GUID (GPT) */
    unsigned int  part_type_humax;    /**< Partition type (Humax) */
    unsigned int  part_type_i386;     /**< Partition type (x86) */
    unsigned int  part_type_mac;      /**< Partition type (Mac) */
    unsigned int  part_type_sun;      /**< Partition type (Sun) */
    unsigned int  part_type_xbox;     /**< Partition type (Xbox) */
    upart_type_t  upart_type;         /**< Unified partition type */
    status_type_t status;             /**< Partition status */
    unsigned int  order;              /**< Partition order */
    errcode_type_t errcode;           /**< Error code */
    const arch_fnct_t *arch;          /**< Architecture functions */
};

/**
 * @brief Partition list structure for iteration
 */
typedef struct list_part_struct list_part_t;
struct list_part_struct
{
    partition_t *part;          /**< Pointer to partition structure */
    list_part_t *prev;          /**< Previous partition in list */
    list_part_t *next;          /**< Next partition in list */
    int to_be_removed;          /**< Removal flag */
};

/**
 * @brief File allocation list structure
 */
typedef struct alloc_list_s alloc_list_t;
struct alloc_list_s
{
    struct td_list_head list;   /**< Linked list node */
    uint64_t start;             /**< Start offset */
    uint64_t end;               /**< End offset */
    unsigned int data;          /**< Additional data flags */
};

typedef struct file_hint_struct file_hint_t;
typedef struct file_stat_struct file_stat_t;
typedef struct file_enable_struct file_enable_t;

/**
 * @brief File recovery state structure
 */
typedef struct file_recovery_struct file_recovery_t;
struct file_recovery_struct
{
    char filename[2048];                    /**< Output filename */
    alloc_list_t location;                  /**< File location information */
    file_stat_t *file_stat;                 /**< Associated file statistics */
    FILE *handle;                           /**< File handle for writing */
    time_t time;                            /**< File timestamp */
    uint64_t file_size;                     /**< Current file size */
    const char *extension;                  /**< File extension */
    uint64_t min_filesize;                  /**< Minimum expected file size */
    uint64_t offset_ok;                     /**< Last known good offset */
    uint64_t offset_error;                  /**< First error offset */
    uint64_t extra;                         /**< Extra bytes between offsets */
    uint64_t calculated_file_size;          /**< Calculated file size */
    data_check_t (*data_check)(const unsigned char*buffer, const unsigned int buffer_size, file_recovery_t *file_recovery); /**< Data validation function */
    void (*file_check)(file_recovery_t *file_recovery);     /**< File validation function */
    void (*file_rename)(file_recovery_t *file_recovery);    /**< File renaming function */
    uint64_t checkpoint_offset;             /**< Checkpoint offset for resume */
    int checkpoint_status;                  /**< Checkpoint status */
    unsigned int blocksize;                 /**< Block size for recovery */
    unsigned int flags;                     /**< Recovery flags */
    unsigned int data_check_tmp;            /**< Temporary data check value */
};

/**
 * @brief Disk list structure for iteration
 */
typedef struct list_disk_struct list_disk_t;
struct list_disk_struct
{
    disk_t* disk;           /**< Pointer to disk structure */
    list_disk_t* prev;      /**< Previous disk in list */
    list_disk_t* next;      /**< Next disk in list */
};

/* ============================================================================
 * CORE DATA STRUCTURES
 * ============================================================================ */

/**
 * @brief PhotoRec recovery options configuration
 */
struct ph_options
{
    int paranoid; /**< Paranoid mode level (0-2) */
    int keep_corrupted_file; /**< Keep partially recovered files */
    unsigned int mode_ext2; /**< Enable EXT2/3/4 optimizations */
    unsigned int expert; /**< Enable expert mode features */
    unsigned int lowmem; /**< Use low memory algorithms */
    int verbose; /**< Verbosity level */
    file_enable_t* list_file_format; /**< Array of file type settings */
};

/**
 * @brief PhotoRec recovery parameters and state
 */
struct ph_param
{
    char* cmd_device; /**< Target device path */
    char* cmd_run; /**< Command line to execute */
    disk_t* disk; /**< Target disk structure */
    partition_t* partition; /**< Target partition */
    unsigned int carve_free_space_only; /**< Only scan unallocated space */
    unsigned int blocksize; /**< Block size for recovery */
    unsigned int pass; /**< Current recovery pass */
    photorec_status_t status; /**< Current recovery phase */
    time_t real_start_time; /**< Recovery start timestamp */
    char* recup_dir; /**< Recovery output directory */
    unsigned int dir_num; /**< Current output directory number */
    unsigned int file_nbr; /**< Number of files recovered */
    file_stat_t* file_stats; /**< Recovery statistics by type */
    uint64_t offset; /**< Current recovery offset */
};

/**
 * @brief File type hint definition
 */
struct file_hint_struct
{
    const char* extension; /**< File extension */
    const char* description; /**< Human-readable description */
    const uint64_t max_filesize; /**< Maximum expected file size */
    const int recover; /**< Whether to recover this type */
    const unsigned int enable_by_default; /**< Default enabled state */
    void (*register_header_check)(file_stat_t* file_stat); /**< Registration function */
};

/**
 * @brief File type enable/disable configuration
 */
struct file_enable_struct
{
    const file_hint_t* file_hint; /**< File type definition */
    unsigned int enable; /**< Whether type is enabled */
};

/**
 * @brief File recovery statistics
 */
struct file_stat_struct
{
    unsigned int not_recovered; /**< Count of failed recoveries */
    unsigned int recovered; /**< Count of successful recoveries */
    const file_hint_t* file_hint; /**< Associated file type */
};

/**
 * @brief Search space allocation block
 */
typedef struct alloc_data_struct
{
    struct td_list_head list; /**< Linked list node */
    uint64_t start; /**< Start offset */
    uint64_t end; /**< End offset */
    file_stat_t* file_stat; /**< Associated file statistics */
    unsigned int data; /**< Additional data flags */
} alloc_data_t;

/* ============================================================================
 * DISK AND PARTITION MANAGEMENT
 * ============================================================================ */

/**
 * @brief Scan system for available disks and images
 * @param list_disk Existing disk list to append to (can be NULL)
 * @param verbose Verbosity level for output
 * @param testdisk_mode Access mode flags
 * @return Linked list of discovered disks
 */
list_disk_t* hd_parse(list_disk_t* list_disk, int verbose, int testdisk_mode);

/**
 * @brief Update geometry information for all disks
 * @param list_disk List of disks to update
 * @param verbose Verbosity level
 */
void hd_update_all_geometry(list_disk_t* list_disk, int verbose);

/**
 * @brief Create disk cache wrapper for improved performance
 * @param disk Original disk structure
 * @param testdisk_mode Access mode flags
 * @return Cached disk structure
 */
disk_t* new_diskcache(disk_t* disk, int testdisk_mode);

/**
 * @brief Free disk list and associated resources
 * @param list_disk Disk list to free
 */
void delete_list_disk(list_disk_t* list_disk);

/**
 * @brief Test file/device availability and create disk structure
 * @param device_path Path to device or image file
 * @param verbose Verbosity level
 * @param testdisk_mode Access mode flags
 * @return Disk structure or NULL on failure
 */
disk_t* file_test_availability(const char* device_path, int verbose, int testdisk_mode);

/**
 * @brief Initialize partition list for a disk
 * @param disk Target disk
 * @param options Recovery options (can be NULL)
 * @return Linked list of partitions
 */
list_part_t* init_list_part(disk_t* disk, const struct ph_options* options);

/**
 * @brief Free partition list
 * @param list_part Partition list to free
 */
void part_free_list(list_part_t* list_part);

/* ============================================================================
 * RECOVERY PARAMETER MANAGEMENT
 * ============================================================================ */

/**
 * @brief Reset recovery parameters to initial state
 * @param params Parameter structure to reset
 * @param options Recovery options
 */
void params_reset(struct ph_param* params, const struct ph_options* options);

/**
 * @brief Advance to next recovery phase
 * @param params Recovery parameters
 * @param options Recovery options
 */
void status_inc(struct ph_param* params, const struct ph_options* options);

/**
 * @brief Get human-readable name for recovery status
 * @param status Recovery status code
 * @return Status name string
 */
const char* status_to_name(photorec_status_t status);

/* ============================================================================
 * SEARCH SPACE MANAGEMENT
 * ============================================================================ */

/**
 * @brief Initialize search space for a partition
 * @param list_search_space Search space list to initialize
 * @param disk Target disk
 * @param partition Target partition
 */
void init_search_space(alloc_data_t* list_search_space, disk_t* disk,
                       partition_t* partition);

/**
 * @brief Update block size based on search space analysis
 * @param list_search_space Search space
 * @param blocksize Current block size
 * @param offset Current offset
 * @param force_blocksize Force specific block size
 * @return Updated block size
 */
unsigned int update_blocksize(alloc_data_t* list_search_space, unsigned int blocksize,
                              const uint64_t offset, const int force_blocksize);

/**
 * @brief Remove used space from search space
 * @param list_search_space Search space
 * @param start Start offset to remove
 * @param end End offset to remove
 */
void remove_used_space(alloc_data_t* list_search_space, uint64_t start, uint64_t end);

/**
 * @brief Get next sector in search space
 * @param list_search_space Search space
 * @param current_search_space Current search space element
 * @param offset Current offset (updated)
 * @param blocksize Block size
 * @return 0 if next sector found, non-zero if end reached
 */
int get_next_sector(alloc_data_t* list_search_space, alloc_data_t** current_search_space,
                    uint64_t* offset, unsigned int blocksize);

/**
 * @brief Display search space information
 * @param list_search_space Search space
 * @param current_search_space Current element
 * @param sector_size Sector size
 * @param keep_corrupted_file Whether corrupted files are kept
 * @param verbose Verbosity level
 */
void info_list_search_space(const alloc_data_t* list_search_space,
                            const alloc_data_t* current_search_space,
                            const unsigned int sector_size,
                            const int keep_corrupted_file,
                            const int verbose);

/* ============================================================================
 * FILE TYPE CONFIGURATION
 * ============================================================================ */

/**
 * @brief Array of all supported file types
 */
extern file_enable_t array_file_enable[];

/**
 * @brief Reset file type settings to defaults
 * @param files_enable File enable array
 */
void reset_array_file_enable(file_enable_t* files_enable);

/**
 * @brief Load file type configuration from file
 * @param files_enable File enable array
 * @return 0 on success, -1 on failure
 */
int file_options_load(file_enable_t* files_enable);

/**
 * @brief Save file type configuration to file
 * @param files_enable File enable array
 * @return 0 on success, -1 on failure
 */
int file_options_save(file_enable_t* files_enable);

/* ============================================================================
 * CORE RECOVERY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Main recovery function
 * @param params Recovery parameters
 * @param options Recovery options
 * @param list_search_space Search space
 * @return 0 on success, non-zero on error
 */
int photorec(struct ph_param* params, const struct ph_options* options,
             alloc_data_t* list_search_space);

/**
 * @brief Standard recovery algorithm
 * @param params Recovery parameters
 * @param options Recovery options
 * @param list_search_space Search space
 * @return Process status
 */
pstatus_t photorec_aux(struct ph_param* params, const struct ph_options* options,
                       alloc_data_t* list_search_space);

/**
 * @brief Brute force recovery for fragmented files
 * @param params Recovery parameters
 * @param options Recovery options
 * @param list_search_space Search space
 * @return Process status
 */
pstatus_t photorec_bf(struct ph_param* params, const struct ph_options* options,
                      alloc_data_t* list_search_space);

/**
 * @brief Find optimal block size for recovery
 * @param params Recovery parameters
 * @param options Recovery options
 * @param list_search_space Search space
 * @return Process status
 */
pstatus_t photorec_find_blocksize(struct ph_param* params,
                                  const struct ph_options* options,
                                  alloc_data_t* list_search_space);

/**
 * @brief FAT unformat recovery
 * @param params Recovery parameters
 * @param options Recovery options
 * @param list_search_space Search space
 * @return Process status
 */
pstatus_t fat_unformat(struct ph_param* params, const struct ph_options* options,
                       alloc_data_t* list_search_space);

/* ============================================================================
 * FILE RECOVERY SUPPORT FUNCTIONS
 * ============================================================================ */

/**
 * @brief Finalize file recovery (standard mode)
 * @param file_recovery File recovery state
 * @param params Recovery parameters
 * @param paranoid Paranoid verification level
 * @param list_search_space Search space
 * @return File recovery status
 */
pfstatus_t file_finish2(file_recovery_t* file_recovery, struct ph_param* params,
                        const int paranoid, alloc_data_t* list_search_space);

/**
 * @brief Handle aborted file recovery
 * @param file_recovery File recovery state
 * @param params Recovery parameters
 * @param list_search_space Search space
 */
void file_recovery_aborted(file_recovery_t* file_recovery, struct ph_param* params,
                           alloc_data_t* list_search_space);

/**
 * @brief Set filename for recovered file
 * @param file_recovery File recovery state
 * @param params Recovery parameters
 */
void set_filename(file_recovery_t* file_recovery, struct ph_param* params);

/* ============================================================================
 * SESSION MANAGEMENT
 * ============================================================================ */

/**
 * @brief Save recovery session state
 * @param list_search_space Current search space
 * @param params Recovery parameters
 * @param options Recovery options
 * @return 0 on success, -1 on failure
 */
int session_save(const alloc_data_t* list_search_space, const struct ph_param* params,
                 const struct ph_options* options);

/**
 * @brief Load previous recovery session
 * @param cmd_device Device path (output)
 * @param current_cmd Command line (output)
 * @param list_search_space Search space to restore
 * @return 0 on success, -1 on failure
 */
int session_load(char** cmd_device, char** current_cmd, alloc_data_t* list_search_space);

/**
 * @brief Perform regular session save during recovery
 * @param list_search_space Current search space
 * @param params Recovery parameters
 * @param options Recovery options
 * @param current_time Current timestamp
 * @return Next checkpoint time
 */
time_t regular_session_save(alloc_data_t* list_search_space, struct ph_param* params,
                            const struct ph_options* options, time_t current_time);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Create recovery output directory
 * @param recup_dir Base directory path
 * @param initial_dir_num Starting directory number
 * @return Next directory number
 */
unsigned int photorec_mkdir(const char* recup_dir, const unsigned int initial_dir_num);

/**
 * @brief Update recovery statistics
 * @param file_stats Statistics to update
 * @param list_search_space Current search space
 */
void update_stats(file_stat_t* file_stats, alloc_data_t* list_search_space);

/**
 * @brief Find block size from search space analysis
 * @param list_search_space Search space
 * @param sector_size Disk sector size
 * @param start_offset Starting offset (output)
 * @return Detected block size
 */
unsigned int find_blocksize(alloc_data_t* list_search_space, unsigned int sector_size,
                            uint64_t* start_offset);

/**
 * @brief Global stop flag for user interruption
 */
extern int need_to_stop;

#ifdef __cplusplus
}
#endif

#endif /* _PHOTOREC_API_H */
