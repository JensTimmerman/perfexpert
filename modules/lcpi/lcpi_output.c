/*
 * Copyright (c) 2011-2013  University of Texas at Austin. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * This file is part of PerfExpert.
 *
 * PerfExpert is free software: you can redistribute it and/or modify it under
 * the terms of the The University of Texas at Austin Research License
 *
 * PerfExpert is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * Authors: Leonardo Fialho and Ashay Rane
 *
 * $HEADER$
 */

#ifdef __cplusplus
extern "C" {
#endif

/* System standard headers */
#include <stdio.h>
#include <math.h>
#include <unistd.h>

/* Utility headers */

/* Modules headers */
#include "lcpi.h"
#include "lcpi_database.h"
#include "lcpi_output.h"

/* PerfExpert common headers */
#include "common/perfexpert_alloc.h"
#include "common/perfexpert_constants.h"
#include "common/perfexpert_hash.h"
#include "common/perfexpert_list.h"
#include "common/perfexpert_string.h"
#include "common/perfexpert_util.h"

/* prety_print */
#define PRETTY_PRINT(size, symbol) \
    {                              \
        int i = size;              \
        while (i > 0) {            \
            printf("%s", symbol);  \
            i--;                   \
        }                          \
        printf("\n");              \
    }

/* prety_print_bar */
#define PRETTY_PRINT_BAR(size, symbol)        \
    {                                         \
        int n = 0;                            \
        printf("[");                          \
        for (n = 0; n < 50; n++) {            \
            if (size > n) {                   \
                if ((n != 49) &&              \
                    (size <= 50)) {           \
                    printf("%s", symbol);     \
                } else {                      \
                    printf("+");              \
                }                             \
            } else {                          \
                if ((0 == (n+1) % 5)          \
                    && (n != 49)) {           \
                    printf("%s", _BLUE(".")); \
                } else {                      \
                    printf(" ");              \
                }                             \
            }                                 \
        }                                     \
        printf("]\n");                        \
    }

/* output_analysis */
int output_analysis(perfexpert_list_t *profiles) {
    lcpi_hotspot_t *h = NULL;
    lcpi_profile_t *p = NULL;
    lcpi_module_t *m = NULL, *t = NULL;
    char *shortname = NULL;

    OUTPUT_VERBOSE((4, "%s", _YELLOW("Printing analysis report")));

    /* For each profile in the list of profiles... */
    perfexpert_list_for(p, profiles, lcpi_profile_t) {
        /* Print total runtime for this profile */
        PRETTY_PRINT(81, "-");
        printf(
            "Total running time for %s is %.2f seconds between all %d cores\n",
            _CYAN(p->name), p->cycles / database_get_hound("CPU_freq"),
            (int)sysconf(_SC_NPROCESSORS_ONLN));
        printf("The wall-clock time for %s is approximately %.2f seconds\n\n",
            _CYAN(p->name), (p->cycles / database_get_hound("CPU_freq") /
                sysconf(_SC_NPROCESSORS_ONLN)));

        /* For each module in the profile's list of modules... */
        perfexpert_hash_iter_str(p->modules_by_name, m, t) {
            perfexpert_util_filename_only(m->name, &shortname);
            m->importance = m->cycles / p->cycles;
            printf("Module %s takes %.2f%% of the total runtime\n",
                _MAGENTA(shortname), m->importance * 100);
        }
        printf("\n");

        /* For each hotspot in the profile's list of hotspots... */
        perfexpert_list_for(h, &(p->hotspots), lcpi_hotspot_t) {
            if (my_module_globals.threshold <= h->importance) {
                if (PERFEXPERT_SUCCESS != output_profile(h)) {
                    OUTPUT(("%s (%s)",
                        _ERROR("printing hotspot analysis"), h->name));
                    return PERFEXPERT_ERROR;
                }
            }
        }
    }
    return PERFEXPERT_SUCCESS;
}

/* output_profile */
static int output_profile(lcpi_hotspot_t *h) {
    int print_ratio = PERFEXPERT_TRUE, warn_fp_ratio = PERFEXPERT_FALSE;
    lcpi_metric_t *l = NULL, *t = NULL;
    char *shortname = NULL;

    OUTPUT_VERBOSE((4, "   [%d] %s", h->id, _YELLOW(h->name)));

    /* Print the runtime of this hotspot */
    perfexpert_util_filename_only(h->file, &shortname);
    switch (h->type) {
        case PERFEXPERT_HOTSPOT_PROGRAM:
            printf("Aggregate (%.2f%% of the total runtime)\n",
                h->importance * 100);
            break;

        case PERFEXPERT_HOTSPOT_FUNCTION:
            printf(
                "Function %s in line %d of %s (%.2f%% of the total runtime)\n",
                _CYAN(h->name), h->line, shortname, h->importance * 100);
            break;

        case PERFEXPERT_HOTSPOT_LOOP:
            printf(
                "Loop in function %s in %s:%d (%.2f%% of the total runtime)\n",
                _CYAN(h->name), shortname, h->line, h->importance * 100);
            break;

        case PERFEXPERT_HOTSPOT_UNKNOWN:
        default:
            return PERFEXPERT_ERROR;
    }
    PERFEXPERT_DEALLOC(shortname);

    /* Print an horizontal double-line */
    PRETTY_PRINT(81, "=");

    /* For each metric... */
    perfexpert_hash_iter_str(h->metrics_by_name, l, t) {
        char *temp = NULL, *cat = NULL, *subcat = NULL, desc[24];

        PERFEXPERT_ALLOC(char, temp, (strlen(l->name) + 1));
        strcpy(temp, l->name);
        perfexpert_string_replace_char(temp, '_', ' ');

        cat = strtok(temp, ".");
        subcat = strtok(NULL, ".");

        /* Format LCPI description */
        bzero(desc, 24);
        if (NULL != strstr(l->name, "overall")) {
            sprintf(desc, "* %s", cat);
        } else {
            sprintf(desc, " - %s", subcat);
        }
        while (23 > strlen(desc)) {
            strcat(desc, " ");
        }

        /* Print ratio and GFLOPS section */
        if ((0 == strcmp(cat, "ratio")) ||
            (0 == strncmp(cat, "GFLOPS", 6))) {
            if (PERFEXPERT_TRUE == print_ratio) {
                printf("%s", _WHITE("Instructions Ratio        %   "));
                printf("%s\n", _CYAN("0..........25..........50...........75"
                    ".........100"));
                print_ratio = PERFEXPERT_FALSE;
            }
            if (100 > (l->value * 100)) {
                printf("%s %4.1f ", desc, (l->value * 100));
                PRETTY_PRINT_BAR((int)rint((l->value * 50)), ">");
            } else {
                printf("%s%4.1f ", desc, (l->value * 100));
                PRETTY_PRINT_BAR(50, ">");
                warn_fp_ratio = PERFEXPERT_TRUE;
            }
        }

        /* Print LCPI section: special colors for overall */
        if (0 == strcmp(cat, "overall")) {
            printf("\n%s", _WHITE("Performance Assessment  LCPI  "));
            printf("%s\n", _CYAN("good.......okay........fair........poor"
                "........bad"));
            if (0.5 >= l->value) {
                printf("%s%5.2f ", _GREEN(desc), l->value);
                PRETTY_PRINT_BAR((int)rint((l->value * 20)), _GREEN(">"));
            } else if ((0.5 < l->value) && (1.5 >= l->value)) {
                printf("%s%5.2f ", _YELLOW(desc), l->value);
                PRETTY_PRINT_BAR((int)rint((l->value * 20)), _YELLOW(">"));
            } else if ((1.5 < l->value) && (2.5 >= l->value)) {
                printf("%s%5.2f ", _RED(desc), l->value);
                PRETTY_PRINT_BAR((int)rint((l->value * 20)), _RED(">"));
            } else {
                printf("%s%5.2f ", _BOLDRED(desc), l->value);
                PRETTY_PRINT_BAR((int)rint((l->value * 20)), _BOLDRED(">"));
            }
            printf("\n%s\n", _WHITE("Slowdown Caused By      LCPI    "
                "(interpretation varies according to the metric)"));
        } else if ((0 == strcmp(cat, "data accesses")) ||
            (0 == strcmp(cat, "instruction accesses")) ||
            (0 == strcmp(cat, "data TLB")) ||
            (0 == strcmp(cat, "instruction TLB")) ||
            (0 == strcmp(cat, "branch instructions")) ||
            (0 == strcmp(cat, "FP instructions"))) {
            printf("%s%5.2f ", desc, l->value);
            PRETTY_PRINT_BAR((int)rint((l->value * 20)), ">");
        }
        PERFEXPERT_DEALLOC(temp);
    }

    /* Do we have something meaningful to show? Some warning? */
    if (LCPI_VARIANCE_LIMIT < h->variance) {
        printf("\n%s the instruction count variation for this bottleneck is "
            "%.2f%%, making\n         the results unreliable!",
            _BOLDRED("WARNING:"), h->variance * 100);
    }
    if ((0 == h->cycles) || (0 == h->instructions)) {
        printf("\n%s the runtime for this code section is too short, PerfExpert"
            " was unable\n         to collect the performance counters it needs"
            ", making the results\n         unreliable!",
            _BOLDRED("WARNING:"));
    }
    if (database_get_hound("CPU_freq") > h->cycles) {
        printf("\n%s the runtime for this code section is too short to gather "
            "meaningful\n         measurements!\n", _BOLDRED("WARNING:"));
        PRETTY_PRINT(81, "-");
        printf("\n");
        return PERFEXPERT_SUCCESS;
    }
    if (database_get_hound("CPI_threshold") >= h->cycles / h->instructions) {
        printf("\n%s  this code section performs just fine!",
            _BOLDGREEN("NOTICE:"));
    }
    if (PERFEXPERT_TRUE == warn_fp_ratio) {
        printf("\n%s this architecture overcounts floating-point operations, "
            "expect to see\n         more than 100%% of these instructions!",
            _BOLDRED("WARNING:"));
    }
    printf("\n");
    PRETTY_PRINT(81, "-");
    printf("\n");

    return PERFEXPERT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

// EOF
