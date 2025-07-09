#include <stdio.h>
#include "src/photorec_api.h"

// Logging function declarations
void log_disks(ph_cli_context_t* ctx);
void log_partitions(ph_cli_context_t* ctx);
void log_search_space(ph_cli_context_t* ctx);
void log_architectures(ph_cli_context_t* ctx);
void log_enabled_file_formats(ph_cli_context_t* ctx);
void log_options(ph_cli_context_t* ctx);
void log_selected_disk(ph_cli_context_t* ctx);
void log_selected_partition(ph_cli_context_t* ctx);
void log_progress_parameters(ph_cli_context_t* ctx);

int main(const int argc, char* argv[])
{
    printf("Testing PhotoRec API Library...\n");

    // Initialize PhotoRec context
    ph_cli_context_t* ctx = init_photorec(
        argc, argv, "/Users/ligengwang/Downloads/test_recup_dir", "/Volumes/thinkplus/demo/disk1.img", 2, "test.log");
    if (ctx == NULL)
    {
        printf("Error: Failed to initialize PhotoRec context\n");
        return 1;
    }
    printf("✓ PhotoRec context initialized successfully\n");

    // Test changing options
    change_options(ctx, 1, 0, 1, 0, 0, 1);
    printf("✓ Options configured: paranoid=1, keep_corrupted=0, ext2_mode=1, ...\n");

    int result = 0;

    // // Test changing blocksize
    // result = change_blocksize(ctx, 4096);
    // printf("✓ Blocksize changed to 4096, result: %d\n", result);

    // // Test file format configuration
    // result = change_all_fileopt(ctx, 1);
    // printf("✓ All file formats enabled, result: %d\n", result);

    // Test disk
    const char* device = "/Volumes/thinkplus/demo/disk1.img";
    disk_t* disk = change_disk(ctx, device);
    printf("✓ Disk changed to %s, result: %p\n", device, disk);

    // Test partition
    partition_t* part = change_part(ctx, 1, 1, 0);
    printf("✓ Partition changed to 1, result: %p\n", part);

    // Log all context information using separate functions
    log_disks(ctx);
    log_partitions(ctx);
    log_search_space(ctx);
    log_architectures(ctx);
    log_enabled_file_formats(ctx);
    log_options(ctx);
    log_selected_disk(ctx);
    log_selected_partition(ctx);
    log_progress_parameters(ctx);

    // ctx->params.blocksize = 0;
    // config_photorec(ctx, "search");

    // Run PhotoRec
    result = run_photorec(ctx);
    printf("✓ PhotoRec run completed, result: %d\n", result);

    // // Test status change
    // change_status(ctx, STATUS_EXT2_ON);
    // printf("✓ Status changed to STATUS_EXT2_ON\n");

    printf("✓ PhotoRec API test completed successfully!\n");

    // Clean up
    finish_photorec(ctx);
    printf("✓ PhotoRec context cleaned up\n");

    return 0;
}

void log_disk(disk_t *disk) {
    printf("  Disk: %p\n", disk);
    printf("    Name (%p): %s\n", &disk->device, disk->device);
    printf("    Size (%p): %llu\n", &disk->disk_size, disk->disk_size);
    printf("    Sector size (%p): %d\n", &disk->sector_size, disk->sector_size);
    printf("    Arch (%p): %p\n", &disk->arch, disk->arch);
    printf("    Unit (%p): %d\n", &disk->unit, disk->unit);
    printf("    Description: %s\n", disk->description_txt);
}

// Logging function implementations
void log_disks(ph_cli_context_t* ctx)
{
    printf("List disk from test_photorec_api.c:\n");
    for (list_disk_t* node = ctx->list_disk; node != NULL; node = node->next)
    {
        log_disk(node->disk);
    }
}

void log_partition(partition_t *part) {
    printf("  Part: %p\n", part);
    printf("    Order: %d\n", part->order);
    printf("    Name: %s\n", part->fsname);
    printf("    Blocksize: %d\n", part->blocksize);
    printf("    Size: %llu\n", part->part_size);
    printf("    Info: %s\n", part->info);
    printf("    Partname: %s\n", part->partname);
    printf("    Fsname: %s\n", part->fsname);
    printf("    Type: %s\n", part->info);
    printf("    Start: %llu\n", part->part_offset);
    printf("    End: %llu\n", part->part_offset + part->part_size);
}

void log_partitions(ph_cli_context_t* ctx)
{
    printf("List part:\n");
    for (list_part_t* node = ctx->list_part; node != NULL; node = node->next)
    {
        log_partition(node->part);
    }
}

void log_search_space(ph_cli_context_t* ctx)
{
    printf("List search space:\n");
    alloc_data_t* search_space = &ctx->list_search_space;
    printf("  Search space: %p\n", search_space);
    printf("    Start: %llu\n", search_space->start);
    printf("    End: %llu\n", search_space->end);
    printf("    Data: %d\n", search_space->data);
}

void log_arch(const arch_fnct_t* arch) {
    printf("  Arch: %p\n", arch);
    printf("    Name: %s\n", arch->part_name);
    printf("    Name option: %s\n", arch->part_name_option);
    printf("    Msg part type: %s\n", arch->msg_part_type);
}

void log_architectures(ph_cli_context_t* ctx)
{
    printf("List arch:\n");
    for (int i = 0; ctx->list_arch[i] != NULL; i++)
    {
        log_arch(ctx->list_arch[i]);
    }
}

void log_enabled_file_formats(ph_cli_context_t* ctx)
{
    printf("Enabled file formats:\n");
    for (int i = 0; ctx->options.list_file_format[i].file_hint != NULL; i++)
    {
        if (ctx->options.list_file_format[i].enable)
        {
            printf("  %s\n", ctx->options.list_file_format[i].file_hint->extension);
        }
    }
}

void log_options(ph_cli_context_t* ctx)
{
    printf("Selected status: %d\n", ctx->params.status);
    printf("Selected paranoid: %d\n", ctx->options.paranoid);
    printf("Selected keep_corrupted_file: %d\n", ctx->options.keep_corrupted_file);
    printf("Selected blocksize: %d\n", ctx->params.blocksize);
    printf("Selected carve_free_space_only: %d\n", ctx->params.carve_free_space_only);
    printf("Selected mode_ext2: %d\n", ctx->options.mode_ext2);
    printf("Selected lowmem: %d\n", ctx->options.lowmem);
    printf("Selected verbose: %d\n", ctx->options.verbose);
    printf("Selected expert: %d\n", ctx->options.expert);
    printf("Selected list_file_format: %p\n", ctx->options.list_file_format);
}

void log_selected_disk(ph_cli_context_t* ctx)
{
    printf("Selected disk:\n");
    log_disk(ctx->params.disk);
}

void log_selected_partition(ph_cli_context_t* ctx)
{
    printf("Selected partition:\n");
    log_partition(ctx->params.partition);
}

void log_progress_parameters(ph_cli_context_t* ctx)
{
    printf("Selected pass: %d\n", ctx->params.pass);
    printf("Selected recup_dir: %s\n", ctx->params.recup_dir);
    printf("Selected dir_num: %d\n", ctx->params.dir_num);
    printf("Selected file_nbr: %d\n", ctx->params.file_nbr);
    printf("Selected file_stats: %p\n", ctx->params.file_stats);
}
