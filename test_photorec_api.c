#include "photorec_api.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("PhotoRec API Library Test\n");
    printf("=========================\n");
    
    // Test basic functionality
    printf("Testing status name function: %s\n", status_to_name(STATUS_FIND_OFFSET));
    printf("Testing status name function: %s\n", status_to_name(STATUS_EXT2_ON));
    printf("Testing status name function: %s\n", status_to_name(STATUS_QUIT));
    
    // Test disk parsing (this will scan for available devices)
    printf("\nTesting disk discovery...\n");
    list_disk_t *list_disk = hd_parse(NULL, 1, TESTDISK_O_RDONLY);
    
    if (list_disk) {
        printf("Disk discovery successful - found disks\n");
        for (list_disk_t *disk = list_disk; disk != NULL; disk = disk->next) {
            printf("Disk: %s\n", disk->disk->description_txt);
        }
        delete_list_disk(list_disk);
    } else {
        printf("No disks found (normal on some systems)\n");
    }
    
    // Test file type array access
    printf("\nTesting file type configuration...\n");
    if (array_file_enable) {
        int count = 0;
        file_enable_t *file_enable = array_file_enable;
        
        // Count enabled file types
        while (file_enable->file_hint != NULL) {
            if (file_enable->enable) {
                count++;
            }
            file_enable++;
        }
        printf("Found %d enabled file types\n", count);
    }
    
    printf("\nPhotoRec API library test completed successfully!\n");
    return 0;
} 