# PhotoRec API Documentation

## Overview

PhotoRec is a powerful file recovery utility that can recover lost files from hard disks, CD-ROMs, and digital camera memory. This API documentation provides a comprehensive guide for integrating PhotoRec's file recovery capabilities into custom applications.

## Key Features

- **File Signature-Based Recovery**: Identifies files by their headers rather than filesystem metadata
- **300+ File Types Supported**: JPEG, Office documents, multimedia files, archives, and more
- **Cross-Platform**: Works on Windows, macOS, Linux, and other Unix-like systems
- **Raw Device Support**: Can recover from damaged or formatted partitions
- **Session Management**: Resume interrupted recovery sessions
- **Configurable Recovery**: Control which file types to recover and recovery parameters

## Architecture Overview

PhotoRec uses a multi-phase recovery process:

1. **STATUS_FIND_OFFSET**: Find optimal block alignment
2. **STATUS_EXT2_ON/OFF**: Main recovery with/without filesystem optimization
3. **STATUS_EXT2_ON_BF/OFF_BF**: Brute force recovery phases
4. **STATUS_QUIT**: Recovery completed

## Data Structures

### Core Structures

```c++
// Recovery parameters
struct ph_param {
    disk_t *disk;                    // Target disk
    partition_t *partition;          // Target partition  
    char *recup_dir;                // Output directory
    unsigned int blocksize;          // Block size for recovery
    photorec_status_t status;        // Current recovery phase
    unsigned int file_nbr;           // Files recovered count
    // ... other fields
};

// Recovery options
struct ph_options {
    int paranoid;                    // Paranoid mode (0-2)
    int keep_corrupted_file;         // Keep partial files
    unsigned int mode_ext2;          // EXT2/3/4 optimizations
    file_enable_t *list_file_format; // File type configuration
    // ... other fields
};

// Search space management
typedef struct alloc_data_struct {
    struct td_list_head list;       // Linked list node
    uint64_t start;                 // Start offset
    uint64_t end;                   // End offset
    file_stat_t *file_stat;         // File statistics
    unsigned int data;              // Data type flag
} alloc_data_t;
```

## Basic Usage

### Simple Recovery Example

```c++
#include "photorec_api.h"

int simple_recovery(const char *device_path, const char *output_dir) {
    struct ph_param params;
    struct ph_options options;
    alloc_data_t list_search_space = {0};
    list_disk_t *list_disk = NULL;
    disk_t *disk = NULL;
    list_part_t *list_part = NULL;
    int result = 0;

    // Initialize parameters
    memset(&params, 0, sizeof(params));
    memset(&options, 0, sizeof(options));

    // Set default options
    options.paranoid = 1;
    options.keep_corrupted_file = 0;
    options.mode_ext2 = 0;
    options.expert = 0;
    options.lowmem = 0;
    options.verbose = 0;
    options.list_file_format = array_file_enable;
    reset_array_file_enable(options.list_file_format);

    // Set recovery directory
    params.recup_dir = strdup(output_dir);
    params.carve_free_space_only = 0; // Scan entire partition

    // Open disk/image
    disk = file_test_availability(device_path, 0, TESTDISK_O_RDONLY);
    if (!disk) {
        fprintf(stderr, "Cannot open %s\n", device_path);
        result = -1;
        goto cleanup;
    }

    params.disk = disk;

    // Get partitions
    list_part = init_list_part(disk, &options);
    if (!list_part) {
        fprintf(stderr, "No partitions found\n");
        result = -1;
        goto cleanup;
    }

    // Use first partition (or whole disk)
    params.partition = list_part->part;

    // Initialize search space
    init_search_space(&list_search_space, disk, params.partition);

    // Start recovery
    result = photorec(&params, &options, &list_search_space);

cleanup:
    if (params.recup_dir) free(params.recup_dir);
    free_search_space(&list_search_space);
    part_free_list(list_part);
    if (disk) delete_list_disk(list_disk);

    return result;
}
```

### Image Recovery with File Type Selection

```c++
#include "photorec_api.h"

int recover_images_only(const char *device_path, const char *output_dir) {
    struct ph_param params;
    struct ph_options options;
    alloc_data_t list_search_space = {0};
    disk_t *disk = NULL;
    list_part_t *list_part = NULL;
    file_enable_t *file_enable;
    int result = 0;

    // Initialize structures
    memset(&params, 0, sizeof(params));
    memset(&options, 0, sizeof(options));

    // Configure options for image recovery
    options.paranoid = 1;
    options.keep_corrupted_file = 0;
    options.mode_ext2 = 0;
    options.expert = 0;
    options.lowmem = 0;
    options.verbose = 1;
    options.list_file_format = array_file_enable;

    // Disable all file types first
    reset_array_file_enable(options.list_file_format);
    for (file_enable = options.list_file_format; 
         file_enable->file_hint != NULL; 
         file_enable++) {
        file_enable->enable = 0;
    }

    // Enable only image formats
    for (file_enable = options.list_file_format; 
         file_enable->file_hint != NULL; 
         file_enable++) {
        const char *ext = file_enable->file_hint->extension;
        if (strcmp(ext, "jpg") == 0 || strcmp(ext, "png") == 0 ||
            strcmp(ext, "gif") == 0 || strcmp(ext, "bmp") == 0 ||
            strcmp(ext, "tiff") == 0 || strcmp(ext, "raw") == 0) {
            file_enable->enable = 1;
        }
    }

    // Set up recovery parameters
    disk = file_test_availability(device_path, 1, TESTDISK_O_RDONLY);
    if (!disk) return -1;

    params.disk = disk;
    params.recup_dir = strdup(output_dir);
    params.carve_free_space_only = 0;

    list_part = init_list_part(disk, &options);
    if (!list_part) {
        free(params.recup_dir);
        return -1;
    }

    params.partition = list_part->part;
    init_search_space(&list_search_space, disk, params.partition);

    // Start recovery
    result = photorec(&params, &options, &list_search_space);

    // Cleanup
    part_free_list(list_part);
    free(params.recup_dir);

    return result;
}
```

## Advanced Usage

### Recovery with Progress Monitoring and Statistics

```c++
#include "photorec_api.h"
#include <signal.h>

typedef struct {
    unsigned int files_recovered;
    unsigned int total_size_mb;
    time_t start_time;
    time_t estimated_finish;
    photorec_status_t current_phase;
    double progress_percentage;
} recovery_stats_t;

static int recovery_interrupted = 0;

void signal_handler(int sig) {
    recovery_interrupted = 1;
    need_to_stop = 1;
}

int comprehensive_recovery(const char *device_path, const char *output_dir, 
                          recovery_stats_t *stats) {
    struct ph_param params;
    struct ph_options options;
    alloc_data_t list_search_space = {0};
    disk_t *disk = NULL;
    int result = 0;

    // Initialize statistics
    memset(stats, 0, sizeof(*stats));
    stats->start_time = time(NULL);

    // Initialize structures
    memset(&params, 0, sizeof(params));
    memset(&options, 0, sizeof(options));

    // Configure for comprehensive recovery
    options.paranoid = 2;                    // Maximum verification
    options.keep_corrupted_file = 1;         // Keep partial files
    options.mode_ext2 = 1;                   // Use filesystem optimizations
    options.expert = 1;                      // Expert mode
    options.verbose = 1;                     // Verbose output
    options.list_file_format = array_file_enable;

    reset_array_file_enable(options.list_file_format);
    file_options_load(options.list_file_format);

    // Set up recovery parameters
    disk = file_test_availability(device_path, 1, TESTDISK_O_RDONLY);
    if (!disk) return -1;

    params.disk = disk;
    params.recup_dir = strdup(output_dir);
    params.carve_free_space_only = 0;        // Scan entire partition

    list_part_t *list_part = init_list_part(disk, &options);
    if (!list_part) {
        free(params.recup_dir);
        return -1;
    }

    params.partition = list_part->part;
    init_search_space(&list_search_space, disk, params.partition);

    // Reset and start recovery
    params_reset(&params, &options);
    params.dir_num = photorec_mkdir(params.recup_dir, 1);

    // Main recovery loop with progress tracking
    while (params.status != STATUS_QUIT && !recovery_interrupted) {
        pstatus_t status;

        // Update statistics
        stats->files_recovered = params.file_nbr;
        stats->current_phase = params.status;

        // Execute recovery phase
        switch (params.status) {
            case STATUS_FIND_OFFSET:
                status = photorec_find_blocksize(&params, &options, &list_search_space);
                break;

            case STATUS_EXT2_ON:
            case STATUS_EXT2_OFF:
                status = photorec_aux(&params, &options, &list_search_space);
                break;

            case STATUS_EXT2_ON_BF:
            case STATUS_EXT2_OFF_BF:
                status = photorec_bf(&params, &options, &list_search_space);
                break;

            case STATUS_UNFORMAT:
                status = fat_unformat(&params, &options, &list_search_space);
                break;

            default:
                status = PSTATUS_OK;
                break;
        }

        // Handle errors
        if (status == PSTATUS_EACCES) {
            fprintf(stderr, "Access denied during recovery\n");
            result = -1;
            break;
        } else if (status == PSTATUS_ENOSPC) {
            fprintf(stderr, "No space left on destination\n");
            result = -2;
            break;
        } else if (status == PSTATUS_STOP) {
            printf("Recovery stopped by user\n");
            break;
        }

        // Save session periodically
        session_save(&list_search_space, &params, &options);

        // Advance to next phase
        status_inc(&params, &options);

        // Update progress (simplified calculation)
        if (params.partition && params.partition->part_size > 0) {
            stats->progress_percentage = 
                (double)params.offset / params.partition->part_size * 100.0;
        }
    }

    // Final statistics
    stats->files_recovered = params.file_nbr;

    // Cleanup
    part_free_list(list_part);
    free(params.recup_dir);

    return result;
}
```

### Batch Processing with Resume Capability

```c++
#include "photorec_api.h"

typedef struct recovery_job {
    char device_path[512];
    char output_dir[512];
    int completed;
    int file_count;
    struct recovery_job *next;
} recovery_job_t;

int batch_recovery(recovery_job_t *jobs) {
    recovery_job_t *current = jobs;
    int total_jobs = 0, completed_jobs = 0;

    // Count total jobs
    for (recovery_job_t *job = jobs; job; job = job->next) {
        total_jobs++;
    }

    printf("Starting batch recovery of %d jobs\n", total_jobs);

    while (current) {
        printf("Processing job %d/%d: %s -> %s\n", 
               completed_jobs + 1, total_jobs, 
               current->device_path, current->output_dir);

        // Check if session exists for resuming
        char *resume_device = NULL;
        char *resume_cmd = NULL;
        alloc_data_t resume_search_space = {0};

        if (session_load(&resume_device, &resume_cmd, &resume_search_space) == 0) {
            printf("Resuming previous session for %s\n", current->device_path);
            // Resume recovery with existing search space
        }

        struct ph_param params;
        struct ph_options options;
        alloc_data_t list_search_space = {0};

        // Initialize for this job
        memset(&params, 0, sizeof(params));
        memset(&options, 0, sizeof(options));

        // Configure options
        options.paranoid = 1;
        options.keep_corrupted_file = 0;
        options.mode_ext2 = 1;
        options.expert = 0;
        options.lowmem = 0;
        options.verbose = 1;
        options.list_file_format = array_file_enable;

        reset_array_file_enable(options.list_file_format);
        file_options_load(options.list_file_format);

        // Set up this job
        disk_t *disk = file_test_availability(current->device_path, 1, 
                                              TESTDISK_O_RDONLY);
        if (!disk) {
            printf("Failed to open %s, skipping\n", current->device_path);
            current = current->next;
            continue;
        }

        params.disk = disk;
        params.recup_dir = strdup(current->output_dir);
        params.carve_free_space_only = 0;

        list_part_t *list_part = init_list_part(disk, &options);
        if (!list_part) {
            printf("No partitions found on %s, skipping\n", current->device_path);
            free(params.recup_dir);
            current = current->next;
            continue;
        }

        params.partition = list_part->part;
        init_search_space(&list_search_space, disk, params.partition);

        // Execute recovery
        int result = photorec(&params, &options, &list_search_space);

        if (result == 0) {
            current->completed = 1;
            current->file_count = params.file_nbr;
            completed_jobs++;
            printf("Job completed: %d files recovered\n", params.file_nbr);
        } else {
            printf("Job failed with error %d\n", result);
        }

        // Cleanup
        part_free_list(list_part);
        free(params.recup_dir);

        // Save batch progress
        // (Implementation specific - could save to file)

        current = current->next;
    }

    printf("Batch processing complete: %d/%d jobs successful\n", 
           completed_jobs, total_jobs);

    return completed_jobs == total_jobs ? 0 : -1;
}
```

### Error Handling and Robust Recovery

```c++
int robust_recovery(const char *device_path, const char *output_dir) {
    struct ph_param params;
    struct ph_options options;
    alloc_data_t list_search_space = {0};
    int result = 0;

    // Initialize with error checking
    if (!device_path || !output_dir) {
        fprintf(stderr, "Invalid parameters\n");
        return -1;
    }

    // Test device availability first
    disk_t *disk = file_test_availability(device_path, 1, TESTDISK_O_RDONLY);
    if (!disk) {
        fprintf(stderr, "Cannot access device %s\n", device_path);
        return -2;
    }

    // Check output directory permissions
    if (access(output_dir, W_OK) != 0) {
        fprintf(stderr, "Cannot write to output directory %s\n", output_dir);
        return -3;
    }

    // Initialize structures
    memset(&params, 0, sizeof(params));
    memset(&options, 0, sizeof(options));

    // Configure with safe defaults
    options.paranoid = 1;
    options.keep_corrupted_file = 0;
    options.mode_ext2 = 0;
    options.expert = 0;
    options.lowmem = 0;
    options.verbose = 1;
    options.list_file_format = array_file_enable;

    reset_array_file_enable(options.list_file_format);

    params.disk = disk;
    params.recup_dir = strdup(output_dir);
    params.carve_free_space_only = 0;

    // Get partitions with error handling
    list_part_t *list_part = init_list_part(disk, &options);
    if (!list_part) {
        fprintf(stderr, "No readable partitions found on %s\n", device_path);
        result = -4;
        goto cleanup;
    }

    // Select partition (use first available)
    params.partition = list_part->part;
    if (!params.partition) {
        fprintf(stderr, "No valid partition found\n");
        result = -5;
        goto cleanup;
    }

    // Initialize search space
    init_search_space(&list_search_space, disk, params.partition);

    // Start recovery with error handling
    params_reset(&params, &options);

    // Create output directory structure
    params.dir_num = photorec_mkdir(params.recup_dir, 1);
    if (params.dir_num == 0) {
        fprintf(stderr, "Failed to create output directory\n");
        result = -6;
        goto cleanup;
    }

    // Main recovery loop with comprehensive error handling
    while (params.status != STATUS_QUIT) {
        pstatus_t status = PSTATUS_OK;

        // Execute phase with try-catch equivalent
        switch (params.status) {
            case STATUS_FIND_OFFSET:
                printf("Phase: Finding optimal block size...\n");
                status = photorec_find_blocksize(&params, &options, &list_search_space);
                break;

            case STATUS_EXT2_ON:
                printf("Phase: Main recovery with filesystem optimization...\n");
                status = photorec_aux(&params, &options, &list_search_space);
                break;

            case STATUS_EXT2_OFF:
                printf("Phase: Main recovery without optimization...\n");
                status = photorec_aux(&params, &options, &list_search_space);
                break;

            case STATUS_EXT2_ON_BF:
                printf("Phase: Brute force recovery with optimization...\n");
                status = photorec_bf(&params, &options, &list_search_space);
                break;

            case STATUS_EXT2_OFF_BF:
                printf("Phase: Brute force recovery...\n");
                status = photorec_bf(&params, &options, &list_search_space);
                break;

            case STATUS_UNFORMAT:
                printf("Phase: FAT unformat recovery...\n");
                status = fat_unformat(&params, &options, &list_search_space);
                break;

            default:
                printf("Unknown phase, moving to next...\n");
                break;
        }

        // Handle phase-specific errors
        switch (status) {
            case PSTATUS_OK:
                printf("Phase completed successfully\n");
                break;

            case PSTATUS_EACCES:
                fprintf(stderr, "Access denied - check permissions\n");
                result = -7;
                goto cleanup;

            case PSTATUS_ENOSPC:
                fprintf(stderr, "No space left on destination device\n");
                result = -8;
                goto cleanup;

            case PSTATUS_STOP:
                printf("Recovery stopped by user request\n");
                goto cleanup;

            default:
                fprintf(stderr, "Unknown error in phase %s\n", 
                        status_to_name(params.status));
                break;
        }

        // Save session after each phase
        if (session_save(&list_search_space, &params, &options) != 0) {
            fprintf(stderr, "Warning: Could not save session\n");
        }

        // Report progress
        printf("Files recovered so far: %d\n", params.file_nbr);

        // Advance to next phase
        status_inc(&params, &options);
    }

    printf("Recovery completed successfully\n");
    printf("Total files recovered: %d\n", params.file_nbr);
    printf("Output directory: %s\n", params.recup_dir);

cleanup:
    if (list_part) part_free_list(list_part);
    if (params.recup_dir) free(params.recup_dir);

    return result;
}
```

## API Reference

### Data Structures

#### struct ph_param
Recovery parameters and state tracking.

**Fields:**
- `disk_t *disk` - Target disk or image file
- `partition_t *partition` - Target partition for recovery
- `char *recup_dir` - Output directory for recovered files
- `unsigned int blocksize` - Block size for recovery operations
- `photorec_status_t status` - Current recovery phase
- `unsigned int file_nbr` - Number of files recovered
- `uint64_t offset` - Current offset in recovery process

#### struct ph_options
Recovery configuration options.

**Fields:**
- `int paranoid` - Paranoid mode level (0=off, 1=normal, 2=maximum)
- `int keep_corrupted_file` - Whether to keep partially recovered files
- `unsigned int mode_ext2` - Enable EXT2/3/4 filesystem optimizations
- `unsigned int expert` - Enable expert mode features
- `unsigned int lowmem` - Use memory-efficient algorithms
- `int verbose` - Verbosity level for output
- `file_enable_t *list_file_format` - Array of file type configurations

#### alloc_data_t
Search space management structure.

**Fields:**
- `struct td_list_head list` - Linked list node for chaining
- `uint64_t start` - Start offset of this block
- `uint64_t end` - End offset of this block
- `file_stat_t *file_stat` - Associated file statistics
- `unsigned int data` - Additional data flags

### Core Functions

#### int photorec(struct ph_param *params, const struct ph_options *options, alloc_data_t *list_search_space)
Main recovery function that orchestrates the entire recovery process.

**Parameters:**
- `params` - Recovery parameters and state
- `options` - Recovery configuration options
- `list_search_space` - Search space definition

**Returns:** 0 on success, non-zero on error

#### pstatus_t photorec_aux(struct ph_param *params, const struct ph_options *options, alloc_data_t *list_search_space)
Standard recovery algorithm for intact files.

**Returns:** Process status (PSTATUS_OK, PSTATUS_STOP, etc.)

#### pstatus_t photorec_bf(struct ph_param *params, const struct ph_options *options, alloc_data_t *list_search_space)
Brute force recovery for fragmented files.

**Returns:** Process status

#### pstatus_t photorec_find_blocksize(struct ph_param *params, const struct ph_options *options, alloc_data_t *list_search_space)
Analyzes the disk to find the optimal block size for recovery.

**Returns:** Process status

### Disk Management

#### disk_t *file_test_availability(const char *device_path, int verbose, int testdisk_mode)
Tests if a device or image file is accessible and creates a disk structure.

**Parameters:**
- `device_path` - Path to device (/dev/sda) or image file
- `verbose` - Verbosity level
- `testdisk_mode` - Access mode flags (TESTDISK_O_RDONLY, etc.)

**Returns:** Disk structure or NULL on failure

#### list_part_t *init_list_part(disk_t *disk, const struct ph_options *options)
Scans a disk and creates a list of available partitions.

**Parameters:**
- `disk` - Target disk
- `options` - Recovery options (can be NULL)

**Returns:** Linked list of partitions

### File Type Configuration

#### void reset_array_file_enable(file_enable_t *files_enable)
Resets all file type settings to their default enabled/disabled state.

#### int file_options_load(file_enable_t *files_enable)
Loads file type configuration from photorec.cfg file.

**Returns:** 0 on success, -1 on failure

#### int file_options_save(file_enable_t *files_enable)
Saves current file type configuration to photorec.cfg file.

**Returns:** 0 on success, -1 on failure

### Session Management

#### int session_save(const alloc_data_t *list_search_space, const struct ph_param *params, const struct ph_options *options)
Saves current recovery state for later resumption.

**Returns:** 0 on success, -1 on failure

#### int session_load(char **cmd_device, char **current_cmd, alloc_data_t *list_search_space)
Loads previously saved recovery session.

**Parameters:**
- `cmd_device` - Output parameter for device path
- `current_cmd` - Output parameter for command line
- `list_search_space` - Search space to restore

**Returns:** 0 on success, -1 on failure

## Building and Integration

### Building the PhotoRec Libraries

#### Prerequisites
Ensure you have the standard build tools and dependencies:
```bash
# On Ubuntu/Debian
sudo apt-get install build-essential autoconf automake libtool pkg-config
sudo apt-get install libncurses5-dev uuid-dev libext2fs-dev libntfs-3g-dev

# On CentOS/RHEL
sudo yum install gcc gcc-c++ autoconf automake libtool pkgconfig
sudo yum install ncurses-devel libuuid-devel e2fsprogs-devel ntfs-3g-devel
```

#### Building Static Libraries
```bash
# Configure the build system
./configure

# Build both static library variants
make library

# Or build them separately:
make library-static    # Builds libphotorec_static.a (internal use)
make library-shared    # Builds libphotorec.a (for distribution)
```

#### Install Libraries and Headers
```bash
# Install libraries and headers to system directories
make library-install

# This installs:
# - libphotorec.a to /usr/local/lib/
# - photorec_api.h to /usr/local/include/
```

#### Library Files Generated
- **libphotorec.a** - Static library for distribution
- **libphotorec_static.a** - Internal static library variant
- **photorec_api.h** - API header file

### Required Headers
```c++
#include "photorec_api.h"
```

### Linking Your Application

#### Using Installed Library
```bash
# If library is installed system-wide
gcc -o myapp myapp.c -lphotorec -lncurses -luuid

# Specify additional library paths if needed
gcc -o myapp myapp.c -L/usr/local/lib -lphotorec -lncurses -luuid
```

#### Using Local Library Build
```bash
# Link directly to built library file
gcc -o myapp myapp.c ./src/libphotorec.a -lncurses -luuid

# Or use the static variant
gcc -o myapp myapp.c ./src/libphotorec_static.a -lncurses -luuid
```

### Dependencies
- libncurses (for interactive features)
- libuuid (for GUID support)
- libext2fs (for EXT2/3/4 optimization, optional)
- libntfs/libntfs-3g (for NTFS optimization, optional)

### Error Handling
Always check return values and handle errors appropriately. The API uses standard POSIX error codes where applicable.

### Memory Management
- Always free allocated resources (disk lists, partition lists, recovery directories)
- Use the provided cleanup functions (part_free_list, delete_list_disk)
- Be careful with string parameters - some functions expect caller to manage memory

### Thread Safety
The PhotoRec API is not thread-safe. If using in a multi-threaded application, ensure proper synchronization around API calls.

## Best Practices

1. **Always validate inputs** - Check device paths, output directories, and parameters
2. **Handle interruption gracefully** - Monitor the `need_to_stop` global variable
3. **Save sessions regularly** - Use session management for long-running recoveries
4. **Configure file types appropriately** - Don't recover unnecessary file types to save time
5. **Monitor disk space** - Ensure adequate space in the output directory
6. **Use appropriate paranoid levels** - Higher levels provide better validation but slower recovery
7. **Test with small images first** - Validate your implementation before processing large disks

## Conclusion

The PhotoRec API provides powerful file recovery capabilities that can be integrated into custom applications. Whether building a simple recovery tool or a complex data recovery suite, these APIs offer the flexibility and functionality needed to recover lost files effectively.

For additional examples and advanced usage patterns, refer to the PhotoRec source code and the existing CLI and GUI implementations. 