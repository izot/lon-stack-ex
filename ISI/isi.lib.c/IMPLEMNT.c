//	Implemnt.c	implementing _IsiImplementEnrollment
//
// Copyright © 2005-2022 Dialog Semiconductor
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//	Revision 0, February 2005, Bernd Gauweiler

#include "isi_int.h"

#ifdef	ISI_SUPPORT_ALIAS
	#define SetBound(v) Bound=v
#else
	#define SetBound(v)
#endif	//	ISI_SUPPORT_ALIAS

static void Bind(unsigned Assembly, unsigned Offset, unsigned Extend, unsigned Address, LonWord Selector, LonBool considerTurnaround)
{
	unsigned NvIndex;
#ifdef	ISI_SUPPORT_ALIAS
	LonBool Bound = FALSE;
	unsigned int AliasIndex;
#endif	//	ISI_SUPPORT_ALIAS
#ifdef  ISI_SUPPORT_TURNAROUNDS
    LonBool turnAround;
#endif  //  ISI_SUPPORT_TURNAROUNDS
	LonAliasEcsConfig Alias;			// note we use the alias_struct for both Nvs and aliases, and need this variable even if alias support is diasabled.

    if (Assembly != ISI_NO_ASSEMBLY)
    {
    	NvIndex = IsiGetNvIndex(Assembly, Offset, ISI_NO_INDEX);
    	while (NvIndex != ISI_NO_INDEX)
        {
    		// notice we use alias_struct Alias here just to reduce the number of local bytes on the stack;
    		// alias struct is a superset of nv_struct, so we can use the larger structure instead of both:
    		Alias.Alias = *IsiGetNv(NvIndex);
    		SetBound(FALSE);

#ifdef  ISI_SUPPORT_TURNAROUNDS
            turnAround = considerTurnaround && LON_GET_ATTRIBUTE(Alias.Alias,LON_NV_ECS_DIRECTION);   //.alias_nv.nv_direction;
#   define  TURNAROUND  turnAround
#else
#   define  TURNAROUND  FALSE
#endif  //  ISI_SUPPORT_TURNAROUNDS
    		if (!Extend || (LON_GET_ATTRIBUTE(Alias.Alias,LON_NV_ECS_SELHIGH) > 0x2Fu))
            {
    			// we need to replace the previous binding, or the NV is not bound at all:

    			_IsiAPIDebug(
    				"Binding assembly %u: NV %u Sel $%02X%02X\n",
    				Assembly, Offset,
    				NvIndex,
    				(unsigned)Selector.msb, (unsigned)Selector.lsb
    			);

    			_IsiBind(Alias.Alias, Address, Selector, TURNAROUND);
    			IsiSetNv(&Alias.Alias, NvIndex);
    			SetBound(TRUE);
    		}
#ifdef	ISI_SUPPORT_ALIAS
    		for(AliasIndex=0; AliasIndex < AliasCount; ++AliasIndex)
            {
    			Alias = *IsiGetAlias(AliasIndex);
    			if (!Extend && Bound && (LON_GET_UNSIGNED_WORD(Alias.Primary) == NvIndex))
                {
    				// free this alias table entry:
    				memset(&Alias, 0xFFu, (unsigned)sizeof(Alias));
    				goto SetAlias;	// Saves a few bytes
    			}
                else if (!Bound && (LON_GET_UNSIGNED_WORD(Alias.Primary) == ISI_ALIAS_UNUSED))
                {
    				// link this alias to the primary and bind it:
    				LON_SET_UNSIGNED_WORD(Alias.Primary, NvIndex);
    				memcpy(&Alias.Alias, IsiGetNv(NvIndex), sizeof(Alias.Alias));
    				_IsiBind(Alias.Alias, Address, Selector, TURNAROUND);
    //				Alias.alias_nv.nv_direction = IsiGetNv(NvIndex)->nv_direction;
    				Bound = TRUE;
SetAlias:
    				IsiSetAlias(&Alias, AliasIndex);
    			}
    		}
#endif	//	ISI_SUPPORT_ALIAS
    		NvIndex = IsiGetNvIndex(Assembly, Offset, NvIndex);
    	}
        //	EPR 36750: issue isiImplemented event instead of isiNormal
#ifdef  ISI_SUPPORT_ALIAS
        if (Bound)
#endif  // ISI_SUPPORT_ALIAS
        IsiUpdateUserInterface(isiImplemented, Assembly);
    }   //  ISI_NO_ASSEMBLY
#if 0
#pragma ignore_notused  considerTurnaround
#endif
}

void _IsiImplementEnrollment(LonBool Extend, LonByte Assembly)
{
	//	1. Find the first matching connection table entry. If this signals "host", issue a CSMC message.
	//	2. Schedule for the CSMC message to be resend once more (?), at ISI_T_CSMC (==ISI_T_CSME)
	//	3. Make sure we belong to this group already, or if not, we join the group.	The group is held in _isiVolatile.Group.
	//	   Remember the resulting address table index for use with step 4.
	//	4. Use IsiGetLocalNvIndex to get the NV indices, one by one. For each of these NVs, do this:
	//		4.a	If the Extend flag is not set, disassociate all aliases from this NV (if any)
	//		4.b If the Extend flag is not set OR the primary NV is unbound, update the nv_struct
	//			Else, find a free alias table entry, associate this with the primary and update the alias.
	//	5. Change connection status to InUse
	//	6. Change ISI engine status to Normal / Idle
	//	7. Update User Interface to Normal
	unsigned Connection, Offset, Index, Address;
	LonWord Selector;
    LonByte ConnectionDataWidth;
	IsiConnection ConnectionData;

#ifdef  ISI_SUPPORT_MANUAL_CONNECTIONS
	unsigned Csmc;
    Csmc = ISI_NO_INDEX;
#endif	//	ISI_SUPPORT_MANUAL_CONNECTIONS

#ifdef	ISI_SUPPORT_CONNECTION_REMOVAL
	if (!Extend) {
		// if this implements a replacement connection, we must make sure to undo the one that we are to
		// replace. This will only handle connections in the InUse state, thus preserving the connection
		// table entry that was set up earlier for the replacing connection (which at this point still is
		// InPending state). Notice the front-end functions IsiLeaveEnrollemnt and IsiDeleteEnrollment
		// verify the correct state (isiNormal); we must use the internal versions _IsiLeaveEnrollment
		// or _IsiDeleteEnrollment, respectively, instead:
		_IsiRemoveConnection(_isiVolatile.State, Assembly, FALSE);
	}
#endif

	Address = _IsiIsGroupAcceptable(_isiVolatile.Group, TRUE);

    Offset = 0;
	for (Connection = _isiVolatile.pendingConnection; _IsiNextConnection(Connection, &ConnectionData); ++Connection)
    {
		if ((ConnectionData.Host == Assembly || ConnectionData.Member == Assembly) && LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_STATE) == isiConnectionStatePending)
        {
    		Selector = ConnectionData.Header.Selector;

#ifdef	ISI_SUPPORT_MANUAL_CONNECTIONS
    		// do we need to issue a CSMC message for this?	We could send a CSME message for the first applicable connection table entry (if hosted).
    		// We can also send a CSME message for each connection table entry that has the HaveCsme flag set: this flag is only set on the host, and
    		// is only set with the first matching connection table entry anyhow...
    		// We keep the connection table index in the Csmc variable - once we have processed all code that is needed to implement this connection,
    		// we use this index to re-send the CSMC message once more just for good measure, in the hope that the second one might help reaching all
    		// guests.
    		if (LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_CSME) && Csmc == ISI_NO_INDEX)
            {
    			Csmc = Connection;
    			_IsiSendCsmX(&ConnectionData, isiCsmc, 3);
    		}
#endif	//	ISI_SUPPORT_MANUAL_CONNECTIONS

    		// now look into all NVs (=selectors) that are goverened by this connection table entry:
            ConnectionDataWidth = LON_GET_ATTRIBUTE(ConnectionData,ISI_CONN_WIDTH);
    		for(Index = 0; Index < ConnectionDataWidth; ++Index, ++Offset)
            {
    			Bind(ConnectionData.Host, Offset, Extend, Address, Selector, ConnectionData.Host != ISI_NO_ASSEMBLY && ConnectionData.Member != ISI_NO_ASSEMBLY);
                Bind(ConnectionData.Member, Offset, Extend, Address, Selector, ConnectionData.Host != ISI_NO_ASSEMBLY && ConnectionData.Member != ISI_NO_ASSEMBLY);
    			Selector = _IsiIncrementSelector(Selector);
    		}

    		//	Mark this connection table entry as used and update the connection table:
    		LON_SET_ATTRIBUTE(ConnectionData,ISI_CONN_STATE,isiConnectionStateInUse);
    		IsiSetConnection(&ConnectionData, Connection);

            if (ConnectionData.Host != ISI_NO_ASSEMBLY) {
                // on the host, issue an immediate CSMI. The intention is to avoiding NV leaks: waiting for the next regular CSMI could take some while,
                // during which another, previously existing, connection might be unaware of the new connection and a possible selector conflict. Issuing
                // a CSMI right now increases the chances of that other connection to become aware of the conflict sooner, and to re-allocate its selectors,
                // if and as needed.
                //
                // EPR 38585: this used to be down at the bottom and would only CSMI for that conntab entry that also gets used with the CSMC.
                // However, it is true that the CSMC needs only sending once, where the CSMI needs sending for each entry.
                _IsiSendCsmi(&ConnectionData);
            }

        }   // if applicable connection table entry

	}	// iterate connection table

#ifdef	ISI_SUPPORT_MANUAL_CONNECTIONS
	//	The Csmc variable is set to the first connection table index, or to ISI_NO_INDEX. In the latter case, do nothing.
	//	The variable is set to a good index on the host device; issue a second CSMC message (just to increase the chances of
	//	getting through), and an immediage CSMI (reducing possible NV leaks):
	if (Csmc != ISI_NO_INDEX) {
		const IsiConnection* pConnection;
		pConnection = IsiGetConnection(Csmc);

		// resend the CSMC once more
		_IsiSendCsmX(pConnection, isiCsmc, 3);
	}
#endif	//	ISI_SUPPORT_MANUAL_CONNECTIONS

    //	update the user interface and return the ISI engine state to normal.
    // 21-Sept-2005, BG: We used to fire the isiImplemented event here. With ISI 3 and support for turnarounds, we
    // prefer firing the same event from the Bind() routine, as this conventiently covers both the host and member
    // assemblies without extra logic. And because both assemblies are covered, we can just clear the state and need
    // not preseving host or guest state flags: if the local host implements the connection, it does not internally
    // fake the receipt of a CSMC message to implement the member. Instead, both members and hosts are always
    // implemented at the same time (in above routine), saving the logic for internally turned-around CSMC.
    _isiVolatile.State = isiStateNormal;

    savePersistentData(IsiNvdSegConnectionTable);
}

//	end of Implemnt.c
