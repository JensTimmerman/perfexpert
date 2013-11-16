
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

#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#include "generic_defs.h"

class argparse {
    public:
        static bool copy_file(const char *source_file,
                const char *destination_file);
        static bool parse_location(std::string& argument,
                std::string& function_name, int& line_number);
        static int parse_arguments(char* arg, options_t& options);
};

#endif  /* ARGPARSE_H_ */
