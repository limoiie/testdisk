# PhotoRec API Documentation

## Overview

PhotoRec is a powerful file recovery utility that can recover lost files from hard disks, CD-ROMs, and digital camera memory. This API documentation provides a comprehensive guide for integrating PhotoRec's file recovery capabilities into custom applications using a modern context-based approach.

## Key Features

- **File Signature-Based Recovery**: Identifies files by their headers rather than filesystem metadata
- **300+ File Types Supported**: JPEG, Office documents, multimedia files, archives, and more
- **Cross-Platform**: Works on Windows, macOS, Linux, and other Unix-like systems
- **Raw Device Support**: Can recover from damaged or formatted partitions
- **Session Management**: Resume interrupted recovery sessions
- **Context-Based API**: Simple, stateful interface for easy integration
- **Configurable Recovery**: Control which file types to recover and recovery parameters

## Architecture Overview

The PhotoRec API uses a context-based design centered around the `ph_cli_context_t` structure, which encapsulates all recovery state and configuration. The recovery process follows these phases:

1. **STATUS_FIND_OFFSET**: Find optimal block alignment
2. **STATUS_EXT2_ON/OFF**: Main recovery with/without filesystem optimization
3. **STATUS_EXT2_ON_BF/OFF_BF**: Brute force recovery phases
4. **STATUS_UNFORMAT**: FAT unformat recovery (when applicable)
5. **STATUS_QUIT**: Recovery completed

## Core Data Structures

### Main Context Structure

```c
// Main PhotoRec context - encapsulates all state
typedef struct ph_cli_context {
    struct ph_options options;        // Recovery options configuration
    struct ph_param params;           // Recovery parameters and state
    int mode;                        // Disk access mode flags
    const arch_fnct_t** list_arch;   // Available partition architectures
    list_disk_t* list_disk;          // List of available disks
    list_part_t* list_part;          // List of partitions on current disk
    alloc_data_t list_search_space;  // Search space for recovery
} ph_cli_context_t;
```

### Supporting Structures

```c
// Recovery options
struct ph_options {
    int paranoid;                    // Paranoid mode (0-2)
    int keep_corrupted_file;         // Keep partial files
    unsigned int mode_ext2;          // EXT2/3/4 optimizations
    unsigned int expert;             // Expert mode features
    unsigned int lowmem;             // Low memory mode
    int verbose;                     // Verbosity level
    file_enable_t* list_file_format; // File type configuration
};

// Recovery parameters and state
struct ph_param {
    char* cmd_device;                // Target device path
    char* cmd_run;                   // Command line to execute
    disk_t* disk;                    // Target disk structure
    partition_t* partition;          // Target partition
    unsigned int carve_free_space_only; // Only scan unallocated space
    unsigned int blocksize;          // Block size for recovery
    unsigned int pass;               // Current recovery pass
    photorec_status_t status;        // Current recovery phase
    time_t real_start_time;          // Recovery start timestamp
    char* recup_dir;                 // Recovery output directory
    unsigned int dir_num;            // Current output directory number
    unsigned int file_nbr;           // Number of files recovered
    file_stat_t* file_stats;         // Recovery statistics by type
    uint64_t offset;                 // Current recovery offset
};

// Search space management
typedef struct alloc_data_struct {
    struct td_list_head list;        // Linked list node
    uint64_t start;                  // Start offset
    uint64_t end;                    // End offset
    file_stat_t* file_stat;          // File statistics
    unsigned int data;               // Data type flag
} alloc_data_t;
```

## Basic Usage

### Simple Recovery Example

```c
#include "photorec_api.h"

int simple_recovery(const char* output_dir) {
    char* argv[] = {"photorec_api", NULL};
    ph_cli_context_t* ctx;
    int result = 0;

    // Initialize PhotoRec context
    ctx = init_photorec(1, argv);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize PhotoRec\n");
        return -1;
    }

    // Select first available disk
    if (ctx->list_disk && ctx->list_disk->disk) {
        change_disk(ctx, ctx->list_disk->disk->device);
    } else {
        fprintf(stderr, "No disks found\n");
        result = -1;
        goto cleanup;
    }

    // Select first partition or whole disk
    if (ctx->list_part && ctx->list_part->part) {
        change_part(ctx, ctx->list_part->part->order);
    }

    // Configure basic options
    change_options(ctx, 1, 0, 0, 0, 0, 1); // paranoid=1, verbose=1

    // Set output directory
    if (ctx->params.recup_dir) free(ctx->params.recup_dir);
    ctx->params.recup_dir = strdup(output_dir);

    // Run recovery
    result = run_photorec(ctx);

cleanup:
    finish_photorec(ctx);
    return result;
}
```

### Image Recovery with File Type Selection

```c
#include "photorec_api.h"

int recover_images_only(const char* device_path, const char* output_dir) {
    char* argv[] = {"photorec_api", NULL};
    ph_cli_context_t* ctx;
    int result = 0;

    // Initialize PhotoRec context
    ctx = init_photorec(1, argv);
    if (!ctx) return -1;

    // Select specific device
    if (!change_disk(ctx, device_path)) {
        fprintf(stderr, "Cannot access device %s\n", device_path);
        result = -1;
        goto cleanup;
    }

    // Configure for image recovery only
    change_options(ctx, 1, 0, 0, 0, 0, 1); // paranoid=1, verbose=1

    // Disable all file types first
    change_all_fileopt(ctx, 0);

    // Enable only image formats
    char* image_exts[] = {"jpg", "png", "gif", "bmp", "tiff", "raw"};
    change_fileopt(ctx, image_exts, 6, NULL, 0);

    // Set output directory
    if (ctx->params.recup_dir) free(ctx->params.recup_dir);
    ctx->params.recup_dir = strdup(output_dir);

    // Select first partition
    if (ctx->list_part && ctx->list_part->part) {
        change_part(ctx, ctx->list_part->part->order);
    }

    // Run recovery
    result = run_photorec(ctx);

cleanup:
    finish_photorec(ctx);
    return result;
}
```

### Advanced Recovery with Custom Configuration

```c
#include "photorec_api.h"

int advanced_recovery(const char* device_path, const char* output_dir,
                     int partition_order, int paranoid_level) {
    char* argv[] = {"photorec_api", NULL};
    ph_cli_context_t* ctx;
    int result = 0;

    // Initialize PhotoRec context
    ctx = init_photorec(1, argv);
    if (!ctx) return -1;

    // Select device
    if (!change_disk(ctx, device_path)) {
        fprintf(stderr, "Cannot access device %s\n", device_path);
        result = -1;
        goto cleanup;
    }

    // Auto-detect or manually set partition architecture
    change_arch(ctx, NULL); // NULL for auto-detect

    // Select specific partition
    if (!change_part(ctx, partition_order)) {
        fprintf(stderr, "Cannot find partition %d\n", partition_order);
        result = -1;
        goto cleanup;
    }

    // Configure advanced options
    change_options(ctx, 
                   paranoid_level,  // paranoid mode (0-2)
                   1,              // keep corrupted files
                   1,              // enable EXT2/3/4 optimizations
                   1,              // expert mode
                   0,              // normal memory usage
                   1);             // verbose output

    // Configure to scan only free space (faster)
    change_carve_space(ctx, 1);

    // Set custom block size (0 for auto-detect)
    change_blocksize(ctx, 0);

    // Set output directory
    if (ctx->params.recup_dir) free(ctx->params.recup_dir);
    ctx->params.recup_dir = strdup(output_dir);

    // Set initial recovery phase
    change_status(ctx, STATUS_FIND_OFFSET);

    // Run recovery
    result = run_photorec(ctx);

    printf("Recovery completed: %d files recovered\n", ctx->params.file_nbr);

cleanup:
    finish_photorec(ctx);
    return result;
}
```

### Batch Processing Multiple Devices

```c
#include "photorec_api.h"

typedef struct recovery_job {
    char device_path[512];
    char output_dir[512];
    int completed;
    int file_count;
} recovery_job_t;

int batch_recovery(recovery_job_t* jobs, int job_count) {
    int successful_jobs = 0;

    for (int i = 0; i < job_count; i++) {
        char* argv[] = {"photorec_api", NULL};
        ph_cli_context_t* ctx;

        printf("Processing job %d/%d: %s -> %s\n", 
               i + 1, job_count, jobs[i].device_path, jobs[i].output_dir);

        // Initialize context for this job
        ctx = init_photorec(1, argv);
        if (!ctx) {
            printf("Failed to initialize job %d\n", i + 1);
            continue;
        }

        // Configure and run recovery
        if (change_disk(ctx, jobs[i].device_path)) {
            if (ctx->list_part && ctx->list_part->part) {
                change_part(ctx, ctx->list_part->part->order);
            }

            change_options(ctx, 1, 0, 1, 0, 0, 1); // Basic configuration

            if (ctx->params.recup_dir) free(ctx->params.recup_dir);
            ctx->params.recup_dir = strdup(jobs[i].output_dir);

            if (run_photorec(ctx) == 0) {
                jobs[i].completed = 1;
                jobs[i].file_count = ctx->params.file_nbr;
                successful_jobs++;
                printf("Job %d completed: %d files recovered\n", 
                       i + 1, jobs[i].file_count);
            } else {
                printf("Job %d failed\n", i + 1);
            }
        } else {
            printf("Job %d: Cannot access device %s\n", i + 1, jobs[i].device_path);
        }

        finish_photorec(ctx);
    }

    printf("Batch processing complete: %d/%d jobs successful\n", 
           successful_jobs, job_count);

    return successful_jobs == job_count ? 0 : -1;
}
```

## API Reference

### Context Management Functions

#### ph_cli_context_t* init_photorec(int argc, char* argv[])
Initializes a new PhotoRec context with default settings and discovers available disks.

**Parameters:**
- `argc` - Command line argument count (typically 1)
- `argv` - Command line arguments (typically just program name)

**Returns:** Initialized PhotoRec context, or NULL on failure

**Description:** Creates a new PhotoRec context, sets up default options, discovers available disks, and prepares the system for file recovery operations.

#### void finish_photorec(ph_cli_context_t* ctx)
Cleans up PhotoRec context and frees all associated resources.

**Parameters:**
- `ctx` - PhotoRec context to clean up

**Description:** Frees all resources including disk lists, partition lists, recovery directory strings, and the context structure itself.

#### int run_photorec(ph_cli_context_t* ctx)
Executes the main PhotoRec recovery process.

#### void abort_photorec(ph_cli_context_t* ctx)
Abort the main PhotoRec recovery process.

**Parameters:**
- `ctx` - Configured PhotoRec context

**Returns:** 0 on success, non-zero on error

**Description:** Runs the complete PhotoRec recovery process through all necessary phases until completion or user interruption.

### Disk and Partition Selection

#### disk_t* change_disk(ph_cli_context_t* ctx, const char* device)
Selects a disk for recovery and initializes its partition list.

**Parameters:**
- `ctx` - PhotoRec context
- `device` - Device path (e.g., "/dev/sda") or image file path

**Returns:** Selected disk structure, or NULL if not found

#### const arch_fnct_t* change_arch(const ph_cli_context_t* ctx, char* part_name_option)
Sets or auto-detects the partition table architecture.

**Parameters:**
- `ctx` - PhotoRec context
- `part_name_option` - Partition name option (NULL for auto-detect)

**Returns:** Selected architecture structure

#### partition_t* change_part(ph_cli_context_t* ctx, const int order)
Selects a specific partition by order number.

**Parameters:**
- `ctx` - PhotoRec context
- `order` - Partition order number

**Returns:** Selected partition structure, or NULL if not found

### Recovery Configuration

#### void change_options(ph_cli_context_t* ctx, int paranoid, int keep_corrupted_file, int mode_ext2, int expert, int lowmem, int verbose)
Configures main recovery options.

**Parameters:**
- `paranoid` - Paranoid mode level (0=off, 1=normal, 2=maximum)
- `keep_corrupted_file` - Keep partially recovered files (0=no, 1=yes)
- `mode_ext2` - Enable EXT2/3/4 optimizations (0=no, 1=yes)
- `expert` - Enable expert mode (0=no, 1=yes)
- `lowmem` - Use low memory algorithms (0=no, 1=yes)
- `verbose` - Verbose output (0=no, 1=yes)

#### void change_status(ph_cli_context_t* ctx, photorec_status_t status)
Sets the initial recovery phase.

**Parameters:**
- `status` - Recovery status to set (STATUS_FIND_OFFSET, STATUS_EXT2_ON, etc.)

#### int change_blocksize(ph_cli_context_t* ctx, unsigned int blocksize)
Sets the block size for recovery operations.

**Parameters:**
- `blocksize` - Block size in bytes (0 for auto-detect)

**Returns:** 0 on success, non-zero on error

#### void change_carve_space(ph_cli_context_t* ctx, int free_space_only)
Configures whether to scan only free space or the entire partition.

**Parameters:**
- `free_space_only` - 1 to scan only free space, 0 to scan entire partition

### File Type Configuration

#### int change_all_fileopt(const ph_cli_context_t* ctx, int all_enable_status)
Enables or disables all file types.

**Parameters:**
- `all_enable_status` - 1 to enable all, 0 to disable all

**Returns:** 0 on success, non-zero on error

#### int change_fileopt(const ph_cli_context_t* ctx, char** exts_to_enable, int exts_to_enable_count, char** exts_to_disable, int exts_to_disable_count)
Selectively enables or disables specific file types.

**Parameters:**
- `exts_to_enable` - Array of file extensions to enable
- `exts_to_enable_count` - Number of extensions to enable
- `exts_to_disable` - Array of file extensions to disable
- `exts_to_disable_count` - Number of extensions to disable

**Returns:** 0 on success, non-zero on error

### Advanced Configuration

#### void change_geometry(ph_cli_context_t* ctx, unsigned int cylinders, unsigned int heads_per_cylinder, unsigned int sectors_per_head, unsigned int sector_size)
Manually sets disk geometry parameters.

**Parameters:**
- `cylinders` - Number of cylinders
- `heads_per_cylinder` - Number of heads per cylinder
- `sectors_per_head` - Number of sectors per head
- `sector_size` - Sector size in bytes

#### int config_photorec(ph_cli_context_t* ctx, char* cmd)
Provides a generic interface for advanced configuration commands.

**Parameters:**
- `cmd` - Configuration command string

**Returns:** 0 on success, non-zero on error

## Building and Integration

### Building the PhotoRec Libraries

```bash
# Configure the build system
./configure

# Build both static library variants
make library

# Install libraries and headers to system directories
make library-install
```

### Required Headers and Linking

```c
#include "photorec_api.h"
```

```bash
# If library is installed system-wide
gcc -o myapp myapp.c -lphotorec -lncurses -luuid

# Using local library build
gcc -o myapp myapp.c ./src/libphotorec.a -lncurses -luuid
```

### Dependencies
- libncurses (for interactive features)
- libuuid (for GUID support)  
- libext2fs (for EXT2/3/4 optimization, optional)
- libntfs/libntfs-3g (for NTFS optimization, optional)

## Error Handling and Best Practices

### Error Handling
```c
ph_cli_context_t* ctx = init_photorec(argc, argv);
if (!ctx) {
    fprintf(stderr, "Failed to initialize PhotoRec\n");
    return -1;
}

if (!change_disk(ctx, device_path)) {
    fprintf(stderr, "Cannot access device %s\n", device_path);
    finish_photorec(ctx);
    return -2;
}

int result = run_photorec(ctx);
if (result != 0) {
    fprintf(stderr, "Recovery failed with error %d\n", result);
}

finish_photorec(ctx);
```

### Best Practices

1. **Always validate inputs** - Check device paths and output directories before starting
2. **Handle interruption gracefully** - Monitor the `need_to_stop` global variable
3. **Free resources properly** - Always call `finish_photorec()` to clean up
4. **Configure file types appropriately** - Only recover needed file types to save time
5. **Monitor disk space** - Ensure adequate space in the output directory
6. **Use appropriate paranoid levels** - Higher levels provide better validation but slower recovery
7. **Test with small images first** - Validate your implementation before processing large disks

### Memory Management
- The context manages most memory automatically
- Always call `finish_photorec()` to free resources
- String parameters like `recup_dir` are managed by the context
- Don't free context members directly - use the cleanup functions

### Thread Safety
The PhotoRec API is not thread-safe. If using in a multi-threaded application, ensure proper synchronization around API calls.

## Conclusion

The PhotoRec API provides a powerful yet simple context-based interface for file recovery operations. The context structure encapsulates all state and configuration, making it easy to integrate PhotoRec functionality into custom applications while maintaining the full power of the underlying recovery engine.

For additional examples and advanced usage patterns, refer to the PhotoRec source code and the demo function in the implementation. 