//	TickS.c	implementing IsiTickS
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

//	Note the implementation of IsiTicksS() makes liberal use of "goto" - this is not so much for code maintainability or beauty,
//	but results in significantly smaller code than alternatives. Tough luck.

// Note that IsiTickDas is an almost identical copy of IsiTicks - not identical, but showing large similarities, especially in the
// broadcast scheduler. Code is forked between the two to allow for a minimum footprint on ISI-S devices; changes to IsiTickS must
// be made with care and with IsiTickDas in mind - the same change might also be required to IsiTickDas.
// Also note that IsiTickDa calls into IsiTickS, so there is some dependency there, too.

static LonBool AdvanceCIdx(void) {
	LonBool b;
	b = ++_isiVolatile.Periodic.lastConnectionIdx >= _isiVolatile.ConnectionTableSize;
	if (b)
		_isiVolatile.Periodic.lastConnectionIdx = 0;
	return b;
}

void IsiTickS(void) {
	IsiMessageCode Code;
	const IsiConnection* pConnection;

	if (_isiVolatile.Running) {

		// Bump up the spreading counter. This is reset with every recognized incoming message,
		// and is a measure for the time between the last receipt of a recognized message, and
		// broadcasting a message from this device. It this time is too short, we broadcast
		// anyway, but re-scramble the periodic broadcasting slot for this device. The intention
		// is to cause more evenly distributed periodic broadcasts.
		_isiVolatile.Spreading = min(250u, _isiVolatile.Spreading+1);

		// Bumb up the startup counter. This is reset during initialization, and stops when
		// reaching 0xFFFE (65,534). The startup counter is used to schedule things that will,
		// or might, happen a certain time after startup. These events include the issue of
		// CSMR reminders during the periodic broadcast (Startup must be >= ISI_T_CSMR), and
		// the acceptance of IsiInitiateAutoEnrolment (Startup must be >= ISI_T_AUTO)

        // EPR 39173 isiWarm should occur only if the engine is idle. The easiest fix is to pause the startup
        // timer if the engine is not idle. This would be the case if the device is already engaged in some
        // enrollment business, or if the engine is in the middle of (trying to) get a unique domain ID.
        // Firing the Warm event even if not idle is not a bad thing in itself, but applications are likely
        // to call IsiInitiateAutoEnrollment() in response to isiWarm - the automatic enrollment can only
        // be initiated when the engine is idle, so short of putting the burden onto the application we can
        // ensure this condition for them:
        if (!_isiVolatile.State) {
    		if (_isiVolatile.Startup < 0xFFFFul) {
    			++_isiVolatile.Startup;
            }
		}

		// notify the application if the warmup phase is complete (Tauto). This tells the
		// application that it is now OK to issue calls to IsiInitiateAutoEnrolment:
		if (_isiVolatile.Startup == ISI_T_AUTO) {
			IsiUpdateUserInterface(isiWarm, 0);
		}

        // decrement the timeout time, regardless of the current state. Note
        // IsiTickDa and IsiTickDas rely on this decrement being done here.
		if (_isiVolatile.Timeout) {
			--_isiVolatile.Timeout;
		}

#ifdef	ISI_SUPPORT_MANUAL_CONNECTIONS
		if (_isiVolatile.State & HOST_STATES) {
			if (_isiVolatile.Timeout == 1ul) {
				_IsiSendCsmx();
				goto HandleCancellation;
			} else if (_isiVolatile.ShortTimer == 1u) {
				// short timer expired. In this state, this means CSMO re-send:
				_IsiResendCsmo();
				_isiVolatile.ShortTimer = (1u + ISI_T_CSMO);
			}
		}
#endif	//	ISI_SUPPORT_MANUAL_CONNECTIONS

		if (_isiVolatile.State & isiStateAccepted) {
			if (_isiVolatile.ShortTimer == 1u) {
				_IsiSendCsme();
			}
			goto HandleCancellation;
		}
		else if (_isiVolatile.State & isiStateInvited) {
HandleCancellation:
			if (_isiVolatile.Timeout == 1ul) {
				_IsiUpdateUi(isiCancelled);
				_isiVolatile.ShortTimer = _isiVolatile.State = 0;
			}
		}

        // decrement the short time, regardless of the current state. Note
        // IsiTickDa and IsiTickDas rely on this decrement being done here.
		if (_isiVolatile.ShortTimer) {
			--_isiVolatile.ShortTimer;
        }

        //  --------------
        //  Are we delivering a premature DRUM?
        //  --------------
        if (_isiVolatile.specialDrum-- == 1ul) {
            _IsiSendDrum();
        }

        //  --------------
        //  Following is the broadcast scheduler:
        //  --------------

		if (_isiVolatile.Wait-- == 0) {
			//	the wait has come to an end
			// retrigger slot wait timer
			// For small values of spreading, we've only just seen an incoming message a short while ago,
			// so let's give way by reallocating the slot
			// For large values, it has been quiet for a while. Keep the slot and wait our time
			_isiVolatile.Wait = (_isiVolatile.Spreading < _isiVolatile.Transport.SpreadingInterval) ? _IsiAllocSlot(PersistDevices) : _IsiGetPeriod(PersistDevices);

#ifdef ISI_SUPPORT_DIAGNOSTICS
			if (_isiVolatile.Spreading < _isiVolatile.Transport.SpreadingInterval) {
				_IsiConditionalDiagnostics(isiReallocateSlot, 0);
			}
#endif
			// figure out what to do with this periodic broadcast slot.
			//
			// We need to send DRUM whenever there is nothing better to do, and at least every eigth broadcast.
			// We need to send HVNB (one per slot) if enabled
			// We need to send CSMR (one per slot), if any	- notice this is also controlled by the ISI_T_CSMR startup timer
			// We need to send CSMI (one per slot), if any
			// Finally, if supported and enabled, we need to allow for application periodics, if any.
            pConnection = IsiGetConnection(_isiVolatile.Periodic.lastConnectionIdx);
			while (LON_GET_ATTRIBUTE_P(pConnection,ISI_CONN_STATE) < isiConnectionStateInUse)
            {
				if (AdvanceCIdx())
                {
					// there is nothing for us to do. Jump to periodic messages that are not connection-releated:
#ifdef	ISI_SUPPORT_HEARTBEATS
					_isiVolatile.Periodic.slotUsage = slotNvHb;
#else	// no heartbeats:
					_isiVolatile.Periodic.slotUsage = slotAppl;
#endif	// ISI_SUPPORT_HEARTBEATS
					break;
				}
			}

			if (_isiVolatile.Periodic.drumPause++) {
#ifdef	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
				if (_isiVolatile.Periodic.slotUsage == slotCsmr) {
					_isiVolatile.Periodic.slotUsage = slotCsmi;
					// the following line is the same as Host && Auto && !Offset:
                    if (pConnection->Desc.OffsetAuto == ConnectionAuto_MASK && pConnection->Host != ISI_NO_ASSEMBLY && _isiVolatile.Startup > ISI_T_CSMR) {
						Code = isiCsmr;
						_IsiSendCsmr(_isiVolatile.Periodic.lastConnectionIdx, pConnection);
						goto SchedulerEndWithNotification;
						// note we do not forward the connection index in this case, as this entry needs
						// reviewed one more time for sending of CSMI messages
					}
				}
#endif	//	ISI_SUPPORT_AUTOMATIC_CONNECTIONS
				if (_isiVolatile.Periodic.slotUsage == slotCsmi) {
					// advance to the next connection table entry in any case, whether a CSMI is due for the current entry or not.
					(void)AdvanceCIdx();

					// whether we CSMI the current entry or not: the next cycle shouldn't start with CSMI. Advance to the next state
					// in any case, so that other periodic messages have a chance of getting delivered and the system won't be choked by
					// numerous CSMI:
#ifdef	ISI_SUPPORT_HEARTBEATS
					_isiVolatile.Periodic.slotUsage = slotNvHb;
#else	// no heartbeats:
					_isiVolatile.Periodic.slotUsage = slotAppl;
#endif	// ISI_SUPPORT_HEARTBEATS

					// see if we need to CSMI the current entry:
					if (pConnection->Host != ISI_NO_ASSEMBLY) {
						Code = isiCsmi;
						_IsiSendCsmi(pConnection);
						goto SchedulerEndWithNotification;
					}
				}

#ifdef ISI_SUPPORT_HEARTBEATS
				if (_isiVolatile.Periodic.slotUsage == slotNvHb) {
					_isiVolatile.Periodic.slotUsage = slotAppl;
					if (_IsiSendNvHb()) {
						goto SchedulerEnd;
					}
				}
#endif	// ISI_SUPPORT_HEARTBEATS

				if (_isiVolatile.Periodic.slotUsage == slotAppl) {
					_isiVolatile.Periodic.slotUsage = slotCsmr;
					if ((_isiVolatile.Flags & isiFlagApplicationPeriodic) && IsiCreatePeriodicMsg()) {
						goto SchedulerEnd;
					}
				}
			}
			Code = isiDrum;
			_IsiSendDrum();
			_isiVolatile.Periodic.drumPause = isiPeriodicTypeDrum+1;

SchedulerEndWithNotification:
#ifdef ISI_SUPPORT_DIAGNOSTICS
			_IsiConditionalDiagnostics(isiSendPeriodic, Code);
#endif
SchedulerEnd:
			if (_isiVolatile.Periodic.drumPause == isiPeriodicTypes) {
				_isiVolatile.Periodic.drumPause = isiPeriodicTypeDrum;
			}
		}
	}
}

//	end of TickS.c
