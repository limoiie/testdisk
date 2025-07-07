#include <stdio.h>
#include "photorec_api.h"

int main(const int argc, char* argv[])
{
    printf("Testing PhotoRec API Library...\n");

    // Initialize PhotoRec context
    ph_cli_context_t* ctx = init_photorec(argc, argv);
    if (ctx == NULL)
    {
        printf("Error: Failed to initialize PhotoRec context\n");
        return 1;
    }
    printf("✓ PhotoRec context initialized successfully\n");

    // Test changing options
    change_options(ctx, 1, 0, 1, 0, 0, 1);
    printf("✓ Options configured: paranoid=1, keep_corrupted=0, ext2_mode=1, ...\n");

    // Test changing blocksize
    int result = change_blocksize(ctx, 512);
    printf("✓ Blocksize changed to 512, result: %d\n", result);

    // Test status change
    change_status(ctx, STATUS_EXT2_ON);
    printf("✓ Status changed to STATUS_EXT2_ON\n");

    // Test file format configuration
    result = change_all_fileopt(ctx, 1);
    printf("✓ All file formats enabled, result: %d\n", result);

    // Print the context
    // -Print the list_disk
    printf("List disk:\n");
    for (list_disk_t* disk = ctx->list_disk; disk != NULL; disk = disk->next)
    {
        printf("  Disk: %p\n", disk);
        printf("    Name: %s\n", disk->disk->device);
        printf("    Size: %llu\n", disk->disk->disk_size);
        printf("    Sector size: %d\n", disk->disk->sector_size);
        printf("    Arch: %p\n", disk->disk->arch);
        printf("    Unit: %d\n", disk->disk->unit);
        printf("    Description: %s\n", disk->disk->description_txt);
    }
    // -Print the list_part
    printf("List part:\n");
    for (list_part_t* part = ctx->list_part; part != NULL; part = part->next)
    {
        printf("  Part: %p\n", part);
        printf("    Name: %s\n", part->part->fsname);
        printf("    Size: %llu\n", part->part->part_size);
        printf("    Offset: %llu\n", part->part->part_offset);
    }
    // -Print the list_search_space
    printf("List search space:\n");
    alloc_data_t* search_space = &ctx->list_search_space;
    printf("  Search space: %p\n", search_space);
    printf("    Start: %llu\n", search_space->start);
    printf("    End: %llu\n", search_space->end);
    printf("    Data: %d\n", search_space->data);

    // -Print the list_arch
    printf("List arch:\n");
    for (int i = 0; ctx->list_arch[i] != NULL; i++)
    {
        printf("  Arch: %p\n", ctx->list_arch[i]);
        printf("    Name: %s\n", ctx->list_arch[i]->part_name);
        printf("    Name option: %s\n", ctx->list_arch[i]->part_name_option);
        printf("    Msg part type: %s\n", ctx->list_arch[i]->msg_part_type);
    }

    printf("✓ PhotoRec API test completed successfully!\n");

    // Clean up
    finish_photorec(ctx);
    printf("✓ PhotoRec context cleaned up\n");

    return 0;
}
