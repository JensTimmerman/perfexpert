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
#include <string.h>

/* Utility headers */
#include <matheval.h>

/* Modules headers */
#include "lcpi.h"
#include "lcpi_types.h"

/* PerfExpert common headers */
#include "common/perfexpert_alloc.h"
#include "common/perfexpert_constants.h"
#include "common/perfexpert_hash.h"
#include "common/perfexpert_list.h"
#include "common/perfexpert_output.h"

/* logic_lcpi_compute */
int logic_lcpi_compute(lcpi_profile_t *profile) {
    lcpi_metric_t *h_lcpi = NULL, *l = NULL, *t = NULL;
    lcpi_hotspot_t *h = NULL;
    double *values = NULL;
    char **names = NULL;
    int count = 0, i = 0;

    OUTPUT_VERBOSE((4, "%s", _YELLOW("Calculating LCPI metrics")));

    /* For each hotspot in this profile... */
    perfexpert_list_for(h, &(profile->hotspots), lcpi_hotspot_t) {
        OUTPUT_VERBOSE((8, "  %s", _YELLOW(h->name)));

        /* For each LCPI definition... */
        perfexpert_hash_iter_str(my_module_globals.metrics_by_name, l, t) {
            PERFEXPERT_ALLOC(lcpi_metric_t, h_lcpi, sizeof(lcpi_metric_t));
            strcpy(h_lcpi->name_md5, l->name_md5);
            h_lcpi->expression = l->expression;
            h_lcpi->value = l->value;
            h_lcpi->name = l->name;

            /* Get the list of variables and their values */
            evaluator_get_variables(h_lcpi->expression, &names, &count);
            if (0 < count) {
                PERFEXPERT_ALLOC(double, values, (sizeof(double *) * count));

                for (i = 0; i < count; i++) {
                    if (-1.0 != database_get_hound(names[i])) {
                        values[i] = database_get_hound(names[i]);
                    } else if (-1.0 != database_get_event(names[i],
                        my_module_globals.hpctoolkit->name, h->id)) {
                        values[i] = database_get_event(names[i],
                            my_module_globals.hpctoolkit->name, h->id);
                    }
                }

                /* Evaluate the LCPI expression */
                h_lcpi->value = evaluator_evaluate(h_lcpi->expression, count,
                    names, values);
                PERFEXPERT_DEALLOC(values);
            }

            /* Add the LCPI to the hotspot's list of LCPIs */
            perfexpert_hash_add_str(h->metrics_by_name, name_md5, h_lcpi);

            OUTPUT_VERBOSE((10, "    %s=[%g]", h_lcpi->name, h_lcpi->value));
        }
    }
    return PERFEXPERT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

// EOF
