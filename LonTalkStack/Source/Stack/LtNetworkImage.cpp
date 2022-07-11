//
// LtNetworkImage.cpp
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
// $Header: //depot/Software/IzoT/Dev/LonTalkStack/Source/Stack/LtNetworkImage.cpp#3 $
//

#include "LtStackInternal.h"

#if FEATURE_INCLUDED(ENCRYPTED_PASSWORDS)
#include "max_api.h"
#endif

LtNetworkImage::LtNetworkImage(LtDeviceStack* pStack) : m_persistence(CURRENT_NETIMG_VER), aliasTable(nvTable)
{
	setStack(pStack);
    m_nState = LT_UNCONFIGURED;
	m_bBlackout = false;
	m_bHasBeenEcsChanged = false;
	m_persistence.registerPersistenceClient(this);
#if PERSISTENCE_TYPE_IS(FTXL)
    m_persistence.setType(LonNvdSegNetworkImage);
#else
	m_persistence.setName("IMG");
#endif
	m_persistence.setCommitFailureNotifyMode(true);
}

void LtNetworkImage::sync()
{
	m_persistence.sync();
}

LtNetworkImage::~LtNetworkImage()
{
}

boolean LtNetworkImage::store(boolean bRecompute)
{
	m_persistence.setSuppressChecksumCalculation(!bRecompute);

	// For now, only support implicit commit
	return m_persistence.schedule();
}

void LtNetworkImage::reset(LonTalkNode& node) 
{
	sync();

	LtPersistenceLossReason reason = m_persistence.restore();

	if (reason != LT_PERSISTENCE_OK)
	{
		if (reason == LT_RESET_DURING_UPDATE)
		{
			m_bBlackout = true;
		}
		if (reason == LT_CORRUPTION ||
		    reason == LT_SIGNATURE_MISMATCH ||
			reason == LT_RESET_DURING_UPDATE)
		{
			node.errorLog(LT_CNFG_CS_ERROR);
		}
	}
}

LtPersistenceLossReason LtNetworkImage::deserialize(byte* pImage, int len, int nVersion)
{
	LtProgramId pid(getStack()->getProgramIdRef());	// Initialize by reference to copy the "modifiable" flag, too (for LtMipApp)
	TableCounts counts;
	LtPersistenceLossReason reason = LT_PERSISTENCE_OK;
	boolean valid = false;
	int i;

#if FEATURE_INCLUDED(ENCRYPTED_PASSWORDS)
	if (nVersion >= NetImageVer_encrypted)
	{
		// Image is encrypted.  We are really just trying to encrypt the keys but encrypting only the keys
		// is more trouble than it's worth.  Makes debugging a bit trickier, but so be it.
		MaxIv iv;
		if ((unsigned)len >= sizeof(iv))
		{
			// If the len was too short, something really bad happened and we just skip decrypting
			memcpy(&iv, pImage, sizeof(iv));
			pImage += sizeof(iv);
			len -= sizeof(iv);
			MAX_Cipher(&iv, pImage, len);
		}
	}
#endif

	addressTable.lock();
	nvTable.lock();

	getCounts(counts);

	pid.setData(&pImage[LT_OS_PROGRAMID]);

	if (!pid.isCompatible(*getStack()->getReadOnly()->getProgramId())) 
	{
		reason = LT_PROGRAM_ID_CHANGE;
	}
	else if (memcmp(&counts, &pImage[LT_OS_COUNTS], sizeof(counts)) != 0)
	{
		reason = LT_PROGRAM_ATTRIBUTE_CHANGE;
	}
	else
	{
		// Just for LtMipApp's update the program ID in the read-only data (mem only)
		getStack()->getReadOnly()->setProgramId(pid);

		// Load image into tables.
		m_nState = pImage[LT_OS_STATE];

		valid = configData.fromLonTalk(&pImage[LT_OS_CONFIGDATA], 0, configData.getLength(), false);

		if (valid && nVersion >= NetImageVer_ECS)
		{
			// See "ECS Lockout" in LtNetworkManager.cpp
			setHasBeenEcsChanged(pImage[LT_OS_ECSNODE]);
		}

		if (valid)
		{
			int offset = LT_OS_DOMAIN;
			int len;

			for (i = 0; valid && i < domainTable.getCount(); i++) 
			{
                LtLonTalkDomainStyle style = (nVersion < NetImageVer_OMA) ? LT_CLASSIC_DOMAIN_STYLE : LT_OMA_DOMAIN_STYLE;
				LtDomainConfiguration dc;
				valid = dc.fromLonTalk(&pImage[offset], len, style) == LT_NO_ERROR;
				offset += len;
				if (valid)
				{
 					getStack()->updateDomainConfiguration(i, &dc, false, true);
				}
			}
			if (valid)
			{
				int count;
				int index;
				if (nVersion == NetImageVer_original)
				{
					// After version NetImageVer_original, address entries immediately follow domain entries.
					offset = LT_OS_ADDRESS;
				}
				if (nVersion < NetImageVer_compact)
				{
					count = addressTable.getCount();
				}
				else
				{
					count =  LtMisc::makeint32(pImage[offset+3], pImage[offset+2],
								pImage[offset+1], pImage[offset]);
					offset += sizeof(AddressTableStoreHeader);;
				}

				for (i = 0; valid && i < count; i++) 
				{
					LtAddressConfiguration ac;

					if (nVersion < NetImageVer_compact)
					{	// All address table entries stored, index not stored
						index = i;
					}
					else
					{	// Index preceeds entry
						index = LtMisc::makeint32(pImage[offset+3], pImage[offset+2],
								pImage[offset+1], pImage[offset]);
						offset += sizeof(index);
					}
					// Note that the address table version matches the network image version for
					// versions 1,2 and 3.  This may not always be the case.
					valid = ac.fromLonTalk(&pImage[offset], len, nVersion) == LT_NO_ERROR;
					offset += len;
					if (valid)
					{
						getStack()->updateAddressConfiguration(index, &ac, true);
					}
				}
			}
            if (valid)
            {
                getStack()->loadAdditionalConfig((LtNetworkImageVersion)nVersion, pImage, offset);
            }

			if (valid)
			{
				if (nVersion < NetImageVer_ECS)
				{
					offset = LT_OS_NV_ROUTE;
				}
				if (getStack()->isNodeStack())
				{
					valid = nvTable.load(pImage, offset, nVersion) == LT_NO_ERROR;
				}
				else
				{
					// Load router configuration.
					valid = getStack()->loadRoutes(&pImage[offset]);
				}
			}
		} 
		if (!valid)
		{
			reason = LT_CORRUPTION;
		}
	}
	nvTable.unlock();
	addressTable.unlock();

	// Now that we have unlocked the tables, send the domain update messages to the LRE
    // EPR 51479 Skip this if the device is a layer 5 mip, which does not require routing and
    // does not support the getDomainConfiguration method.
    if (!getStack()->getLayer5Mip())
    {
	    for (i = 0; valid && i < domainTable.getCount(); i++) 
	    {
		    LtDomainConfiguration *pDc;
		    LtErrorType err = getStack()->getLayer4()->getDomainConfiguration(i, &pDc);

		    if (err == LT_NO_ERROR)
		    {
			    LtSubnetNodeClient* pClient = (LtSubnetNodeClient*) pDc;
			    pClient->notify();
		    }
	    }
    }
	return reason;
}

void LtNetworkImage::initImage()
{
}

boolean LtNetworkImage::getBlackout()
{
	boolean rtn = m_bBlackout;
	m_bBlackout = false;
	return rtn;
}

void LtNetworkImage::setNvAliasCount(int numPublicNvEntries, int numPrivateNvEntries, int numAliasEntries) 
{
    nvTable.setCounts(numPublicNvEntries, numPrivateNvEntries, numAliasEntries);
}

void LtNetworkImage::getCounts(TableCounts& counts)
{
	nvTable.getCounts(counts.m_nvCount, counts.m_privateNvCount, counts.m_aliasCount);
	counts.m_domainCount = domainTable.getCount();
	counts.m_addressCount = addressTable.getCount();
}

LtErrorType LtNetworkImage::setAuthKey(int index, byte* key) 
{
	LtDomainConfiguration dc;
    LtErrorType err = getStack()->getDomainConfiguration(index, &dc);
	if (err == LT_NO_ERROR)
	{
		dc.setKey(key);
		err = getStack()->updateDomainConfiguration(index, &dc, true, false);
	}
	return err;
}

#if PERSISTENCE_TYPE_IS(FTXL)
// Return the the maximum number of bytes that will be consumed for
// serialized data.
int  LtNetworkImage::getMaxSerialLength()
{
    int imageLen = 
        LT_OS_DOMAIN + 
        domainTable.getMaxStoreSize() +
        addressTable.getMaxStoreSize() + 
        nvTable.getMaxStoreSize(); 

    return imageLen;
}
#endif

void LtNetworkImage::serialize(byte* &pImage, int &imageLen)
{
	addressTable.lock();
	nvTable.lock();
	// Allocate space for the image.  Use a fixed amount plus compute
	// the NV size (since it could be quite large).
	imageLen = LT_OS_DOMAIN + domainTable.getStoreSize() + addressTable.getStoreSize() +
        getStack()->getAdditionallConfigStoreSize();

	if (getStack()->isNodeStack())
	{
		imageLen += nvTable.getStoreSize();
	}
	else
	{
		imageLen += getStack()->getRoutesStoreSize();
	}
	pImage = (byte*) malloc(imageLen);
	if (pImage != null)
	{
		int offset;
		TableCounts counts;
		getStack()->getReadOnly()->getProgramId()->getData(&pImage[LT_OS_PROGRAMID]);
		getCounts(counts);
		pImage[LT_OS_ECSNODE] = getHasBeenEcsChanged();
		memcpy(&pImage[LT_OS_COUNTS], &counts, sizeof(counts));
		configData.toLonTalk(&pImage[LT_OS_CONFIGDATA], 0, configData.getLength());
		offset = LT_OS_DOMAIN;
		domainTable.store(pImage, offset);
		addressTable.store(pImage, offset);
        getStack()->storeAdditionalConfig(pImage, offset);
		if (getStack()->isNodeStack())
		{
			nvTable.store(pImage, offset);
		}
		else
		{
			// Routing map update
			getStack()->storeRoutes(&pImage[offset]);
		}
		pImage[LT_OS_STATE] = (byte) m_nState;
	}
	nvTable.unlock();
	addressTable.unlock();

#if FEATURE_INCLUDED(ENCRYPTED_PASSWORDS)
	if (pImage != NULL)
	{
		int newLen = imageLen + sizeof(MaxIv);
		byte *pExtendedImage = (byte*)malloc(newLen);
		// If malloc failed, we just return unencrypted data which will decrypt all wrong.  Oh well.  Shouldn't happen.
		if (pExtendedImage != NULL)
		{
			MaxIv *pIv = (MaxIv*)pExtendedImage;
			MAX_GetIv(pIv);
			pExtendedImage += sizeof(MaxIv);
			memcpy(pIv+1, pImage, imageLen);
			free(pImage);
			MAX_Cipher(pIv, pIv+1, imageLen);
			pImage = (byte *)pIv;
			imageLen = newLen;
		}
	}
#endif
}

boolean LtNetworkImage::configurationChange(int newState)
{
	return unconfigured() == unconfigured(newState);
}


LtErrorType LtNetworkImage::changeState(int newState, boolean clearKeys) 
{
	LtErrorType err = LT_NO_ERROR;
    if (newState == LT_CONFIGURED ||
	    newState == LT_UNCONFIGURED ||
		newState == LT_HARD_OFFLINE ||
		newState == LT_APPLICATIONLESS) 
	{
		m_nState = newState;
		store();
	}
	else
	{
		err = LT_INVALID_PARAMETER;
	}
    if (clearKeys) 
	{
        int i=0;
        while (i < domainTable.getCount()) 
		{
			LtDomainConfiguration *pDc;
			domainTable.get(i++, &pDc);
			pDc->setKey(LtDomainConfiguration::getNullKey());
        }
    }
	return err;
}

boolean LtNetworkImage::unconfigured(int state)
{
	return state == LT_UNCONFIGURED || state == LT_APPLICATIONLESS;
}

boolean LtNetworkImage::unconfigured() 
{
	return unconfigured(m_nState);
}

int LtNetworkImage::getState() 
{
    return m_nState;
}

void LtNetworkImage::notifyPersistenceLost(LtPersistenceLossReason reason)
{
	getStack()->notifyPersistenceLost(reason);
}

LtErrorType LtNetworkImage::initialize(int state, int options, int domainIndex)
{
	LtErrorType err;

	setHasBeenEcsChanged(false);

	// This method initializes the domain, address and NV tables.
	err = addressTable.initialize(0, NM_MAX_INDEX, NULL, 0, domainIndex);

	if (err == LT_NO_ERROR)
	{
		err = nvTable.initialize(0, NM_MAX_INDEX, NULL, 0, domainIndex);
	}

	if (err == LT_NO_ERROR)
	{
		err = aliasTable.initialize(0, NM_MAX_INDEX, NULL, 0, domainIndex);
	}

	if (err == LT_NO_ERROR)
	{
	    byte			    key[LT_OMA_DOMAIN_KEY_LENGTH];
        boolean             useOma = FALSE;
        memcpy(key, LtDomainConfiguration::getNullKey(), sizeof(key));
        if (options&LT_DEVICE_INIT_OPT_PRESERVE_AUTH)
        {
            LtDomainConfiguration *pIncomingDomain = NULL;
            if (LtDomainConfiguration::isFlexDomain(domainIndex))
            {
                domainTable.getFlexAuthDomain(&pIncomingDomain);
            }
            else
            {
                domainTable.get(domainIndex, &pIncomingDomain);
            }
            if (pIncomingDomain != NULL)
            {
                memcpy(key, pIncomingDomain->getKey(), sizeof(key));
                useOma = pIncomingDomain->getUseOma();
            }
        }
		err = domainTable.initialize(0, NM_MAX_INDEX, NULL, 0, domainIndex);
	    if ((err == LT_NO_ERROR) && (options&LT_DEVICE_INIT_OPT_PRESERVE_AUTH))
	    {
            LtDomain domain;
            LtDomainConfiguration domainConfiguration(domain, key, 0, 0, useOma);
            err = domainTable.set(0, &domainConfiguration);
	    }
	}

	if (err == LT_NO_ERROR && !(options&LT_DEVICE_INIT_OPT_PRESERVE_AUTH))
	{
		// Turn off NM auth and clear the priority field.
		byte buf[LT_CD_NMAUTH+1];
		err = getStack()->getConfigurationData(buf, 0, sizeof(buf));
		if (err == LT_NO_ERROR)
		{
			buf[LT_CD_NMAUTH] &= ~0x08;
			buf[LT_CD_COMMPARAMS+7] = 0;
			err = getStack()->updateConfigurationData(buf, 0, sizeof(buf));
		}
	}

	if (err == LT_NO_ERROR && state)
	{	
		err = getStack()->changeState(state, false);
	}

	return err;
}


    // Initialized NV, alias and address tables, LT_EXP_INIT_CONFIG style
LtErrorType LtNetworkImage::expInitConfig(void)
{
    LtErrorType err = addressTable.initialize(0, NM_MAX_INDEX, NULL, 0, 0 /* Not used */);
    if (err == LT_NO_ERROR)
    {
        err = nvTable.initialize(0, NM_MAX_INDEX, NULL, 0, 0 /* Not used */);
    }
    if (err == LT_NO_ERROR)
    {
        err = aliasTable.initialize(0, NM_MAX_INDEX, NULL, 0, 0 /* Not used */);
    }
	return err;
}
