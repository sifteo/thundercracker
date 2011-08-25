/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_H
#define _FRONTEND_H

void frontend_init(struct em8051 *cpu);
void frontend_exit(void);

int frontend_loop(void);
void frontend_async_quit(void);

#endif
