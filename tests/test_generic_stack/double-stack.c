/**
 * double-stack.c
 *   A simple implementation of stacks of doubles.
 *
 * <insert MIT license>
 */

#include "double-stack.h"
#define TYPE double
#define TYPED(THING) Double##THING
#include "generic-stack.c"