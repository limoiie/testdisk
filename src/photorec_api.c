#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(DISABLED_FOR_FRAMAC)
#undef ENABLE_DFXML
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>	/* unlink, ftruncate */
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <ctype.h>      /* tolower */
#include "types.h"
#include "common.h"
#include "intrf.h"
#include <errno.h>
#ifdef HAVE_WINDEF_H
#include <windef.h>
#endif
#ifdef HAVE_WINBASE_H
#include <stdarg.h>
#include <winbase.h>
#endif
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include "dir.h"
#include "fat.h"
#include "fat_dir.h"
#include "list.h"
#include "lang.h"
#include "filegen.h"
#include "sessionp.h"
#include "photorec.h"
#include "phrecn.h"
#include "log.h"
#include "log_part.h"
#include "file_tar.h"
#include "phcfg.h"
#include "pblocksize.h"
#include "askloc.h"
#include "autoset.h"
#include "chgarch.h"
#include "common.h"
#include "fat_unformat.h"
#include "pnext.h"
#include "phbf.h"
#include "phbs.h"
#include "file_found.h"
#include "dfxml.h"
#include "fnctdsk.h"
#include "hdaccess.h"
#include "hdcache.h"
#include "partauto.h"
#include "pdisksel.h"
#include "pfree_whole.h"
#include "phcli.h"
#include "poptions.h"
#include "psearchn.h"

/* Global variable for stopping recovery operations */
int need_to_stop = 0;

/* Context type definition */
typedef struct ph_cli_context
{
    struct ph_options options;
    struct ph_param params;
    int mode;
    const arch_fnct_t** list_arch;
    list_disk_t* list_disk;
    list_part_t* list_part;
    alloc_data_t list_search_space;
    int log_opened;
    int log_errno;
} ph_cli_context_t;

extern const file_enable_t array_file_enable[];

extern const arch_fnct_t arch_none;
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_GPT)
extern const arch_fnct_t arch_gpt;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_HUMAX)
extern const arch_fnct_t arch_humax;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_I386)
extern const arch_fnct_t arch_i386;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC)
extern const arch_fnct_t arch_mac;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_SUN)
extern const arch_fnct_t arch_sun;
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_XBOX)
extern const arch_fnct_t arch_xbox;
#endif

static const arch_fnct_t* list_arch[] = {
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_I386)
    &arch_i386,
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_GPT)
    &arch_gpt,
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_HUMAX)
    &arch_humax,
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_MAC)
    &arch_mac,
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_NONE)
    &arch_none,
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_SUN)
    &arch_sun,
#endif
#if !defined(SINGLE_PARTITION_TYPE) || defined(SINGLE_PARTITION_XBOX)
    &arch_xbox,
#endif
    NULL
};

list_disk_t* init_list_disk(const ph_cli_context_t* ctx)
{
    list_disk_t* list_disk = NULL;
    if (ctx->params.cmd_device)
    {
        // If cmd_device is provided, that is, explicitly specifying the disk
        disk_t* disk_car = file_test_availability(ctx->params.cmd_device,
                                                  ctx->options.verbose, ctx->mode);
        list_disk = insert_new_disk(list_disk, disk_car);
    }
#ifndef DISABLED_FOR_FRAMAC
    if (list_disk == NULL)
    {
        // Otherwise, list all available disks
        list_disk = hd_parse(list_disk, ctx->options.verbose, ctx->mode);
    }
    hd_update_all_geometry(list_disk, ctx->options.verbose); // Update geometry
    for (list_disk_t* element_disk = list_disk;
         element_disk != NULL;
         element_disk = element_disk->next)
    {
        // Init cache
        element_disk->disk = new_diskcache(element_disk->disk, ctx->mode);
    }
#endif
    return list_disk;
}


disk_t* change_disk(ph_cli_context_t* ctx, const char* device)
{
    disk_t* selected = photorec_disk_selection_cli(device, ctx->list_disk,
                                                   &ctx->list_search_space);
    if (selected == NULL)
    {
        return NULL;
    }
    ctx->params.disk = selected;
    ctx->list_part = init_list_part(selected, &ctx->options);
    return selected;
}

const arch_fnct_t* change_arch(const ph_cli_context_t* ctx, char* part_name_option)
{
    autodetect_arch(ctx->params.disk, &arch_none);
    change_arch_type_cli(ctx->params.disk, ctx->options.verbose, &part_name_option);
    autoset_unit(ctx->params.disk);
    return ctx->params.disk->arch;
}

partition_t* change_part(ph_cli_context_t* ctx, const int order, const int mode_ext2,
                         const int carve_free_space_only)
{
    for (const list_part_t* element = ctx->list_part;
         element != NULL;
         element = element->next)
    {
        if (element->part->order == order)
        {
            ctx->params.partition = element->part;
            ctx->params.carve_free_space_only = carve_free_space_only;
            ctx->options.mode_ext2 = mode_ext2;

            // Initialize search space
            if (td_list_empty(&ctx->list_search_space.list))
            {
                init_search_space(&ctx->list_search_space, ctx->params.disk,
                                  ctx->params.partition);
            }

            // Initialize blocksize
            if (ctx->params.carve_free_space_only > 0)
            {
                ctx->params.blocksize = remove_used_space(
                    ctx->params.disk, ctx->params.partition, &ctx->list_search_space);
                /* Only free space is carved, list_search_space is modified.
                 * To carve the whole space, need to quit and reselect the params->partition */
            }
            else
            {
                ctx->params.blocksize = ctx->params.partition->blocksize;
            }
            return element->part;
        }
    }
    return NULL;
}

void change_status(ph_cli_context_t* ctx, const photorec_status_t status)
{
#ifndef DISABLED_FOR_FRAMAC
    ctx->params.status = status;
#endif
}

void change_options(ph_cli_context_t* ctx, const int paranoid,
                    const int keep_corrupted_file, const int mode_ext2, const int expert,
                    const int lowmem, const int verbose)
{
    ctx->options.paranoid = paranoid;
    ctx->options.keep_corrupted_file = keep_corrupted_file;
    ctx->options.mode_ext2 = mode_ext2;
    ctx->options.expert = expert;
    ctx->options.lowmem = lowmem;
    ctx->options.verbose = verbose;
}

int change_all_fileopt(const ph_cli_context_t* ctx, const int all_enable_status)
{
    for (file_enable_t* file_enable = &ctx->options.list_file_format[0];
         file_enable->file_hint != NULL;
         file_enable++)
    {
        file_enable->enable = all_enable_status;
    }
    return 0;
}

int change_fileopt(const ph_cli_context_t* ctx,
                   char** exts_to_enable, const int exts_to_enable_count,
                   char** exts_to_disable, const int exts_to_disable_count)
{
    for (file_enable_t* file_enable = &ctx->options.list_file_format[0];
         file_enable->file_hint != NULL;
         file_enable++)
    {
        for (int i = 0; i < exts_to_enable_count; i++)
        {
            if (strncmp(file_enable->file_hint->extension, exts_to_enable[i],
                        strlen(file_enable->file_hint->extension)) == 0)
            {
                file_enable->enable = 1;
            }
        }
        for (int i = 0; i < exts_to_disable_count; i++)
        {
            if (strncmp(file_enable->file_hint->extension, exts_to_disable[i],
                        strlen(file_enable->file_hint->extension)) == 0)
            {
                file_enable->enable = 0;
            }
        }
    }
    return 0;
}

int change_blocksize(ph_cli_context_t* ctx, const unsigned int blocksize)
{
    if (blocksize > 0)
    {
        ctx->params.blocksize = blocksize;
    }
    else
    {
        ctx->params.blocksize = 0;
    }
    return 0;
}

void change_geometry(ph_cli_context_t* ctx, const unsigned int cylinders,
                     const unsigned int heads_per_cylinder,
                     const unsigned int sectors_per_head,
                     const unsigned int sector_size)
{
    char* x = MALLOC(100);
    sprintf(x, "geometry,C,%u,H,%u,S,%u,N,%u", cylinders, heads_per_cylinder,
            sectors_per_head, sector_size);
    ctx->params.cmd_run = x;
    menu_photorec_cli(ctx->list_part, &ctx->params, &ctx->options,
                      &ctx->list_search_space);
    free(x);
}

void change_ext2_mode(ph_cli_context_t* ctx, const int group_number)
{
    char* x = MALLOC(100);
    sprintf(x, "ext2_group,%d", group_number);
    ctx->params.cmd_run = x;
    menu_photorec_cli(ctx->list_part, &ctx->params, &ctx->options,
                      &ctx->list_search_space);
    free(x);
}

void change_ext2_inode(ph_cli_context_t* ctx, const int inode_number)
{
    char* x = MALLOC(100);
    sprintf(x, "ext2_inode,%d", inode_number);
    ctx->params.cmd_run = x;
    menu_photorec_cli(ctx->list_part, &ctx->params, &ctx->options,
                      &ctx->list_search_space);
    free(x);
}

int config_photorec(ph_cli_context_t* ctx, char* cmd)
{
    ctx->params.cmd_run = cmd;
    return menu_photorec_cli(ctx->list_part, &ctx->params, &ctx->options,
                             &ctx->list_search_space);
}

ph_cli_context_t* init_photorec(int argc, char* argv[], char* recup_dir, char* device,
                                const int log_mode, const char* log_file)
{
#if defined(ENABLE_DFXML)
    xml_set_command_line(argc, argv);
#endif

    ph_cli_context_t* ctx = MALLOC(sizeof(ph_cli_context_t));
    *ctx = (ph_cli_context_t){
        .options = {
            .paranoid = 1,
            .keep_corrupted_file = 0,
            .mode_ext2 = 0,
            .expert = 0,
            .lowmem = 0,
            .verbose = log_mode == 2 ? 1 : 0,
            .list_file_format = (file_enable_t*)array_file_enable
        },
        .params = {
            .recup_dir = recup_dir,
            .cmd_device = device,
            .cmd_run = NULL,
            .carve_free_space_only = 0,
            .disk = NULL
        },
        .mode = TESTDISK_O_RDONLY | TESTDISK_O_READAHEAD_32K,
        .list_arch = list_arch,
        .list_disk = NULL,
        .list_part = NULL,
        .list_search_space = {
            .list = TD_LIST_HEAD_INIT(ctx->list_search_space.list)
        },
        .log_opened = 0,
        .log_errno = 0
    };

    // TODO
    // Exec Mode
    // ctx->mode |= TESTDISK_O_ALL; // For /all
    // ctx->mode |= TESTDISK_O_DIRECT; // For /direct

    // Prepare enabled file_options
    reset_array_file_enable(ctx->options.list_file_format);
    file_options_load(ctx->options.list_file_format);

    // List disks, then update their metadata
    ctx->list_disk = init_list_disk(ctx);
    ctx->log_opened = log_open(log_file, log_mode == 0 ? TD_LOG_NONE : TD_LOG_APPEND,
                               &ctx->log_errno);

    log_disk_list(ctx->list_disk);
    return ctx;
}

void finish_photorec(ph_cli_context_t* ctx)
{
    part_free_list(ctx->list_part);
#ifndef DISABLED_FOR_FRAMAC
    delete_list_disk(ctx->list_disk);
#endif
#ifdef ENABLE_DFXML
    xml_clear_command_line();
#endif
    free(ctx);
}

int run_photorec(ph_cli_context_t* ctx)
{
    need_to_stop = 0;
    struct ph_param* params = &ctx->params;
    const struct ph_options* options = &ctx->options;
    alloc_data_t* list_search_space = &ctx->list_search_space;

    pstatus_t ind_stop = PSTATUS_OK;
    const unsigned int blocksize_is_known = params->blocksize;

    /*@ assert valid_read_string(ctx->params.recup_dir); */
    params_reset(params, options);

    /*@ assert valid_read_string(ctx->params.recup_dir); */
    log_info("params->cmd_run: %s\n", params->cmd_run);
    log_info("params->cmd_device: %s\n", params->cmd_device);
    log_info("params->status: %d\n", params->status);
    log_info("params->blocksize: %d\n", params->blocksize);
    log_info("params->pass: %d\n", params->pass);
    log_info("params->file_nbr: %d\n", params->file_nbr);
    log_info("params->file_stats: %p\n", params->file_stats);
    log_info("params->recup_dir: %s\n", params->recup_dir);
    log_info("params->dir_num: %d\n", params->dir_num);
    log_info("params->disk: %p\n", params->disk);
    log_info("params->disk->device: %s\n", params->disk->device);
    log_info("params->disk->disk_size: %llu\n", params->disk->disk_size);
    log_info("params->disk->sector_size: %d\n", params->disk->sector_size);
    log_info("params->disk->arch: %p\n", params->disk->arch);
    log_info("params->disk->unit: %d\n", params->disk->unit);
    log_info("params->partition: %p\n", params->partition);
    log_info("params->real_start_time: %ld\n", params->real_start_time);
    log_info("params->carve_free_space_only: %d\n", params->carve_free_space_only);
    log_info("params->list_search_space: %p\n", list_search_space);
    log_info("params->options: %p\n", options);
    log_info("params->options->paranoid: %d\n", options->paranoid);
    log_info("params->options->keep_corrupted_file: %d\n", options->keep_corrupted_file);
    log_info("params->options->mode_ext2: %d\n", options->mode_ext2);
    log_info("params->options->expert: %d\n", options->expert);
    log_info("params->options->lowmem: %d\n", options->lowmem);
    log_info("params->options->verbose: %d\n", options->verbose);

    /* Handle command-line status options - moved to change_status(..) */

#ifndef DISABLED_FOR_FRAMAC
    log_info("\nAnalyse\n");
    log_partition(params->disk, params->partition);
#endif
    /*@ assert valid_read_string(params->recup_dir); */

    /* make the first recup_dir */
    params->dir_num = photorec_mkdir(params->recup_dir, params->dir_num);

#ifdef ENABLE_DFXML
  /* Open the XML output file */
  xml_open(params->recup_dir, params->dir_num);
  xml_setup(params->disk, params->partition);
#endif

    /*@
      @ loop invariant valid_ph_param(params);
      @*/
    for (params->pass = 0; params->status != STATUS_QUIT; params->pass++)
    {
        const unsigned int old_file_nbr = params->file_nbr;
#ifndef DISABLED_FOR_FRAMAC
        log_info("Pass %u (blocksize=%u) ", params->pass, params->blocksize);
        log_info("%s\n", status_to_name(params->status));
#endif

        /* CLI mode: No ncurses progress display, just log progress */
        log_info("Starting recovery pass %u with blocksize %u\n", params->pass,
                 params->blocksize);

        switch (params->status)
        {
        case STATUS_UNFORMAT:
#ifndef DISABLED_FOR_FRAMAC
            ind_stop = fat_unformat(params, options, list_search_space);
#endif
            params->blocksize = blocksize_is_known;
            break;
        case STATUS_FIND_OFFSET:
#ifndef DISABLED_FOR_FRAMAC
            {
                uint64_t start_offset = 0;
                if (blocksize_is_known > 0)
                {
                    ind_stop = PSTATUS_OK;
                    if (!td_list_empty(&list_search_space->list))
                        start_offset = (td_list_first_entry(
                            &list_search_space->list, alloc_data_t,
                            list))->start % params->blocksize;
                }
                else
                {
                    ind_stop =
                        photorec_find_blocksize(params, options, list_search_space);
                    params->blocksize = find_blocksize(
                        list_search_space, params->disk->sector_size, &start_offset);
                }
                /* CLI mode: No interactive blocksize menu, use detected blocksize */
                log_info("Using blocksize: %u, start_offset: %llu\n", params->blocksize,
                         (unsigned long long)start_offset);
                update_blocksize(params->blocksize, list_search_space, start_offset);
            }
#else
	params->blocksize=512;
#endif
            break;
        case STATUS_EXT2_ON_BF:
        case STATUS_EXT2_OFF_BF:
#ifndef DISABLED_FOR_FRAMAC
            ind_stop = photorec_bf(params, options, list_search_space);
#endif
            break;
        default:
            ind_stop = photorec_aux(params, options, list_search_space);
            break;
        }
        session_save(list_search_space, params, options);
        if (need_to_stop != 0)
            ind_stop = PSTATUS_STOP;
        switch (ind_stop)
        {
        case PSTATUS_ENOSPC:
            /* CLI mode: No interactive destination selection, just quit */
            log_critical("No more space available. Recovery stopped.\n");
            params->status = STATUS_QUIT;
            break;
        case PSTATUS_EACCES:
            /* CLI mode: No interactive retry, just quit */
            log_critical("Cannot create file. Recovery stopped.\n");
            params->status = STATUS_QUIT;
            break;
        case PSTATUS_STOP:
            if (session_save(list_search_space, params, options) < 0)
            {
                /* CLI mode: No user confirmation, automatically quit on session save failure */
                log_critical(
                    "PhotoRec has been unable to save its session status. Quitting.\n");
                params->status = STATUS_QUIT;
            }
            else
            {
                log_flush();
                /* CLI mode: No user confirmation, automatically quit on stop */
                log_info("PhotoRec has been stopped. Session saved.\n");
                params->status = STATUS_QUIT;
            }
            break;
        case PSTATUS_OK:
            status_inc(params, options);
            if (params->status == STATUS_QUIT)
                unlink("photorec.ses");
            break;
        }
#ifndef DISABLED_FOR_FRAMAC
        {
            const time_t current_time = time(NULL);
            log_info("Elapsed time %uh%02um%02us\n",
                     (unsigned)((current_time-params->real_start_time)/60/60),
                     (unsigned)((current_time-params->real_start_time)/60%60),
                     (unsigned)((current_time-params->real_start_time)%60));
        }
        update_stats(params->file_stats, list_search_space);
        if (params->pass > 0)
        {
            log_info("Pass %u +%u file%s\n", params->pass, params->file_nbr-old_file_nbr,
                     (params->file_nbr-old_file_nbr<=1?"":"s"));
            write_stats_log(params->file_stats);
        }
        log_flush();
#endif
    }

    /* CLI mode: No interactive image creation prompt, skip it */
    log_info("Recovery completed. Skipping interactive image creation.\n");

    info_list_search_space(list_search_space, NULL, params->disk->sector_size,
                           options->keep_corrupted_file, options->verbose);

    /* Free memory */
    free_search_space(list_search_space);

    /* CLI mode: No interactive completion screen, just log results */
    log_info("Recovery finished: %u files saved in %s directory.\n", params->file_nbr,
             params->recup_dir);
    switch (ind_stop)
    {
    case PSTATUS_OK:
        log_info("Recovery completed successfully.\n");
        if (params->file_nbr > 0)
        {
            log_info(
                "You are welcome to donate to support and encourage further development:\n");
            log_info("https://www.cgsecurity.org/wiki/Donation\n");
        }
        break;
    case PSTATUS_STOP:
        log_info("Recovery aborted by the user.\n");
        break;
    case PSTATUS_EACCES:
        log_critical("Cannot create file in current directory.\n");
        break;
    case PSTATUS_ENOSPC:
        log_critical("Cannot write file, no space left.\n");
        break;
    }

    free(params->file_stats);
    params->file_stats = NULL;
    free_header_check();
#ifdef ENABLE_DFXML
  xml_shutdown();
  xml_close();
#endif
    return 0;
}

void abort_photorec(ph_cli_context_t*)
{
    need_to_stop = 1;
}
