//
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Shared/fdelt_chk.c#1 $
//
// This file comes from the libc source for version 2.15+:
// (e.g. http://www.eglibc.org/cgi-bin/viewvc.cgi/trunk/libc/debug/fdelt_chk.c)
// It is included here to fulfill the missing definition of __fdelt_chk
// from the FD_* macros included by <sys/select.h> when FORTIFY_SOURCE
// is enabled (which it is when our Ubuntu build VMs [libc 2.19] build the code)
// when run on operating systems that use libc < 2.15 (e.g. Debian Wheezy and Raspbian).
//

/* Copyright (C) 2011-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <sys/select.h>


long int
__fdelt_chk (long int d)
{
  if (d < 0 || d >= FD_SETSIZE)
    __chk_fail ();

  return d / __NFDBITS;
}
//strong_alias (__fdelt_chk, __fdelt_warn)
