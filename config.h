/*
 *  ff_config.h
 *  Perian
 *
 *  Created by Alexander Strange on 3/14/08.
 *  Copyright 2008 Alexander Strange. All rights reserved.
 *
 */

#if (defined(__ppc__) || defined(__ppc64__))
#include "ppc/config.h"
#elif (defined (__i386__) || defined( __x86_64__ ))
#include "intel/config.h"
#endif
