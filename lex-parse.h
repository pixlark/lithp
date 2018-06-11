#ifndef LITHP_LEX_PARSE_H
#define LITHP_LEX_PARSE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "vm.h"
#include "ds_util.h"

Cell * parse_source(Lisp_VM * vm, char * source);

#endif
