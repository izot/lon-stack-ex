/****************************************************************
 *  lon.h
 *
 * Copyright © 1990-2022 Dialog Semiconductor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *  This file contains the data structures, typedefs, and
 *  literals common to all subsystems of the LON(R) Applications
 *  Programming Interface
 *
 ****************************************************************/

#ifndef LON_H
#define LON_H

#define LON_NEURON_ID_LEN        6      /* # of bytes in neuron id           */
#define LON_DOMAIN_ID_LEN        6      /* Max # of bytes in domain id       */
#define LON_KEY_LEN              6      /* # of bytes in authentication key  */
#define LON_PROG_ID_LEN          8      /* # of bytes in neuron program id   */
#define LON_LOCN_LEN             6      /* # of bytes in neuron location     */

#define LON_MAX_SUBNET          255     /* Maximum value for subnet num */
#define LON_MAX_GROUP           255     /* Maximum value for group num  */
#define LON_MAX_GROUP_MEMBER    127     /* Maximum value for group membership */
#define LON_MAX_NODE_ID         127     /* Maximum value of node_num field. */
#define LON_MAX_CHAN_ID         0x3fff  /* Maximum system-wide channel ID   */
#define LON_MAX_ROUTER_ID       0x3fff  /* Maximum system-wide router ID    */
#define LON_MAX_NV_SELECTOR     0x3fff  /* Max value of Netvar id       */
#define LON_MAX_NV_LEN          31      /* Max # bytes in NV value      */


typedef BYTE    NeuronId[LON_NEURON_ID_LEN];
typedef BYTE    ProgramId[LON_PROG_ID_LEN];

typedef struct
{
	BYTE nid[LON_NEURON_ID_LEN];
} LonNeuronId;

typedef struct
{
	BYTE pid[LON_PROG_ID_LEN];
} LonProgramId;


#endif

