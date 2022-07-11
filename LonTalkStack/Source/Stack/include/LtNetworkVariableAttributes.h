//
// LtNetworkVariableAttributes.h
//
// Copyright © 2022 Dialog Semiconductor
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNetworkVariableAttributes.h#1 $
//


/**
 * This class defines the attributes of a network variable for self identification
 * purposes.
 *
 */

class LtNetworkVariableAttributes {

private:

protected:

public:
    LtNetworkVariableAttributes()
	{
        dynamic = false;
        configClass = false;
        authConfig = true;
        priorityConfig = true;
        serviceTypeConfig = true;
        offline = false;
        polled = false;
        sync = false;
        snvtTypeIndex = 0;
        length = 0;
        arraySize = 0;
        arrayIndex = 0;
        service = LT_ACKD;
        priority = false;
        authenticated = false;
        output = false;
        descriptionAvailable = false;
        nameAvailable = false;
        rateEstimateAvailable = false;
        maxRateEstimateAvailable = false;
	}
    
    /**
     * True if NV was dynamically created.
     */
    boolean dynamic;
    
    /**
     * True if NV is a configuration class NV. 
     */
    boolean configClass;
    
    /**
     * True if authentication attribute is configurable for the NV.
     */
    boolean authConfig;
    
    /**
     * True if priority attribute is configurable for the NV.
     */
    boolean priorityConfig;
    
    /**
     * True if the NV's service type is configurable.
     */
    boolean serviceTypeConfig;
    
    /**
     * True if device must be taken offline prior to modifying this NV.
     */
    boolean offline;
    
    /**
     * True if NV is a polled output (application does not generate NV update messages).
     */
    boolean polled;
    
    /**
     * True if NV is synchronous (all values assigned to an NV are propagated).
     */
    boolean sync;
    
    /**
     * The NV's snvt type index.  Zero if NV is user-defined.
     */
    int snvtTypeIndex;
    
    /**
     * The length of the NV (or NV array element).
     */
    byte length;
    
    /**
     * The number of NV array elements (0 if not an NV array).
     */
    int arraySize;
    
    /** 
     * The index of the NV within the NV array. (0 if not in NV array).
     */
    int arrayIndex;
    
    /**
     * The default service type of the NV.
     */
    LtServiceType service;
    
    /**
     * The default priority attribute of the NV.
     */
    boolean priority;
    
    /** 
     * The default authentication attribute of the NV.
     */
    boolean authenticated;
    
    /** 
     * The default direction of the NV (true for output).
     */
    boolean output;
    
    /**
     * Description string indicator - true if string is available.
     */
    boolean descriptionAvailable;
    
    /**
     * Name string indicator - true if string is available.
     */
    boolean nameAvailable;
    
    /** 
     * Rate estimate indicator - true if rate estimate is available.
     */
    boolean rateEstimateAvailable;
    
    /** 
     * Maximum rate estimate indicator - true if rate estimate is available.
     */
    boolean maxRateEstimateAvailable;
};

