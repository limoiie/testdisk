/*
 * PhotoRec API - Context-Based File Recovery Library Interface
 * 
 * This header provides a context-based API for implementing custom interfaces
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
#define TESTDISK_O_ALL             0x00000020
#define TESTDISK_O_DIRECT          0x00000040

/* Disk unit types */
#define UNIT_SECTOR                1
#define UNIT_CHS                   2

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
 * FORWARD DECLARATIONS AND BASIC TYPES
 * ============================================================================ */

/**
 * @brief Linked list structure for internal use
 */
struct td_list_head
{
    struct td_list_head* next;
    struct td_list_head* prev;
};

#define TD_LIST_HEAD_INIT(name) { &(name), &(name) }

typedef struct
{
    unsigned long int cylinders;
    unsigned int heads_per_cylinder;
    unsigned int sectors_per_head;
    unsigned int bytes_per_sector; /* WARN: may be uninitialized */
} CHSgeometry_t;

/**
 * @brief Disk structure for device information
 */
typedef struct disk_struct disk_t;

struct disk_struct
{
#define DISKNAME_MAX	64
#define DISKDESCRIPTION_MAX	128
    char description_txt[DISKDESCRIPTION_MAX]; /**< Human-readable disk description */
    char description_short_txt[DISKDESCRIPTION_MAX];
    /**< Human-readable disk description */
    CHSgeometry_t geom; /* logical CHS */
    uint64_t disk_size; /**< Disk size in bytes */
    char* device; /**< Device path (e.g., /dev/sda) */
    char* model; /**< Model */
    char* serial_no; /**< Serial number */
    char* fw_rev; /**< Firmware revision */
    const char*(*description)(disk_t* disk); /**< Description function */
    const char*(*description_short)(disk_t* disk); /**< Short description function */
    int (*pread)(disk_t* disk, void* buf, const unsigned int count,
                 const uint64_t offset); /**< Read function */
    int (*pwrite)(disk_t* disk, const void* buf, const unsigned int count,
                  const uint64_t offset); /**< Write function */
    int (*sync)(disk_t* disk); /**< Sync function */
    void (*clean)(disk_t* disk); /**< Clean function */
    const struct arch_fnct_struct* arch; /**< Partition table architecture */
    const struct arch_fnct_struct* arch_autodetected;
    /**< Autodetected partition table architecture */
    void* data; /**< Data */
    uint64_t disk_real_size; /**< Real disk size */
    uint64_t user_max; /**< User max */
    uint64_t native_max; /**< Native max */
    uint64_t dco; /**< DCO */
    uint64_t offset;
    /* offset to first sector, may be modified in the futur to handle broken raid */
    void* rbuffer; /**< Read buffer */
    void* wbuffer; /**< Write buffer */
    unsigned int rbuffer_size; /**< Read buffer size */
    unsigned int wbuffer_size; /**< Write buffer size */
    int write_used; /**< Write used */
    int autodetect; /**< Autodetect */
    int access_mode; /**< Access mode */
    int unit; /**< Unit */
    unsigned int sector_size; /**< Sector size */
};

/**
 * @brief Disk list structure for iteration
 */
typedef struct list_disk_struct list_disk_t;

struct list_disk_struct
{
    disk_t* disk; /**< Pointer to disk structure */
    list_disk_t* prev; /**< Previous disk in list */
    list_disk_t* next; /**< Next disk in list */
};

/* Partition types and related enums */
typedef enum upart_type
{
    UP_UNK = 0, UP_APFS, UP_BEOS, UP_BTRFS, UP_CRAMFS, UP_EXFAT, UP_EXT2, UP_EXT3,
    UP_EXT4, UP_EXTENDED, UP_FAT12, UP_FAT16, UP_FAT32, UP_FATX, UP_FREEBSD, UP_F2FS,
    UP_GFS2, UP_HFS, UP_HFSP, UP_HFSX, UP_HPFS, UP_ISO, UP_JFS, UP_LINSWAP, UP_LINSWAP2,
    UP_LINSWAP_8K, UP_LINSWAP2_8K, UP_LINSWAP2_8KBE, UP_LUKS, UP_LVM, UP_LVM2,
    UP_MD, UP_MD1, UP_NETWARE, UP_NTFS, UP_OPENBSD, UP_OS2MB, UP_ReFS, UP_RFS,
    UP_RFS2, UP_RFS3, UP_RFS4, UP_SUN, UP_SYSV4, UP_UFS, UP_UFS2, UP_UFS_LE,
    UP_UFS2_LE, UP_VMFS, UP_WBFS, UP_XFS, UP_XFS2, UP_XFS3, UP_XFS4, UP_XFS5, UP_ZFS
} upart_type_t;

typedef enum status_type
{
    STATUS_DELETED, STATUS_PRIM, STATUS_PRIM_BOOT, STATUS_LOG, STATUS_EXT,
    STATUS_EXT_IN_EXT
} status_type_t;

typedef enum errcode_type
{
    BAD_NOERR, BAD_SS, BAD_ES, BAD_SH, BAD_EH, BAD_EBS, BAD_RS, BAD_SC, BAD_EC, BAD_SCOUNT
} errcode_type_t;

/* EFI GUID structure */
typedef struct efi_guid_s efi_guid_t;

struct efi_guid_s
{
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t clock_seq_hi_and_reserved;
    uint8_t clock_seq_low;
    uint8_t node[6];
};

/**
 * @brief Partition structure for filesystem information
 */
typedef struct partition_struct partition_t;

struct partition_struct
{
    char fsname[128]; /**< Filesystem name */
    char partname[128]; /**< Partition name */
    char info[128]; /**< Additional information */
    uint64_t part_offset; /**< Partition offset in bytes */
    uint64_t part_size; /**< Partition size in bytes */
    uint64_t sborg_offset; /**< Superblock origin offset */
    uint64_t sb_offset; /**< Superblock offset */
    unsigned int sb_size; /**< Superblock size */
    unsigned int blocksize; /**< Block size */
    efi_guid_t part_uuid; /**< Partition UUID (GPT) */
    efi_guid_t part_type_gpt; /**< Partition type GUID (GPT) */
    unsigned int part_type_humax; /**< Partition type (Humax) */
    unsigned int part_type_i386; /**< Partition type (x86) */
    unsigned int part_type_mac; /**< Partition type (Mac) */
    unsigned int part_type_sun; /**< Partition type (Sun) */
    unsigned int part_type_xbox; /**< Partition type (Xbox) */
    upart_type_t upart_type; /**< Unified partition type */
    status_type_t status; /**< Partition status */
    unsigned int order; /**< Partition order */
    errcode_type_t errcode; /**< Error code */
    const struct arch_fnct_struct* arch; /**< Architecture functions */
};

/**
 * @brief Partition list structure for iteration
 */
typedef struct list_part_struct list_part_t;

struct list_part_struct
{
    partition_t* part; /**< Pointer to partition structure */
    list_part_t* prev; /**< Previous partition in list */
    list_part_t* next; /**< Next partition in list */
    int to_be_removed; /**< Removal flag */
};

/**
 * @brief File allocation list structure
 */
typedef struct alloc_list_s alloc_list_t;

struct alloc_list_s
{
    struct td_list_head list; /**< Linked list node */
    uint64_t start; /**< Start offset */
    uint64_t end; /**< End offset */
    unsigned int data; /**< Additional data flags */
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
    char filename[2048]; /**< Output filename */
    alloc_list_t location; /**< File location information */
    file_stat_t* file_stat; /**< Associated file statistics */
    FILE* handle; /**< File handle for writing */
    time_t time; /**< File timestamp */
    uint64_t file_size; /**< Current file size */
    const char* extension; /**< File extension */
    uint64_t min_filesize; /**< Minimum expected file size */
    uint64_t offset_ok; /**< Last known good offset */
    uint64_t offset_error; /**< First error offset */
    uint64_t extra; /**< Extra bytes between offsets */
    uint64_t calculated_file_size; /**< Calculated file size */
    data_check_t (*data_check)(const unsigned char* buffer,
                               unsigned int buffer_size,
                               file_recovery_t* file_recovery);
    /**< Data validation function */
    void (*file_check)(file_recovery_t* file_recovery); /**< File validation function */
    void (*file_rename)(file_recovery_t* file_recovery); /**< File renaming function */
    uint64_t checkpoint_offset; /**< Checkpoint offset for resume */
    int checkpoint_status; /**< Checkpoint status */
    unsigned int blocksize; /**< Block size for recovery */
    unsigned int flags; /**< Recovery flags */
    unsigned int data_check_tmp; /**< Temporary data check value */
};

/* Forward declaration for arch functions */
typedef struct arch_fnct_struct arch_fnct_t;

struct arch_fnct_struct
{
    const char* part_name;
    const char* part_name_option;
    const char* msg_part_type;
    list_part_t*(*read_part)(disk_t* disk, const int verbose, const int saveheader);
    int (*write_part)(disk_t* disk, const list_part_t* list_part, const int ro,
                      const int verbose);
    list_part_t*(*init_part_order)(const disk_t* disk, list_part_t* list_part);
    /* geometry must be initialized to 0,0,0 in get_geometry_from_mbr()*/
    int (*get_geometry_from_mbr)(const unsigned char* buffer, const int verbose,
                                 CHSgeometry_t* geometry);
    int (*check_part)(disk_t* disk, const int verbose, partition_t* partition,
                      const int saveheader);
    int (*write_MBR_code)(disk_t* disk);
    void (*set_prev_status)(const disk_t* disk, partition_t* partition);
    void (*set_next_status)(const disk_t* disk, partition_t* partition);
    int (*test_structure)(const list_part_t* list_part);
    unsigned int (*get_part_type)(const partition_t* partition);
    int (*set_part_type)(partition_t* partition, unsigned int part_type);
    void (*init_structure)(const disk_t* disk, list_part_t* list_part, const int verbose);
    int (*erase_list_part)(disk_t* disk);
    const char*(*get_partition_typename)(const partition_t* partition);
    int (*is_part_known)(const partition_t* partition);
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

/**
 * @brief PhotoRec CLI context - main API structure
 *
 * This structure encapsulates all PhotoRec state and configuration,
 * providing a context-based API for file recovery operations.
 */
typedef struct ph_cli_context ph_cli_context_t;

struct ph_cli_context
{
    struct ph_options options; /**< Recovery options configuration */
    struct ph_param params; /**< Recovery parameters and state */
    int mode; /**< Disk access mode flags */
    const arch_fnct_t** list_arch; /**< Available partition architectures */
    list_disk_t* list_disk; /**< List of available disks */
    list_part_t* list_part; /**< List of partitions on current disk */
    alloc_data_t list_search_space; /**< Search space for recovery */
    int log_opened; /**< Log file opened */
    int log_errno; /**< Log file error number */
};

/* ============================================================================
 * MAIN API FUNCTIONS - Context Management
 * ============================================================================ */

/**
 * @brief Initialize PhotoRec context
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @param recup_dir Recovery directory
 * @param device Device path (e.g., "/dev/sda" or image file path)
 * @param log_mode Log mode. 0: no log, 1: info, 2: debug
 * @param log_file Log file
 * @return Initialized PhotoRec context, or NULL on failure
 *
 * This function initializes a new PhotoRec context with default settings,
 * discovers available disks, and prepares the system for file recovery.
 */
ph_cli_context_t* init_photorec(int argc, char* argv[], char* recup_dir, char* device,
                                int log_mode, const char* log_file);

/**
 * @brief Run PhotoRec file recovery
 * @param ctx PhotoRec context
 * @return 0 on success, non-zero on error
 *
 * Executes the main PhotoRec recovery process using the current
 * context configuration. This function will run until completion
 * or user interruption.
 */
int run_photorec(ph_cli_context_t* ctx);

/**
 * @brief Clean up PhotoRec context
 * @param ctx PhotoRec context to clean up
 *
 * Frees all resources associated with the PhotoRec context including
 * disk lists, partition lists, and allocated memory.
 */
void finish_photorec(ph_cli_context_t* ctx);

/**
 * @brief Abort PhotoRec file recovery
 * @param ctx PhotoRec context
 */
void abort_photorec(ph_cli_context_t* ctx);

/* ============================================================================
 * CONFIGURATION FUNCTIONS - Disk and Partition Selection
 * ============================================================================ */

/**
 * @brief Change the target disk for recovery
 * @param ctx PhotoRec context
 * @param device Device path (e.g., "/dev/sda" or image file path)
 * @return Selected disk structure, or NULL if not found
 *
 * Selects the specified disk and initializes the partition list.
 * The device can be a physical disk or an image file.
 */
disk_t* change_disk(ph_cli_context_t* ctx, const char* device);

/**
 * @brief Change the partition table architecture
 * @param ctx PhotoRec context
 * @param part_name_option Partition name option (can be NULL for auto-detect)
 * @return Selected architecture structure
 *
 * Manually sets or auto-detects the partition table architecture
 * (GPT, MBR, Mac, Sun, etc.) and updates the disk unit accordingly.
 */
const arch_fnct_t* change_arch(const ph_cli_context_t* ctx, char* part_name_option);

/**
 * @brief Change the target partition for recovery
 * @param ctx PhotoRec context
 * @param order Partition order number
 * @param mode_ext2 Enable EXT2/3/4 optimizations (0=no, 1=yes)
 * @param carve_free_space_only Only scan unallocated space (0=no, 1=yes)
 * @return Selected partition structure, or NULL if not found
 *
 * Selects a specific partition by its order number for recovery.
 */
partition_t* change_part(ph_cli_context_t* ctx, int order, int mode_ext2,
                         int carve_free_space_only);

/* ============================================================================
 * CONFIGURATION FUNCTIONS - Recovery Options
 * ============================================================================ */

/**
 * @brief Change recovery options
 * @param ctx PhotoRec context
 * @param paranoid Paranoid mode level (0-2)
 * @param keep_corrupted_file Keep corrupted files (0=no, 1=yes)
 * @param mode_ext2 Enable EXT2/3/4 optimizations (0=no, 1=yes)
 * @param expert Enable expert mode (0=no, 1=yes)
 * @param lowmem Use low memory mode (0=no, 1=yes)
 * @param verbose Verbose output (0=no, 1=yes)
 * 
 * Configures various recovery options that affect how PhotoRec
 * performs file recovery and validation.
 */
void change_options(ph_cli_context_t* ctx, int paranoid,
                    int keep_corrupted_file, int mode_ext2, int expert,
                    int lowmem, int verbose);

/**
 * @brief Change recovery status/phase
 * @param ctx PhotoRec context
 * @param status Recovery status to set
 * 
 * Sets the initial recovery phase. Useful for resuming recovery
 * from a specific phase or skipping certain phases.
 */
void change_status(ph_cli_context_t* ctx, photorec_status_t status);

/**
 * @brief Change block size for recovery
 * @param ctx PhotoRec context
 * @param blocksize Block size in bytes (0 for auto-detect)
 * @return 0 on success, non-zero on error
 * 
 * Sets the block size used for recovery operations. Setting to 0
 * enables automatic block size detection.
 */
int change_blocksize(ph_cli_context_t* ctx, unsigned int blocksize);

/* ============================================================================
 * CONFIGURATION FUNCTIONS - File Type Selection
 * ============================================================================ */

/**
 * @brief Enable or disable all file types
 * @param ctx PhotoRec context
 * @param all_enable_status 1 to enable all, 0 to disable all
 * @return 0 on success, non-zero on error
 * 
 * Bulk enable or disable all supported file types for recovery.
 */
int change_all_fileopt(const ph_cli_context_t* ctx, const int all_enable_status);

/**
 * @brief Configure specific file types for recovery
 * @param ctx PhotoRec context
 * @param exts_to_enable Array of file extensions to enable
 * @param exts_to_enable_count Number of extensions to enable
 * @param exts_to_disable Array of file extensions to disable
 * @param exts_to_disable_count Number of extensions to disable
 * @return 0 on success, non-zero on error
 * 
 * Selectively enable or disable specific file types by extension.
 * This allows fine-grained control over which file types to recover.
 */
int change_fileopt(const ph_cli_context_t* ctx,
                   char** exts_to_enable, int exts_to_enable_count,
                   char** exts_to_disable, int exts_to_disable_count);

/* ============================================================================
 * CONFIGURATION FUNCTIONS - Advanced Options
 * ============================================================================ */

/**
 * @brief Change disk geometry settings
 * @param ctx PhotoRec context
 * @param cylinders Number of cylinders
 * @param heads_per_cylinder Number of heads per cylinder
 * @param sectors_per_head Number of sectors per head
 * @param sector_size Sector size in bytes
 * 
 * Manually sets disk geometry parameters. This is typically only
 * needed for very old disks or specialized image files.
 */
void change_geometry(ph_cli_context_t* ctx, unsigned int cylinders,
                     unsigned int heads_per_cylinder,
                     unsigned int sectors_per_head,
                     unsigned int sector_size);

/**
 * @brief Configure EXT2/3/4 group mode
 * @param ctx PhotoRec context
 * @param group_number EXT2/3/4 group number
 * 
 * Sets specific EXT2/3/4 group for optimized recovery.
 */
void change_ext2_mode(ph_cli_context_t* ctx, int group_number);

/**
 * @brief Configure EXT2/3/4 inode mode
 * @param ctx PhotoRec context
 * @param inode_number EXT2/3/4 inode number
 * 
 * Sets specific EXT2/3/4 inode for optimized recovery.
 */
void change_ext2_inode(ph_cli_context_t* ctx, int inode_number);

/**
 * @brief General configuration interface
 * @param ctx PhotoRec context
 * @param cmd Configuration command string
 * @return 0 on success, non-zero on error
 * 
 * Provides a generic interface for sending configuration commands
 * to PhotoRec. This allows access to advanced features not covered
 * by the specific configuration functions.
 */
int config_photorec(ph_cli_context_t* ctx, char* cmd);

/* ============================================================================
 * UTILITY FUNCTIONS AND GLOBALS
 * ============================================================================ */

/**
 * @brief Array of all supported file types
 */
extern const file_enable_t array_file_enable[];

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

/**
 * @brief Get human-readable name for recovery status
 * @param status Recovery status code
 * @return Status name string
 */
const char* status_to_name(photorec_status_t status);

/* ============================================================================
 * LOWER-LEVEL UTILITY FUNCTIONS (used internally)
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
 * @brief Insert a new disk into the list of disks
 * @param list_disk List of disks
 * @param disk Disk to insert
 * @return List of disks
 */
list_disk_t* insert_new_disk(list_disk_t* list_disk, disk_t* disk);

/**
* @brief Test file/device availability and create disk structure
* @param device_path Path to device or image file
* @param verbose Verbosity level
* @param testdisk_mode Access mode flags
* @return Disk structure or NULL on failure
*/
disk_t* file_test_availability(const char* device_path, int verbose, int testdisk_mode);

/**
 * @brief Free disk list and associated resources
 * @param list_disk Disk list to free
 */
void delete_list_disk(list_disk_t* list_disk);

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

/**
 * @brief Automatically set disk unit type based on partition table
 * @param disk Target disk structure
 */
void autoset_unit(disk_t* disk);

/**
 * @brief Automatically detect partition table architecture
 * @param disk Target disk structure
 * @param arch Fallback architecture (can be NULL)
 */
void autodetect_arch(disk_t* disk, const arch_fnct_t* arch);

#ifdef __cplusplus
}
#endif

#endif /* _PHOTOREC_API_H */
