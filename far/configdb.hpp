﻿#ifndef CONFIGDB_HPP_552309E5_DEA6_42FD_BD7B_0F59C839FE62
#define CONFIGDB_HPP_552309E5_DEA6_42FD_BD7B_0F59C839FE62
#pragma once

/*
configdb.hpp

хранение настроек в базе sqlite.
*/
/*
Copyright © 2011 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "transactional.hpp"
#include "bitflags.hpp"

struct VersionInfo;
class representation_source;
class representation_destination;

namespace os
{
	class thread;
}

class representable: noncopyable
{
public:
	virtual ~representable() = default;
	virtual void Export(representation_destination& Representation) const = 0;
	virtual void Import(const representation_source& Representation) = 0;
};

class GeneralConfig: public representable, virtual public transactional
{
public:
	virtual ~GeneralConfig() override = default;

	virtual bool SetValue(const string_view& Key, const string_view& Name, const string_view& Value) = 0;
	virtual bool SetValue(const string_view& Key, const string_view& Name, unsigned long long Value) = 0;
	virtual bool SetValue(const string_view& Key, const string_view& Name, const bytes_view& Value) = 0;

	virtual bool GetValue(const string_view& Key, const string_view& Name, bool& Value, bool Default) const = 0;
	virtual bool GetValue(const string_view& Key, const string_view& Name, long long& Value, long long Default) const = 0;
	virtual bool GetValue(const string_view& Key, const string_view& Name, string& Value, const string& Default) const = 0;
	virtual bool GetValue(const string_view& Key, const string_view& Name, string& Value, const wchar_t* Default) const = 0;

	virtual bool DeleteValue(const string_view& Key, const string_view& Name) = 0;
	virtual bool EnumValues(const string_view& Key, DWORD Index, string& strName, string& strValue) const = 0;
	virtual bool EnumValues(const string_view& Key, DWORD Index, string& strName, long long& Value) const = 0;

	template<typename T>
	auto ValuesEnumerator(const string&&) const = delete;
	template<typename T>
	auto ValuesEnumerator(const string& Key) const
	{
		using value_type = std::pair<string, T>;
		return make_inline_enumerator<value_type>([this, &Key](size_t Index, value_type& Value)
		{
			return EnumValues(Key, static_cast<DWORD>(Index), Value.first, Value.second);
		});
	}

protected:
	GeneralConfig() = default;
};

class async_delete
{
public:
	virtual ~async_delete() = default;
	virtual void finish() = 0;
};

class HierarchicalConfig: public representable, virtual public async_delete, virtual public transactional, public conditional<HierarchicalConfig>
{
public:
	class key: public conditional<key>
	{
	public:
		explicit key(unsigned long long Key): m_Key(Key) {}

		unsigned long long get() const { return m_Key; }
		bool operator!() const noexcept { return m_Key == 0; }

	private:
		unsigned long long m_Key;
	};

	static key root_key() { return key(0); }

	virtual key CreateKey(const key& Root, const string_view& Name, const string* Description = nullptr) = 0;
	virtual key FindByName(const key& Root, const string_view& Name) const = 0;
	virtual bool SetKeyDescription(const key& Root, const string_view& Description) = 0;

	virtual bool SetValue(const key& Root, const string_view& Name, const string_view& Value) = 0;
	virtual bool SetValue(const key& Root, const string_view& Name, unsigned long long Value) = 0;
	virtual bool SetValue(const key& Root, const string_view& Name, const bytes_view& Value) = 0;

	virtual bool GetValue(const key& Root, const string_view& Name, unsigned long long& Value) const = 0;
	virtual bool GetValue(const key& Root, const string_view& Name, string& strValue) const = 0;
	virtual bool GetValue(const key& Root, const string_view& Name, bytes& Value) const = 0;
	bool GetValue(const key& Root, const string_view& Name, bytes&& Value) const { return GetValue(Root, Name, Value); }

	virtual bool DeleteKeyTree(const key& Key) = 0;
	virtual bool DeleteValue(const key& Root, const string_view& Name) = 0;
	virtual bool EnumKeys(const key& Root, DWORD Index, string& strName) const = 0;
	virtual bool EnumValues(const key& Root, DWORD Index, string& strName, int& Type) const = 0;
	virtual bool Flush() = 0;

	void KeysEnumerator(const key&&) const = delete;
	auto KeysEnumerator(const key& Root) const
	{
		using value_type = string;
		return make_inline_enumerator<value_type>([this, &Root](size_t Index, value_type& Value)
		{
			return EnumKeys(Root, static_cast<DWORD>(Index), Value);
		});
	}

	void ValuesEnumerator(const key&&) const = delete;
	auto ValuesEnumerator(const key& Root) const
	{
		using value_type = std::pair<string, int>;
		return make_inline_enumerator<value_type>([this, &Root](size_t Index, value_type& Value)
		{
			return EnumValues(Root, static_cast<DWORD>(Index), Value.first, Value.second);
		});
	}

protected:
	HierarchicalConfig() = default;
	virtual ~HierarchicalConfig() override = default;
};

namespace detail
{
	struct async_deleter
	{
		void operator()(async_delete* Ptr) const
		{
			Ptr->finish();
		}
	};
}

using HierarchicalConfigUniquePtr = std::unique_ptr<HierarchicalConfig, detail::async_deleter>;

class ColorsConfig: public representable, virtual public transactional
{
public:
	virtual ~ColorsConfig() override = default;
	virtual bool SetValue(const string_view& Name, const FarColor& Value) = 0;
	virtual bool GetValue(const string_view& Name, FarColor& Value) const = 0;

protected:
	ColorsConfig() = default;
};

class AssociationsConfig: public representable, virtual public transactional {

public:
	virtual ~AssociationsConfig() override = default;
	virtual bool EnumMasks(DWORD Index, unsigned long long *id, string &strMask) = 0;
	virtual bool EnumMasksForType(int Type, DWORD Index, unsigned long long *id, string &strMask) = 0;
	virtual bool GetMask(unsigned long long id, string &strMask) = 0;
	virtual bool GetDescription(unsigned long long id, string &strDescription) = 0;
	virtual bool GetCommand(unsigned long long id, int Type, string &strCommand, bool *Enabled=nullptr) = 0;
	virtual bool SetCommand(unsigned long long id, int Type, const string_view& Command, bool Enabled) = 0;
	virtual bool SwapPositions(unsigned long long id1, unsigned long long id2) = 0;
	virtual unsigned long long AddType(unsigned long long after_id, const string_view& Mask, const string_view& Description) = 0;
	virtual bool UpdateType(unsigned long long id, const string_view& Mask, const string_view& Description) = 0;
	virtual bool DelType(unsigned long long id) = 0;

	auto MasksEnumerator()
	{
		using value_type = std::pair<unsigned long long, string>;
		return make_inline_enumerator<value_type>([this](size_t Index, value_type& Value)
		{
			return EnumMasks(static_cast<DWORD>(Index), &Value.first, Value.second);
		});
	}

	auto TypedMasksEnumerator(int Type)
	{
		using value_type = std::pair<unsigned long long, string>;
		return make_inline_enumerator<value_type>([this, Type](size_t Index, value_type& Value)
		{
			return EnumMasksForType(Type, static_cast<DWORD>(Index), &Value.first, Value.second);
		});
	}

protected:
	AssociationsConfig() = default;
};

class PluginsCacheConfig: public representable, virtual public transactional
{
public:
	virtual ~PluginsCacheConfig() override = default;
	virtual unsigned long long CreateCache(const string_view& CacheName) = 0;
	virtual unsigned long long GetCacheID(const string_view& CacheName) const = 0;
	virtual bool DeleteCache(const string_view& CacheName) = 0;
	virtual bool IsPreload(unsigned long long id) const = 0;
	virtual string GetSignature(unsigned long long id) const = 0;
	virtual bool GetExportState(unsigned long long id, const string_view& ExportName) const = 0;
	virtual string GetGuid(unsigned long long id) const = 0;
	virtual string GetTitle(unsigned long long id) const = 0;
	virtual string GetAuthor(unsigned long long id) const = 0;
	virtual string GetDescription(unsigned long long id) const = 0;
	virtual bool GetMinFarVersion(unsigned long long id, VersionInfo *Version) const = 0;
	virtual bool GetVersion(unsigned long long id, VersionInfo *Version) const = 0;
	virtual bool GetDiskMenuItem(unsigned long long id, size_t index, string &Text, GUID& Guid) const = 0;
	virtual bool GetPluginsMenuItem(unsigned long long id, size_t index, string &Text, GUID& Guid) const = 0;
	virtual bool GetPluginsConfigMenuItem(unsigned long long id, size_t index, string &Text, GUID& Guid) const = 0;
	virtual string GetCommandPrefix(unsigned long long id) const = 0;
	virtual unsigned long long GetFlags(unsigned long long id) const = 0;
	virtual bool SetPreload(unsigned long long id, bool Preload) = 0;
	virtual bool SetSignature(unsigned long long id, const string_view& Signature) = 0;
	virtual bool SetDiskMenuItem(unsigned long long id, size_t index, const string_view& Text, const GUID& Guid) = 0;
	virtual bool SetPluginsMenuItem(unsigned long long id, size_t index, const string_view& Text, const GUID& Guid) = 0;
	virtual bool SetPluginsConfigMenuItem(unsigned long long id, size_t index, const string_view& Text, const GUID& Guid) = 0;
	virtual bool SetCommandPrefix(unsigned long long id, const string_view& Prefix) = 0;
	virtual bool SetFlags(unsigned long long id, unsigned long long Flags) = 0;
	virtual bool SetExportState(unsigned long long id, const string_view& ExportName, bool Exists) = 0;
	virtual bool SetMinFarVersion(unsigned long long id, const VersionInfo *Version) = 0;
	virtual bool SetVersion(unsigned long long id, const VersionInfo *Version) = 0;
	virtual bool SetGuid(unsigned long long id, const string_view& Guid) = 0;
	virtual bool SetTitle(unsigned long long id, const string_view& Title) = 0;
	virtual bool SetAuthor(unsigned long long id, const string_view& Author) = 0;
	virtual bool SetDescription(unsigned long long id, const string_view& Description) = 0;
	virtual bool EnumPlugins(DWORD index, string &CacheName) const = 0;
	virtual bool DiscardCache() = 0;
	virtual bool IsCacheEmpty() const = 0;

protected:
	PluginsCacheConfig() = default;
};

enum class hotkey_type: int
{
	drive_menu,
	plugins_menu,
	config_menu,
};

class PluginsHotkeysConfig: public representable, virtual public transactional
{
public:
	virtual ~PluginsHotkeysConfig() override = default;
	virtual bool HotkeysPresent(hotkey_type HotKeyType) = 0;
	virtual string GetHotkey(const string_view& PluginKey, const GUID& MenuGuid, hotkey_type HotKeyType) = 0;
	virtual bool SetHotkey(const string_view& PluginKey, const GUID& MenuGuid, hotkey_type HotKeyType, const string_view& HotKey) = 0;
	virtual bool DelHotkey(const string_view& PluginKey, const GUID& MenuGuid, hotkey_type HotKeyType) = 0;

protected:
	PluginsHotkeysConfig() = default;
};

enum history_record_type: int;

class HistoryConfig: public representable, virtual public transactional
{
public:
	virtual ~HistoryConfig() override = default;

	//command,view,edit,folder,dialog history
	virtual bool Enum(DWORD index, unsigned int TypeHistory, const string_view& HistoryName, unsigned long long& id, string& strName, history_record_type& Type, bool& Lock, os::chrono::time_point& Time, string& strGuid, string& strFile, string& strData, bool Reverse = false) = 0;
	virtual bool Delete(unsigned long long id) = 0;
	virtual bool DeleteAndAddAsync(unsigned long long DeleteId, unsigned int TypeHistory, const string_view& HistoryName, const string_view& strName, int Type, bool Lock, string &strGuid, string &strFile, string &strData) = 0;
	virtual bool DeleteOldUnlocked(unsigned int TypeHistory, const string_view& HistoryName, int DaysToKeep, int MinimumEntries) = 0;
	virtual bool EnumLargeHistories(DWORD index, unsigned int TypeHistory, int MinimumEntries, string& strHistoryName) = 0;
	virtual bool GetNewest(unsigned int TypeHistory, const string_view& HistoryName, string &strName) = 0;
	virtual bool Get(unsigned long long id, string &strName) = 0;
	virtual bool Get(unsigned long long id, string &strName, history_record_type& Type, string &strGuid, string &strFile, string &strData) = 0;
	virtual DWORD Count(unsigned int TypeHistory, const string_view& HistoryName) = 0;
	virtual bool FlipLock(unsigned long long id) = 0;
	virtual bool IsLocked(unsigned long long id) = 0;
	virtual bool DeleteAllUnlocked(unsigned int TypeHistory, const string_view& HistoryName) = 0;
	virtual unsigned long long GetNext(unsigned int TypeHistory, const string_view& HistoryName, unsigned long long id, string &strName) = 0;
	virtual unsigned long long GetPrev(unsigned int TypeHistory, const string_view& HistoryName, unsigned long long id, string &strName) = 0;
	virtual unsigned long long CyclicGetPrev(unsigned int TypeHistory, const string_view& HistoryName, unsigned long long id, string &strName) = 0;

	//view,edit file positions and bookmarks history
	virtual unsigned long long SetEditorPos(const string_view& Name, int Line, int LinePos, int ScreenLine, int LeftPos, uintptr_t CodePage) = 0;
	virtual unsigned long long GetEditorPos(const string_view& Name, int *Line, int *LinePos, int *ScreenLine, int *LeftPos, uintptr_t *CodePage) = 0;
	virtual bool SetEditorBookmark(unsigned long long id, size_t i, int Line, int LinePos, int ScreenLine, int LeftPos) = 0;
	virtual bool GetEditorBookmark(unsigned long long id, size_t i, int *Line, int *LinePos, int *ScreenLine, int *LeftPos) = 0;
	virtual unsigned long long SetViewerPos(const string_view& Name, long long FilePos, long long LeftPos, int Hex_Wrap, uintptr_t CodePage) = 0;
	virtual unsigned long long GetViewerPos(const string_view& Name, long long *FilePos, long long *LeftPos, int *Hex, uintptr_t *CodePage) = 0;
	virtual bool SetViewerBookmark(unsigned long long id, size_t i, long long FilePos, long long LeftPos) = 0;
	virtual bool GetViewerBookmark(unsigned long long id, size_t i, long long *FilePos, long long *LeftPos) = 0;
	virtual void DeleteOldPositions(int DaysToKeep, int MinimumEntries) = 0;

	struct enum_data
	{
		unsigned long long Id;
		string Name;
		history_record_type Type;
		bool Lock;
		os::chrono::time_point Time;
		string Guid;
		string File;
		string Data;
	};

	void Enumerator(unsigned int, const string&&, bool = false) = delete;
	auto Enumerator(unsigned int HistoryType, const string& HistoryName, bool Reverse = false)
	{
		using value_type = enum_data;
		return make_inline_enumerator<value_type>([this, HistoryType, &HistoryName, Reverse](size_t Index, value_type& Value)
		{
			return Enum(static_cast<DWORD>(Index), HistoryType, HistoryName, Value.Id, Value.Name, Value.Type, Value.Lock, Value.Time, Value.Guid, Value.File, Value.Data, Reverse);
		});
	}

	auto LargeHistoriesEnumerator(unsigned int HistoryType, int MinimumEntries)
	{
		using value_type = string;
		return make_inline_enumerator<value_type>([this, HistoryType, MinimumEntries](size_t Index, value_type& Value)
		{
			return EnumLargeHistories(static_cast<DWORD>(Index), HistoryType, MinimumEntries, Value);
		});
	}

protected:
	HistoryConfig() = default;
};

enum dbcheck: int;

class config_provider: noncopyable
{
public:
	enum class mode { m_default, m_import, m_export };

	explicit config_provider(mode Mode = mode::m_default);
	~config_provider();
	bool ShowProblems() const;
	bool ServiceMode(const string& File);

	void AsyncCall(const std::function<void()>& Routine);

	static void ClearPluginsCache();

	const auto& GeneralCfg() const { return m_GeneralCfg; }
	const auto& LocalGeneralCfg() const { return m_LocalGeneralCfg; }
	const auto& ColorsCfg() const { return m_ColorsCfg; }
	const auto& AssocConfig() const { return m_AssocConfig; }
	const auto& PlCacheCfg() const { return m_PlCacheCfg; }
	const auto& PlHotkeyCfg() const { return m_PlHotkeyCfg; }
	const auto& HistoryCfg() const { return m_HistoryCfg; }
	const auto& HistoryCfgMem() const { return m_HistoryCfgMem; }

	HierarchicalConfigUniquePtr CreatePluginsConfig(const string_view& guid, bool Local=false);
	HierarchicalConfigUniquePtr CreateFiltersConfig();
	HierarchicalConfigUniquePtr CreateHighlightConfig();
	HierarchicalConfigUniquePtr CreateShortcutsConfig();
	HierarchicalConfigUniquePtr CreatePanelModesConfig();

private:
	template<class T> void CheckAndImportDatabase(T* Database, const char* ImportNodeName, bool IsPlugin);
	template<class T> std::unique_ptr<T> CreateDatabase();
	template<class T> HierarchicalConfigUniquePtr CreateHierarchicalConfig(dbcheck DbId, const string& DbName, const char* ImportNodeName, bool IsLocal = false, bool IsPlugin = false);
	bool Import(const string& File);
	bool Export(const string& File);
	void TryImportDatabase(representable* p, const char* NodeName = nullptr, bool IsPlugin = false);
	void CheckDatabase(class SQLiteDb *pDb);

	int m_LoadResult;
	std::vector<os::thread> m_Threads;
	std::vector<string> m_Problems;
	std::unique_ptr<representation_source> m_TemplateSource;
	mode m_Mode;

	std::unique_ptr<GeneralConfig> m_GeneralCfg;
	std::unique_ptr<GeneralConfig> m_LocalGeneralCfg;
	std::unique_ptr<ColorsConfig> m_ColorsCfg;
	std::unique_ptr<AssociationsConfig> m_AssocConfig;
	std::unique_ptr<PluginsCacheConfig> m_PlCacheCfg;
	std::unique_ptr<PluginsHotkeysConfig> m_PlHotkeyCfg;
	std::unique_ptr<HistoryConfig> m_HistoryCfg;
	std::unique_ptr<HistoryConfig> m_HistoryCfgMem;

	BitFlags m_CheckedDb;
};

config_provider& ConfigProvider();

#endif // CONFIGDB_HPP_552309E5_DEA6_42FD_BD7B_0F59C839FE62
