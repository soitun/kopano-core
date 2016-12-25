/*
 * Copyright 2005 - 2016 Zarafa and its licensors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <kopano/platform.h>
#include <new>
#include "ZCABProvider.h"
#include "ZCABLogon.h"

#include <mapidefs.h>
#include <mapicode.h>
#include <mapiguid.h>

#include <kopano/ECGuid.h>
#include <kopano/ECInterfaceDefs.h>
#include <kopano/ECDebug.h>
#include <kopano/Trace.h>
#include <kopano/memory.hpp>

using namespace KCHL;

ZCABProvider::ZCABProvider(ULONG ulFlags, const char *szClassName) :
    ECUnknown(szClassName)
{
}

HRESULT ZCABProvider::Create(ZCABProvider **lppZCABProvider)
{
	auto lpZCABProvider = new(std::nothrow) ZCABProvider(0, "ZCABProvider");
	if (lpZCABProvider == nullptr)
		return MAPI_E_NOT_ENOUGH_MEMORY;
	HRESULT hr = lpZCABProvider->QueryInterface(IID_ZCABProvider,
	             reinterpret_cast<void **>(lppZCABProvider));
	if(hr != hrSuccess)
		delete lpZCABProvider;
	return hrSuccess;
}

HRESULT ZCABProvider::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE2(ZCABProvider, this);
	REGISTER_INTERFACE2(ECUnknown, this);
	REGISTER_INTERFACE2(IABProvider, &this->m_xABProvider);
	REGISTER_INTERFACE2(IUnknown, &this->m_xABProvider);
	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ZCABProvider::Shutdown(ULONG * lpulFlags)
{
	*lpulFlags = 0;
	return hrSuccess;
}

HRESULT ZCABProvider::Logon(LPMAPISUP lpMAPISup, ULONG ulUIParam, LPTSTR lpszProfileName, ULONG ulFlags, ULONG * lpulcbSecurity, LPBYTE * lppbSecurity, LPMAPIERROR * lppMAPIError, LPABLOGON * lppABLogon)
{
	HRESULT hr = hrSuccess;
	object_ptr<ZCABLogon> lpABLogon;

	if (!lpMAPISup || !lppABLogon) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	// todo: remove flags & guid .. probably add other stuff from profile?
	hr = ZCABLogon::Create(lpMAPISup, 0, nullptr, &~lpABLogon);
	if(hr != hrSuccess)
		goto exit;

	AddChild(lpABLogon);

	hr = lpABLogon->QueryInterface(IID_IABLogon, (void **)lppABLogon);
	if(hr != hrSuccess)
		goto exit;

	if (lpulcbSecurity)
		*lpulcbSecurity = 0;

	if (lppbSecurity)
		*lppbSecurity = NULL;

	if (lppMAPIError)
		*lppMAPIError = NULL;

exit:
	return hr;
}

DEF_ULONGMETHOD1(TRACE_MAPI, ZCABProvider, ABProvider, AddRef, (void))
DEF_ULONGMETHOD1(TRACE_MAPI, ZCABProvider, ABProvider, Release, (void))
DEF_HRMETHOD1(TRACE_MAPI, ZCABProvider, ABProvider, QueryInterface, (REFIID, refiid), (void **, lppInterface))
DEF_HRMETHOD1(TRACE_MAPI, ZCABProvider, ABProvider, Shutdown, (ULONG *, lpulFlags))
DEF_HRMETHOD1(TRACE_MAPI, ZCABProvider, ABProvider, Logon, (LPMAPISUP, lpMAPISup), (ULONG, ulUIParam), (LPTSTR, lpszProfileName), (ULONG, ulFlags), (ULONG *, lpulcbSecurity), (LPBYTE *, lppbSecurity), (LPMAPIERROR *, lppMAPIError), (LPABLOGON *, lppABLogon))
