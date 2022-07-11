#ifndef LTNETWORKVARIABLECONFIGURATION_H
#define LTNETWORKVARIABLECONFIGURATION_H

//
// LtNetworkVariableConfiguration.h
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/include/LtNetworkVariableConfiguration.h#2 $
//

#define LT_UNBOUND_INDEX		0xffff
#define LT_UNUSED_ALIAS			0xffff
#define LT_UNUSED_INDEX			0xffff
#define LT_UNUSED_SELECTOR		0xffff
#define LT_MAX_BOUND_SELECTOR	0x2fff
#define LT_MAX_SELECTOR			0x3fff
#define LT_NV_STORE_SIZE		10

#define LT_SELECTION_UNCONDITIONAL		0x00
#define LT_SELECTION_BYSOURCE			0x01
#define LT_SELECTION_NEVER				0x02

class LTA_EXTERNAL_CLASS LtNetworkVariableConfigurationBase 
{
	DefineBool(IsAlias)
	DefineBool(Priority)
	DefineBool(Turnaround)
	DefineBool(Authenticated)
	DefineBool(Output)
	DefineBool(ReadByIndex)
	DefineBool(WriteByIndex)
	DefineBool(RemoteNmAuth)
	DefineBool(SourceSelectionOnly)
	DefineInt(NvUpdateSelection)
	DefineInt(NvResponseSelection)
	DefineInt(NvRequestSelection)
	DefineInt(Index)
	DefineInt(AddressTableIndex)
	DefineInt(NvIndex)
	DefineInt(Selector)
	DefineInt(PrimaryIndex)
        // The incarnation number is a non-persistent value used to identify an instance
        // of an NV, when the index may be reused as NVs are deleted/recreated.
    DefineInt(IncarnationNumber)    

private:
    LtServiceType	m_serviceType;
	LtOutgoingAddress m_address;
	LtVector* m_pAliases;

public:
    LtNetworkVariableConfigurationBase(LtBlob &blob) 
    {
		m_pAliases = NULL;
        setIncarnationNumber(0);    // Not passed.
        package(&blob);
    };
    static void ltShutdown(void);  // This should only be called on process exit
protected:
    void package(LtBlob *pBlob); 
    friend class LtStackBlob;

protected:

	void transferred() { m_pAliases = null; }
public:
	LtNetworkVariableConfigurationBase(int nIndex, int nType, int x);
    LtNetworkVariableConfigurationBase();
	~LtNetworkVariableConfigurationBase();

	static int getStoreSize() { return LT_NV_STORE_SIZE; }

    boolean isBound(int flags = ISBOUND_ANY);
    boolean inUse();
	boolean hasAddress();
    boolean hasOutputAddress();
	int getPrimary();
	boolean isAlias() { return m_bIsAlias; }

	void initSelector();
	static boolean unboundSelector(int selector);
	static boolean unboundSelectorToIndex(int selector, int& index);
	static boolean unboundSelectorForIndex(int selector, int index);
    static int computeSelector(int index);
    void init(int nType);
	void init(int nIndex, int nPrimary, int nType);

    LtOutgoingAddress* getAddress() { return &m_address; }

    LtServiceType getServiceType() { return m_serviceType; }
	void setServiceType(LtServiceType serviceType) { m_serviceType = serviceType; }

    int toLonTalk(byte* data, boolean bExact = false, int nVersion = 2, bool expanded = false);
    int fromLonTalk(byte data[], int length, int nVersion = 2, bool expanded = false);

	void initialize();

	void addAlias(int index);
	void removeAlias(int index);
	boolean nextAlias(LtVectorPos &pos, int &index);
	void clearAliases() { delete m_pAliases; m_pAliases = null; }
    void incrementIncarnation();
    boolean incarnationMatches(int incarnationNumber);
};

class LTA_EXTERNAL_CLASS LtNetworkVariableConfiguration : public LtNetworkVariableConfigurationBase
{
	DefineBool(IsACopy)
public:

	LtNetworkVariableConfiguration& operator=( const LtNetworkVariableConfiguration& ac)
	{
		*static_cast<LtNetworkVariableConfigurationBase*>(this) =
			*static_cast<const LtNetworkVariableConfigurationBase *>(&ac);
		return *this;
	}
	LtNetworkVariableConfiguration() : LtNetworkVariableConfigurationBase()
	{
		m_bIsACopy = TRUE;
	}
    
    LtNetworkVariableConfiguration(int nIndex, int nType, int x) : 
		LtNetworkVariableConfigurationBase(nIndex, nType, x)
	{
		m_bIsACopy = TRUE;
	}

	LtNetworkVariableConfiguration(LtBlob &blob) :
		LtNetworkVariableConfigurationBase(blob)
	{
		m_bIsACopy = TRUE;
	}

	~LtNetworkVariableConfiguration() 
	{
		if (m_bIsACopy)
		{	// Make sure base class does not delete alias vector, since
			// this is a copy.
			transferred();
		}
	}
};

#endif
