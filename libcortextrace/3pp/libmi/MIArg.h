/******************************************************************************
 * Copyright (c) 2005 The Regents of the University of California.
 * This material was produced under U.S. Government contract W-7405-ENG-36
 * for Los Alamos National Laboratory, which is operated by the University
 * of California for the U.S. Department of Energy. The U.S. Government has
 * rights to use, reproduce, and distribute this software. NEITHER THE
 * GOVERNMENT NOR THE UNIVERSITY MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. If software is modified
 * to produce derivative works, such modified software should be clearly  
 * marked, so as not to confuse it with the version available from LANL.
 *
 * Additionally, this program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * LA-CC 04-115
 *
 * Copyright 2011-2014 Cray Inc. All Rights Reserved.
 *
 ******************************************************************************/
 
 /*
  * Based on the QNX Java implementation of the MI interface
  */

#ifndef _MIARG_H_
#define _MIARG_H_

#include "MIList.h"
#include "MIString.h"

/**
 * Represents a set of name, type, value.
 */
struct MIArg {
	char *  name;
	char *  type;
	char *  value;
};
typedef struct MIArg	MIArg;

extern MIArg *MIArgNew(void);
extern void MIArgFree(MIArg *arg);
extern MIList *MIArgsParse(MIValue *miValue);
extern MIArg *MIArgParse(MIValue *tuple);
extern MIString *MIArgToString(MIArg *arg);
#endif /* _MIARG_H_ */
