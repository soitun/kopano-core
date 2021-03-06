/*
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright 2005 - 2016 Zarafa and its licensors
 */
#pragma once
#include <list>
#include <memory>
#include <kopano/zcdefs.h>
#include <mapix.h>
#include <kopano/memory.hpp>
#include <kopano/CommonUtil.h>
#include <kopano/archiver-common.h>

namespace KC {

class ArchiverSession;

namespace helpers {

class MAPIPropHelper;
class MessageState;

/**
 * The MAPIPropHelper class provides some common utility functions that relate to IMAPIProp
 * objects in the archiver context.
 */
class KC_EXPORT MAPIPropHelper {
public:
	static HRESULT Create(IMAPIProp *, std::unique_ptr<MAPIPropHelper> *);
	KC_HIDDEN virtual ~MAPIPropHelper() = default;
	HRESULT GetMessageState(std::shared_ptr<ArchiverSession>, MessageState *);
	HRESULT GetArchiveList(std::list<SObjectEntry> *archives, bool ignore_source_key = false);
	HRESULT SetArchiveList(const std::list<SObjectEntry> &archives, bool explicit_commit = false);
	HRESULT SetReference(const SObjectEntry &sEntry, bool bExplicitCommit = false);
	KC_HIDDEN HRESULT GetReference(SObjectEntry *);
	HRESULT ClearReference(bool bExplicitCommit = false);
	HRESULT ReferencePrevious(const SObjectEntry &sEntry);
	HRESULT OpenPrevious(std::shared_ptr<ArchiverSession>, IMessage **);
	KC_HIDDEN HRESULT RemoveStub();
	HRESULT SetClean();
	HRESULT DetachFromArchives();
	virtual HRESULT GetParentFolder(std::shared_ptr<ArchiverSession>, IMAPIFolder **);

protected:
	KC_HIDDEN MAPIPropHelper(IMAPIProp *);
	KC_HIDDEN HRESULT Init();

private:
	object_ptr<IMAPIProp> m_ptrMapiProp;

	PROPMAP_DECL()
	PROPMAP_DEF_NAMED_ID(ARCHIVE_STORE_ENTRYIDS)
	PROPMAP_DEF_NAMED_ID(ARCHIVE_ITEM_ENTRYIDS)
	PROPMAP_DEF_NAMED_ID(ORIGINAL_SOURCEKEY)
	PROPMAP_DEF_NAMED_ID(STUBBED)
	PROPMAP_DEF_NAMED_ID(DIRTY)
	PROPMAP_DEF_NAMED_ID(REF_STORE_ENTRYID)
	PROPMAP_DEF_NAMED_ID(REF_ITEM_ENTRYID)
	PROPMAP_DEF_NAMED_ID(REF_PREV_ENTRYID)
};

class MessageState final {
public:
	bool isStubbed() const { return m_ulState & msStubbed; }
	bool isDirty() const { return m_ulState & msDirty; }
	bool isCopy() const { return m_ulState & msCopy; }
	bool isMove() const { return m_ulState & msMove; }

private:
	enum msFlags {
		msStubbed	= 0x01,	//<	The message is stubbed, mutual exclusive with msDirty
		msDirty		= 0x02,	//<	The message is dirty, mutual exclusive with msStubbed
		msCopy		= 0x04,	//< The message is copied, mutual exclusive with msMove
		msMove		= 0x08	//< The message is moved, mutual exclusive with msCopy
	};

	ULONG m_ulState = 0;
friend class MAPIPropHelper;
};

}} /* namespace */
