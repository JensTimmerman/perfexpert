#
# Copyright (c) 2011-2013  University of Texas at Austin. All rights reserved.
#
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# This file is part of PerfExpert.
#
# PerfExpert is free software: you can redistribute it and/or modify it under
# the terms of the The University of Texas at Austin Research License
# 
# PerfExpert is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.
# 
# Authors: Leonardo Fialho and Ashay Rane
#
# $HEADER$
#

lib_LIBRARIES = libmacpo.a

libmacpo_a_SOURCES = libmacpo.cc argparse.cpp minst.cpp instrumentor.cpp \
                streams.cpp aligncheck.cpp ir_methods.cpp loop_traversal.cpp \
                tracer.cpp generic_vars.cpp vector_strides.cpp \
                analysis_profile.cpp tripcount.cpp branchpath.cpp \
                pntr_overlap.cpp stride_check.cpp traversal.cpp reuse_dist.cpp

libmacpo_a_CXXFLAGS = -I. -Wno-deprecated -I$(srcdir)/include \
                -I$(srcdir)/../common

include_HEADERS = libmacpo.h
