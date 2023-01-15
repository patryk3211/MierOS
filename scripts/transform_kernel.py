#!/usr/bin/python
## Usage:
## transform_kernel.py <kernel_file_name>

import sys
import lief

image = lief.parse(sys.argv[1])

image.header.file_type = lief.ELF.E_TYPE.DYNAMIC

for ent in image.dynamic_entries:
    if ent.tag == lief.ELF.DYNAMIC_TAGS.FLAGS_1:
        ent.remove(lief.ELF.DYNAMIC_FLAGS_1.PIE)
        #if ent.value == 0:
        #    image.remove(ent)
        break

image.write(sys.argv[1])
