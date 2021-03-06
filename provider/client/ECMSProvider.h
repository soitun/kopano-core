/*
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright 2005 - 2016 Zarafa and its licensors
 */
#pragma once
#include <kopano/ECUnknown.h>
#include <kopano/Util.h>
#include <kopano/memory.hpp>
#include <kopano/zcdefs.h>
#include "WSTransport.h"
#include <string>

class ECMSProvider KC_FINAL_OPG : public KC::ECUnknown, public IMSProvider {
protected:
	ECMSProvider() = default;
public:
	static HRESULT Create(ECMSProvider **);
	virtual HRESULT QueryInterface(const IID &, void **) override;
	virtual HRESULT Shutdown(unsigned int *flags) override;
	virtual HRESULT Logon(IMAPISupport *, ULONG_PTR ui_param, const TCHAR *profile, ULONG eid_size, const ENTRYID *eid, ULONG flags, const IID *intf, ULONG *ssec_size, BYTE **spool_sec, MAPIERROR **, IMSLogon **, IMsgStore **) override;
	virtual HRESULT CompareStoreIDs(ULONG eid1_size, const ENTRYID *eid1, ULONG eid2_size, const ENTRYID *eid2, ULONG flags, ULONG *result) override;

private:
	static HRESULT LogonByEntryID(KC::object_ptr<WSTransport> &, sGlobalProfileProps *, ULONG eid_size, const ENTRYID *eid);

	std::string m_strLastUser, m_strLastPassword;
	ALLOC_WRAP_FRIEND;
};

class ECMSProviderSwitch KC_FINAL_OPG : public KC::ECUnknown, public IMSProvider {
protected:
	ECMSProviderSwitch() = default;

public:
	static HRESULT Create(ECMSProviderSwitch **);
	virtual HRESULT QueryInterface(const IID &, void **) override;
	virtual HRESULT Shutdown(unsigned int *flags) override;
	virtual HRESULT Logon(IMAPISupport *, ULONG_PTR ui_param, const TCHAR *profile, ULONG eid_size, const ENTRYID *eid, ULONG flags, const IID *intf, ULONG *ssec_size, BYTE **spool_sec, MAPIERROR **, IMSLogon **, IMsgStore **) override;
	virtual HRESULT CompareStoreIDs(ULONG eid1_size, const ENTRYID *eid1, ULONG eid2_size, const ENTRYID *eid2, ULONG flags, ULONG *result) override;

protected:
	ALLOC_WRAP_FRIEND;
};
