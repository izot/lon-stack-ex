//
// LtAppDynamic.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtAppDynamic.h#1 $
//

/**
 * This interface defines optional methods which must be implemented by an application
 * using the LonTalk Java Stack as implemented via the classes LonTalkStack and
 * Layer7.  These are only needed if the application supports version 2 self
 * identification data.  In particular, this version of the SI data supports
 * changeable NV types and dynamic NVs.
 *
 * Methods which get/set option fields such as NV name, description and rate
 * estimates need not concern themselves with the availability flags in the
 * attributes as these are managed by the infrastructure.  In particular, the 
 * infrastructure will not attempt to get the name if the nameAvailable flag
 * is not set and upon setting the name, the nameAvailable flag will be set.
 *
 */

class LtAppDynamic {

public:    
    /**
     * This method gets the NV name.  For array elements, the name does not 
     * include array subscripts.
     * @param index
     *              the NV index
     * @return      the NV name string
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual LPSTR getNvName(int index) = 0;
    
    /**
     * This method gets the NV description. 
     * @param index
     *              the NV index
     * @return      the NV description string
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual LPSTR getNvDescription(int index) = 0;
    
    /**
     * This method gets the rate estimate for an NV. 
     * @param index
     *              the NV index
     * @return      the rate estimate (messages per 10 seconds).
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual int getNvRateEstimate(int index) = 0;
    
    /**
     * This method gets the maximum rate estimate for an NV. 
     * @param index
     *              the NV index
     * @return      the maximum rate estimate (messages per 10 seconds).
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual int getNvMaxRateEstimate(int index) = 0;
    
    /**
     * This method gets the attributes of an NV. 
     * @param index
     *              the NV index
     * @return      the attributes.
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual LtNetworkVariableAttributes* getNvAttributes(int index) 
                                        = 0;
    
    /**
     * This method sets the attributes of an NV. 
     * @param index
     *              the NV index
     * @param nva   
     *              the network variable attributes
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual void setNvAttributes(int index, LtNetworkVariableAttributes* nva) 
                                        = 0;
    
    /**
     * This method sets the NV name.  For array elements, the name does not 
     * include array subscripts.
     * @param index
     *              the NV index
     * @param name
     *              the NV name string
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual void setNvName(int index, char* name) = 0;
    
    /**
     * This method sets the NV description.  
     * @param index
     *              the NV index
     * @param description
     *              the NV description string
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual void setNvDescription(int index, char* description) = 0;
    
    /**
     * This method sets the rate estimate for an NV. 
     * @param index
     *              the NV index
     * @param rateEstimate
     *              the rate estimate (messages per 10 seconds).
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual void setNvRateEstimate(int index, int rateEstimate) = 0;
    
    /**
     * This method sets the maximum rate estimate for an NV. 
     * @param index
     *              the NV index
     * @param rateEstimate
     *              the maximum rate estimate (messages per 10 seconds).
     * @exception IllegalArgumentException
     *              Invalid NV index
     */
    virtual void setNvMaxRateEstimate(int index, int rateEstimate) = 0;

    /**
     * This method returns the device's self documentation string.
     * @return      a String containing the self documentation string.
     */
    virtual char* getNodeSd()= 0;

    /**
     * This method defines a dynamic NV.  
     *
     * Note that NV arrays are somewhat tricky.
     * The NV defined via this call may be an NV array in which case the arraySize in
     * the attributes is non-zero.  Subsequent to the define operation, attributes 
     * can be looked up on a per element basis.  The attributes, name, description, etc.
     * can be stored globally for the array except for the "snvtTypeIndex" which must 
     * maintained per element and for "arrayIndex" which is unique per element.
     *
     * The infrastructure takes care of calling "registerNv" appropriately for the
     * new NV/NV array elements.
     * @param index
     *              the NV index to create.  Each NV or NV array element is represented
     *              by a unique NV index.
     * @param attributes
     *              the NV attributes
     * @exception IllegalArgumentException
     *              Failed to define NV due to problem with index or arrayLength
     */
    virtual void defineNv(int index, LtNetworkVariableAttributes* attributes) 
                                                    = 0;
    
    /**
     * This method removes a dynamically created NV.
     * @param index
     *              the NV index to remove
     * @param count
     *              the number of NVs to remove.  This count includes one count per
     *              array element where applicable.  Removing a subset of the elements
     *              of an array is not supported.  For example, assume index 100, 101
     *              and 110 are non-array NVs and that indices 102..109 form an NV
     *              array of 8 elements.  Then, the following calls are valid:
     *                  removeNv(101,10);
     *                  removeNv(102,8);
     *              The following calls are invalid:
     *                  removeNv(101,6);
     *                  removeNv(105,1);
     * @exception IllegalArgumentException
     *              index or index/count are invalid
     */
    virtual void removeNv(int index, int count) = 0;
};

