/*
 * Copyright (c) 2013  University of Texas at Austin. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * This file is part of PerfExpert.
 *
 * PerfExpert is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * PerfExpert is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PerfExpert. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Leonardo Fialho
 *
 * $HEADER$
 */

#ifdef __cplusplus
extern "C" {
#endif

/* System standard headers */
#include <stdio.h>
#include <stdlib.h>

/* Utility headers */
#include <sqlite3.h>

/* PerfExpert headers */
#include "config.h"
#include "ct.h"
#include "perfexpert_constants.h"
#include "perfexpert_list.h"
#include "perfexpert_output.h"
#include "perfexpert_database.h"

/* Global variables, try to not create them! */
globals_t globals; // Variable to hold global options, this one is OK

/* main, life starts here */
int ct_main(int argc, char** argv) {
    perfexpert_list_t *fragments;
    fragment_t *fragment;
    recommendation_t *recommendation;
    int rc = PERFEXPERT_NO_TRANS;

    /* Set default values for globals */
    globals = (globals_t) {
        .verbose_level = 0,              // int
        .inputfile     = NULL,           // char *
        .inputfile_FP  = stdin,          // char *
        .outputfile    = NULL,           // char *
        .outputfile_FP = stdout,         // FILE *
        .dbfile        = NULL,           // char *
        .workdir       = NULL,           // char *
        .pid           = (long)getpid(), // long int
        .colorful      = 0               // int
    };

    /* Parse command-line parameters */
    if (PERFEXPERT_SUCCESS != parse_cli_params(argc, argv)) {
        OUTPUT(("%s", _ERROR("Error: parsing command line arguments")));
        return PERFEXPERT_ERROR;
    }

    /* Create the list of fragments */
    fragments = (perfexpert_list_t *)malloc(sizeof(perfexpert_list_t));
    if (NULL == fragments) {
        OUTPUT(("%s", _ERROR("Error: out of memory")));
        return PERFEXPERT_ERROR;
    }
    perfexpert_list_construct(fragments);

    /* Open input file */
    if (NULL != globals.inputfile) {
        if (NULL == (globals.inputfile_FP = fopen(globals.inputfile, "r"))) {
            OUTPUT(("%s (%s)", _ERROR("Error: unable to open input file"),
                globals.inputfile));
            return PERFEXPERT_ERROR;
        }
    }

    /* Print to a file or STDOUT is? */
    if (NULL != globals.outputfile) {
        OUTPUT_VERBOSE((7, "printing recommendations to file (%s)",
            globals.outputfile));
        globals.outputfile_FP = fopen(globals.outputfile, "w+");
        if (NULL == globals.outputfile_FP) {
            OUTPUT(("%s (%s)", _ERROR("Error: unable to open output file"),
                globals.outputfile));
            rc = PERFEXPERT_ERROR;
            goto cleanup;
        }
    } else {
        OUTPUT_VERBOSE((7, "printing recommendations to STDOUT"));
    }

    /* Connect to database */
    if (PERFEXPERT_SUCCESS != perfexpert_database_connect(&(globals.db),
        globals.dbfile)) {
        OUTPUT(("%s", _ERROR("Error: connecting to database")));
        rc = PERFEXPERT_ERROR;
        goto cleanup;
    }

    /* Parse input parameters */
    if (PERFEXPERT_SUCCESS != parse_transformation_params(fragments)) {
        OUTPUT(("%s", _ERROR("Error: parsing input params")));
        rc = PERFEXPERT_ERROR;
        goto cleanup;
    }

    /* Select transformations for each code bottleneck */
    fragment = (fragment_t *)perfexpert_list_get_first(fragments);
    while ((perfexpert_list_item_t *)fragment != &(fragments->sentinel)) {
        /* Query DB for transformations */
        if (PERFEXPERT_SUCCESS != select_transformations(fragment)) {
            OUTPUT(("%s", _ERROR("Error: applying transformation")));
            rc = PERFEXPERT_ERROR;
            goto cleanup;
        }

        /* Apply recommendation (actually only one will be applied) */
        switch (apply_recommendations(fragment)) {
            case PERFEXPERT_NO_TRANS:
                OUTPUT(("%s", _GREEN("Sorry, we have no transformations")));
                goto move_on;
                continue;

            case PERFEXPERT_SUCCESS:
                rc = PERFEXPERT_SUCCESS;
                break;

            case PERFEXPERT_ERROR:
            default: 
                OUTPUT(("%s", _ERROR("Error: selecting transformations")));
                rc = PERFEXPERT_ERROR;
                goto cleanup;
        }

        /* Move to the next code bottleneck */
        move_on:
        fragment = (fragment_t *)perfexpert_list_get_next(fragment);
    }

    cleanup:
    /* Close files and DB connection */
    if (NULL != globals.inputfile) {
        fclose(globals.inputfile_FP);
    }
    if (NULL != globals.outputfile) {
        fclose(globals.outputfile_FP);
    }
    perfexpert_database_disconnect(globals.db);

    /* TODO: Free memory */

    return rc;
}

#ifdef __cplusplus
}
#endif

// EOF