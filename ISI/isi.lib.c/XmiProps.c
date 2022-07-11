//	XmiProps.c	implementing _IsiSelectTransportProps
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
//	revision 0, January 2005, Bernd Gauweiler
//	revision 1, June 2005, Bernd Gauweiler: EPR 37197, review of transport properties desired


#include "isi_int.h"
#include "FtxlTypes.h"
//#include <mem.h>

//	8-June-2005, Bernd:
//
//	Transport properties are now being set as follows:
//	FT channel: repeat timer 16ms, tx timer  48ms, group- and non-group-receive timer both 128ms
//	PL channel: repeat timer 16ms, tx timer 192ms, group-receive timer 3072ms, non-group receive timer 768ms
//
//	Brief summary how we arrive at these numbers:
//	FT:	"as fast as possible" seemed to have been the argument here. The Tx Timer is taken from NSS and more or less
//	irrelevant for ISI anyhow, so that's not critical
//	PL: Bob, Bob, Phil and Glen agreed that minimum spacing is OK -> repeat timer of 16ms. The TX Timer is taken from NSS
//	and mostly irrelevant for ISI. The repeat timers are more interesting. The group-receive timer covers 4 maximum-sized ISI
//	packets send from a node that uses an energy-storage power supply on the secondary frequency using 6-byte domain ID
//  (with the domain ID being a Neuron ID), using group addressing or bcast, repeated service, no authentication, no priority,
//	no req/resp, no ACKD service, which take 94.43ms each to transmit, and allows for 3 recharging cycles where the power-supply
//	recharges before the next packet goes out (850ms under worst case conditions).
//	The non-group receive timer makes the same assumptions but uses typical rather then worst-case recharging times of 53ms.
//	The idea was to set the group-receive timer so that duplication detection continues to work for network variables, typical
//	ISI service types and addressing modes assumed. Duplication detection for ISI broadcasts is a nice-to have, but making this
//	timer too long would probably cause a loss of incoming messages at some point due to lack of receive transaction database
//	space. We calculate the non-group receive timer to typical conditions therefore, knowing that ISI messages are not sentitive
//	to duplicates.

static const IsiTransport transport[] = {
	// FT:
	{	(0<<4),					// repeat timer, encoded (and pre-shifted into normal address table location)
		3,						// Tx timer, encoded
		0,						// group receive timer, encoded
		0, 						// non-group receive timer, encoded
		ISI_SUBNET_START_TPFT,	// base subnet
        5u * ISI_TICKS_PER_SECOND,    // width of the broadcast slot in ticks, based on 1.5% bandwidth consumption of 180pkt/s total bandwidth
        ISI_TICKS_PER_SECOND    // width of the spreading interval. Must be at least 4 ticks, because the broadcast has a jitter of +- 1 tick already.
	},
	// PL:
	{	(0<<4),					// repeat timer, encoded (and pre-shifted into normal address table location)
		7,						// Tx timer, encoded
		9,						// group receive timer, encoded
		5, 						// non-group receive timer, encoded
		ISI_SUBNET_START_PL20,	// base subnet
        10u * ISI_TICKS_PER_SECOND,  // width of the broadcast slot in ticks, based on 1.5% bandwidth consumption of 14pkt/s total bandwidth
        (3u*ISI_TICKS_PER_SECOND)/2u    // width of the spreading interval. Must be at least 4 ticks, because the broadcast has a jitter of +- 1 tick already.
	}
};

void _IsiSelectTransportProps(IsiChannelType Channel)
{
	const IsiTransport* pTransport;

	if (Channel == isiChannelIp852 || Channel == isiChannelIzotIp || Channel == isiChannelTpFt) {
		pTransport = &transport[0];
	} else {
		// assume powerline:
		pTransport = &transport[1];
	}

	// determine the set of transport parameters based on the channel type
	if (memcmp(&_isiVolatile.Transport, pTransport, (unsigned)sizeof(IsiTransport))) {
		memcpy(&_isiVolatile.Transport, pTransport, (unsigned)sizeof(IsiTransport));

		// parameters have changed. Update the parameter pointer, and update all used address table records:
#ifdef	ISI_SUPPORT_TIMG
		{
			unsigned Index;
			LonAddress Address;

			// without dynamic device count support, we don't process TIMG messages (as ISI-S is limited to a single channel type anyhow,
			// so the whole and sole purpose of processing TIMG was to receive device count estimates). Since we don't process TIMG in this
			// compact flavor, there won't be an update on the channel type, and no need to update the timers in address table and config
			// structure on the fly.
            unsigned int address_count = _address_table_count();     // getting the value from the stack, instead of from the read_only_data
        	for (Index=0; Index < address_count; ++Index)
            {
				Address = *access_address(Index);
                if (LON_GET_ATTRIBUTE(Address.Group,LON_ADDRESS_GROUP_TYPE))
                {
					// this is a group address. We care about those (and not about anything else)
					LON_SET_ATTRIBUTE(Address.Group,LON_ADDRESS_GROUP_RECEIVE_TIMER,_isiVolatile.Transport.GroupRcvTimer);
					LON_SET_ATTRIBUTE(Address.Group,LON_ADDRESS_GROUP_REPEAT_TIMER,(_isiVolatile.Transport.RepeatTimer)>>4);
					LON_SET_ATTRIBUTE(Address.Group,LON_ADDRESS_GROUP_TRANSMIT_TIMER,_isiVolatile.Transport.TransmitTimer);
					update_address((const LonAddress*)&Address, Index);
				}
			}
		}
#endif	//	ISI_SUPPORT_TIMG
	}

	// check to see if the non-group receive timer needs updating, too.
	// This needs to be done in any case, so that we are sure to set the correct non-grp rcv timer at power-up.
	if (LON_GET_ATTRIBUTE(config_data,LON_CONFIG_NONGRPRCV) != (_isiVolatile.Transport.NonGroupTimer))
    {
		LonConfigData Config;
		memcpy(&Config, &config_data, (unsigned)sizeof(LonConfigData));
		LON_SET_ATTRIBUTE(Config,LON_CONFIG_NONGRPRCV,_isiVolatile.Transport.NonGroupTimer);
		update_config_data(&Config);
	}
}

