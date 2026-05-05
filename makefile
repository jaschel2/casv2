# ----------------------------
# MAT267 CAS for TI-84 Plus CE
# ----------------------------

NAME = MAT267CAS
DESCRIPTION = "MAT267 CAS Calculator"

# Source files — toolchain auto-finds all .c in src/
CFLAGS = -Wall -Os

# Include the toolchain's master makefile
include $(shell cedev-config --makefile)
