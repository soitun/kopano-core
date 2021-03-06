/*
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright 2005 - 2016 Zarafa and its licensors
 */
#include <kopano/platform.h>
#include <utility>
#include "ZCABLogon.h"
#include "ZCABContainer.h"
#include <kopano/ECTags.h>
#include <kopano/ECGuid.h>
#include <kopano/memory.hpp>
#include "kcore.hpp"
#include <mapix.h>
#include <edkmdb.h>

using namespace KC;

zcabFolderEntry::~zcabFolderEntry()
{
	MAPIFreeBuffer(lpStore);
	MAPIFreeBuffer(lpFolder);
}

zcabFolderEntry::zcabFolderEntry(zcabFolderEntry &&o) :
	cbStore(o.cbStore), cbFolder(o.cbFolder),
	lpStore(o.lpStore), lpFolder(o.lpFolder),
	strwDisplayName(std::move(o.strwDisplayName))
{
	o.cbStore = o.cbFolder = 0;
	o.lpStore = o.lpFolder = nullptr;
}

ZCABLogon::ZCABLogon(IMAPISupport *lpMAPISup, ULONG ulProfileFlags,
    const GUID *lpGUID) :
	m_lpMAPISup(lpMAPISup),
	m_lFolders(std::make_shared<std::vector<zcabFolderEntry>>())
{
	// The specific GUID for *this* addressbook provider, if available
	m_ABPGuid = lpGUID != nullptr ? *lpGUID : GUID_NULL;
}

HRESULT ZCABLogon::Create(IMAPISupport *lpMAPISup, ULONG ulProfileFlags,
    const GUID *lpGuid, ZCABLogon **lppZCABLogon)
{
	return alloc_wrap<ZCABLogon>(lpMAPISup, ulProfileFlags, lpGuid).put(lppZCABLogon);
}

HRESULT ZCABLogon::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE2(ZCABLogon, this);
	REGISTER_INTERFACE2(ECUnknown, this);
	REGISTER_INTERFACE2(IABLogon, this);
	REGISTER_INTERFACE2(IUnknown, this);
	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ZCABLogon::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError)
{
	return MAPI_E_CALL_FAILED;
}

HRESULT ZCABLogon::Logoff(ULONG ulFlags)
{
	//FIXME: Release all Other open objects ?
	//Releases all open objects, such as any subobjects or the status object. 
	//Releases the provider's support object.
	m_lpMAPISup.reset();
	return hrSuccess;
}

/** 
 * Adds a folder to the addressbook provider. Data is read from global profile section.
 * 
 * @param[in] strDisplayName Display string in hierarchy table
 * @param[in] cbStore bytes in lpStore
 * @param[in] lpStore Store entryid lpFolder entryid is in
 * @param[in] cbFolder bytes in lpFolder
 * @param[in] lpFolder Folder entryid pointing to a contacts folder (assumed, should we check?)
 * 
 * @return MAPI Error code
 */
HRESULT ZCABLogon::AddFolder(const wchar_t *lpwDisplayName, ULONG cbStore,
    LPBYTE lpStore, ULONG cbFolder, LPBYTE lpFolder)
{
	zcabFolderEntry entry;

	if (cbStore == 0 || lpStore == NULL || cbFolder == 0 || lpFolder == NULL)
		return MAPI_E_INVALID_PARAMETER;

	entry.strwDisplayName = lpwDisplayName;

	entry.cbStore = cbStore;
	auto hr = MAPIAllocateBuffer(cbStore, reinterpret_cast<void **>(&entry.lpStore));
	if (hr != hrSuccess)
		return hr;
	memcpy(entry.lpStore, lpStore, cbStore);

	entry.cbFolder = cbFolder;
	hr = MAPIAllocateBuffer(cbFolder, reinterpret_cast<void **>(&entry.lpFolder));
	if (hr != hrSuccess)
		return hr;
	memcpy(entry.lpFolder, lpFolder, cbFolder);
	m_lFolders->emplace_back(std::move(entry));
	return hrSuccess;
}

/** 
 * EntryID is NULL: Open "root container". Returns an ABContainer
 * which returns the provider root container. The EntryID in that one
 * table entry opens the ABContainer which has all the folders from
 * stores which were added using the AddFolder interface.
 * 
 * Root container EntryID: 00000000727f0430e3924fdab86ae52a7fe46571 (version + guid)
 * Sub container EntryID : 00000000727f0430e3924fdab86ae52a7fe46571 + obj type + 0 + folder contact eid
 * Contact Item EntryID  : 00000000727f0430e3924fdab86ae52a7fe46571 + obj type + email offset + kopano item eid
 * 
 * @param[in] cbEntryID 0 or bytes in lpEntryID
 * @param[in] lpEntryID NULL or a valid entryid
 * @param[in] lpInterface NULL or supported interface
 * @param[in] ulFlags unused
 * @param[out] lpulObjType MAPI_ABCONT, or a type from ZCAB
 * @param[out] lppUnk pointer to ZCABContainer
 * 
 * @return 
 */
HRESULT ZCABLogon::OpenEntry(ULONG cbEntryID, const ENTRYID *lpEntryID,
    const IID *lpInterface, ULONG ulFlags, ULONG *lpulObjType,
    IUnknown **lppUnk)
{
	object_ptr<ZCABContainer> lpRootContainer;
	object_ptr<IUnknown> lpContact;
	object_ptr<IProfSect> lpProfileSection;
	memory_ptr<SPropValue> lpFolderProps;
	ULONG cValues = 0;
	static constexpr SizedSPropTagArray(3, sptaFolderProps) =
		{3, {PR_ZC_CONTACT_STORE_ENTRYIDS,
		PR_ZC_CONTACT_FOLDER_ENTRYIDS, PR_ZC_CONTACT_FOLDER_NAMES_W}};
	
	// Check input/output variables 
	if (lppUnk == nullptr)
		return MAPI_E_INVALID_PARAMETER;

	if(cbEntryID == 0 && lpEntryID == NULL) {
		// this is the "Kopano Contacts Folders" container. Get the hierarchy of this folder. SetEntryID(0000 + guid + MAPI_ABCONT + ?) ofzo?
		auto hr = ZCABContainer::Create(nullptr, nullptr, m_lpMAPISup, this, &~lpRootContainer);
		if (hr != hrSuccess)
			return hr;
	} else if (cbEntryID < 4 + sizeof(GUID) || lpEntryID == nullptr) {
		return MAPI_E_UNKNOWN_ENTRYID;
	} else {
		// you can only open the top level container
		if (memcmp((LPBYTE)lpEntryID +4, &MUIDZCSAB, sizeof(GUID)) != 0)
			return MAPI_E_UNKNOWN_ENTRYID;
		auto hr = m_lpMAPISup->OpenProfileSection(reinterpret_cast<const MAPIUID *>(&pbGlobalProfileSectionGuid), 0, &~lpProfileSection);
		if (hr != hrSuccess)
			return hr;
		hr = lpProfileSection->GetProps(sptaFolderProps, 0, &cValues, &~lpFolderProps);
		if (FAILED(hr))
			return hr;

		// remove old list, if present
		m_lFolders->clear();

		// make the list
		if (lpFolderProps[0].ulPropTag == PR_ZC_CONTACT_STORE_ENTRYIDS &&
			lpFolderProps[1].ulPropTag == PR_ZC_CONTACT_FOLDER_ENTRYIDS &&
			lpFolderProps[2].ulPropTag == PR_ZC_CONTACT_FOLDER_NAMES_W &&
			lpFolderProps[0].Value.MVbin.cValues == lpFolderProps[1].Value.MVbin.cValues &&
			lpFolderProps[1].Value.MVbin.cValues == lpFolderProps[2].Value.MVszW.cValues)
			for (ULONG c = 0; c < lpFolderProps[1].Value.MVbin.cValues; ++c)
				AddFolder(lpFolderProps[2].Value.MVszW.lppszW[c],
						  lpFolderProps[0].Value.MVbin.lpbin[c].cb, lpFolderProps[0].Value.MVbin.lpbin[c].lpb,
						  lpFolderProps[1].Value.MVbin.lpbin[c].cb, lpFolderProps[1].Value.MVbin.lpbin[c].lpb);

		hr = ZCABContainer::Create(m_lFolders, nullptr, m_lpMAPISup, this, &~lpRootContainer);
		if (hr != hrSuccess)
			return hr;

		if (cbEntryID > 4+sizeof(GUID)) {
			// we're actually opening a contact .. so pass-through to the just opened rootcontainer
			hr = lpRootContainer->OpenEntry(cbEntryID, lpEntryID, &IID_IUnknown, ulFlags, lpulObjType, &~lpContact);
			if (hr != hrSuccess)
				return hr;
		}
	}

	HRESULT hr = hrSuccess;
	if (lpContact) {
		hr = lpContact->QueryInterface(lpInterface != nullptr ? *lpInterface : IID_IDistList, reinterpret_cast<void **>(lppUnk));
	} else {
		if (lpulObjType != nullptr)
			*lpulObjType = MAPI_ABCONT;
		hr = lpRootContainer->QueryInterface(lpInterface != nullptr ? *lpInterface : IID_IABContainer, reinterpret_cast<void **>(lppUnk));
	}
	return hr;
}

// we don't implement CompareEntryIDs .. a real ab provider should do this action.

HRESULT ZCABLogon::Advise(ULONG cbEntryID, const ENTRYID *lpEntryID,
    ULONG evt_mask, IMAPIAdviseSink *lpAdviseSink, ULONG *lpulConnection)
{
	if (lpAdviseSink == NULL || lpulConnection == NULL)
		return MAPI_E_INVALID_PARAMETER;
	if (lpEntryID == NULL)
		return MAPI_E_INVALID_PARAMETER;
	return MAPI_E_NO_SUPPORT;
}

HRESULT ZCABLogon::PrepareRecips(ULONG ulFlags,
    const SPropTagArray *lpPropTagArray, LPADRLIST lpRecipList)
{
	if(lpPropTagArray == NULL || lpPropTagArray->cValues == 0)
		/* There is no work to do. */
		return hrSuccess;
	return MAPI_E_NO_SUPPORT;
}
