/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#ifndef _SYSCALLNR_H
#define _SYSCALLNR_H

#define SYSCALL_task_exit     1
#define SYSCALL_task_create   2
#define SYSCALL_task_getid    3
#define SYSCALL_task_yield    4
#define SYSCALL_task_wait     5

#define SYSCALL_beep          6
#define SYSCALL_vm86      182

#define SYSCALL_putchar       1000
#define SYSCALL_getchar       1001

#endif /*_SYSCALLNR_H*/