// steam_api.dll for the Steam build of Street Fighter X Tekken. The 2012 exe imports SDK 1.19 flat
// functions and calls SDK 1.19 vtables, xlive.dll imports SDK 1.65. This dll answers the old ones
// itself and forwards every other export to the real 1.65 dll, shipped beside it as
// steam_api_real.dll (see steam_api_forwarders.h, link.exe rejects forwarders in a .def here).
#include <windows.h>

// Only the interface classes and version strings are used, nothing here links steam_api.lib.
#include <steam/steam_api.h>

#include "steam_api_forwarders.h"

#include <stdarg.h>
#include <stdio.h>

namespace {

HMODULE g_self = nullptr;
HMODULE g_real = nullptr;
FILE* g_log = nullptr;

typedef ESteamAPIInitResult(S_CALLTYPE* InitFn)(const char*, SteamErrMsg*);
typedef HSteamUser(S_CALLTYPE* GetUserFn)();
typedef void*(S_CALLTYPE* FindInterfaceFn)(HSteamUser, const char*);

InitFn g_realInit = nullptr;
GetUserFn g_realGetUser = nullptr;
FindInterfaceFn g_realFindInterface = nullptr;

ISteamApps* g_apps = nullptr;       // SDK 1.65 interfaces behind the shims.
ISteamFriends* g_friends = nullptr;

void Log(const char* format, ...)
{
	if (!g_log) {
		return;
	}
	SYSTEMTIME now;
	GetLocalTime(&now);
	fprintf(g_log, "%02u:%02u:%02u.%03u ", now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
	va_list args;
	va_start(args, format);
	vfprintf(g_log, format, args);
	va_end(args);
	fputc('\n', g_log);
	fflush(g_log);
}

void OpenLog()
{
	wchar_t path[MAX_PATH];
	if (!GetModuleFileNameW(g_self, path, MAX_PATH)) {
		return;
	}
	wchar_t* slash = wcsrchr(path, L'\\');
	if (!slash) {
		return;
	}
	wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"steam_api_shim.log");
	g_log = _wfopen(path, L"w");
}

bool LoadReal()
{
	if (g_real) {
		return true;
	}
	wchar_t path[MAX_PATH];
	if (!GetModuleFileNameW(g_self, path, MAX_PATH)) {
		return false;
	}
	wchar_t* slash = wcsrchr(path, L'\\');
	if (!slash) {
		return false;
	}
	wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"steam_api_real.dll");
	g_real = LoadLibraryW(path);
	if (!g_real) {
		Log("steam_api_real.dll failed to load (error %lu)", GetLastError());
		return false;
	}
	g_realInit = (InitFn)GetProcAddress(g_real, "SteamInternal_SteamAPI_Init");
	g_realGetUser = (GetUserFn)GetProcAddress(g_real, "SteamAPI_GetHSteamUser");
	g_realFindInterface = (FindInterfaceFn)GetProcAddress(g_real, "SteamInternal_FindOrCreateUserInterface");
	if (!g_realInit || !g_realGetUser || !g_realFindInterface) {
		Log("steam_api_real.dll lacks the 1.65 entry points");
		return false;
	}
	return true;
}

// The same list SteamAPI_InitEx builds inline in the 1.65 header.
const char* const kInterfaceVersions =
	STEAMUTILS_INTERFACE_VERSION "\0"
	STEAMNETWORKINGUTILS_INTERFACE_VERSION "\0"
	STEAMAPPS_INTERFACE_VERSION "\0"
	STEAMCONTROLLER_INTERFACE_VERSION "\0"
	STEAMFRIENDS_INTERFACE_VERSION "\0"
	STEAMHTMLSURFACE_INTERFACE_VERSION "\0"
	STEAMHTTP_INTERFACE_VERSION "\0"
	STEAMINPUT_INTERFACE_VERSION "\0"
	STEAMINVENTORY_INTERFACE_VERSION "\0"
	STEAMMATCHMAKINGSERVERS_INTERFACE_VERSION "\0"
	STEAMMATCHMAKING_INTERFACE_VERSION "\0"
	STEAMMUSIC_INTERFACE_VERSION "\0"
	STEAMNETWORKINGMESSAGES_INTERFACE_VERSION "\0"
	STEAMNETWORKINGSOCKETS_INTERFACE_VERSION "\0"
	STEAMNETWORKING_INTERFACE_VERSION "\0"
	STEAMPARENTALSETTINGS_INTERFACE_VERSION "\0"
	STEAMPARTIES_INTERFACE_VERSION "\0"
	STEAMREMOTEPLAY_INTERFACE_VERSION "\0"
	STEAMREMOTESTORAGE_INTERFACE_VERSION "\0"
	STEAMSCREENSHOTS_INTERFACE_VERSION "\0"
	STEAMUGC_INTERFACE_VERSION "\0"
	STEAMUSERSTATS_INTERFACE_VERSION "\0"
	STEAMUSER_INTERFACE_VERSION "\0"
	STEAMVIDEO_INTERFACE_VERSION "\0"
	"\0";

template <typename T>
T* Interface(T*& cached, const char* version)
{
	if (!cached && LoadReal()) {
		HSteamUser user = g_realGetUser();
		if (user) {
			cached = (T*)g_realFindInterface(user, version);
			Log("%s -> %p", version, cached);
		}
	}
	return cached;
}

ISteamApps* Apps() { return Interface(g_apps, STEAMAPPS_INTERFACE_VERSION); }
ISteamFriends* Friends() { return Interface(g_friends, STEAMFRIENDS_INTERFACE_VERSION); }

// Logs the first call of each vtable slot, so a run shows exactly what the exe reaches.
void Slot(const char* iface, int index, const char* name)
{
	static bool seen[2][80] = {};
	int which = iface[0] == 'A' ? 0 : 1;
	if (index < 80 && !seen[which][index]) {
		seen[which][index] = true;
		Log("ISteam%s slot %d (%s) first call", iface, index, name);
	}
}

// SDK 1.19 ISteamApps (STEAMAPPS_INTERFACE_VERSION005). Slot order is the header's.
struct ISteamApps005 {
	virtual bool BIsSubscribed() { Slot("Apps", 0, "BIsSubscribed"); return Apps() ? Apps()->BIsSubscribed() : false; }
	virtual bool BIsLowViolence() { Slot("Apps", 1, "BIsLowViolence"); return Apps() ? Apps()->BIsLowViolence() : false; }
	virtual bool BIsCybercafe() { Slot("Apps", 2, "BIsCybercafe"); return Apps() ? Apps()->BIsCybercafe() : false; }
	virtual bool BIsVACBanned() { Slot("Apps", 3, "BIsVACBanned"); return Apps() ? Apps()->BIsVACBanned() : false; }
	virtual const char* GetCurrentGameLanguage()
	{
		Slot("Apps", 4, "GetCurrentGameLanguage");
		const char* language = Apps() ? Apps()->GetCurrentGameLanguage() : "english";
		Log("GetCurrentGameLanguage -> %s", language);
		return language;
	}
	virtual const char* GetAvailableGameLanguages() { Slot("Apps", 5, "GetAvailableGameLanguages"); return Apps() ? Apps()->GetAvailableGameLanguages() : "english"; }
	virtual bool BIsSubscribedApp(AppId_t appId)
	{
		Slot("Apps", 6, "BIsSubscribedApp");
		bool owned = Apps() ? Apps()->BIsSubscribedApp(appId) : false;
		// Every DLC id the exe asks about, once, so an owner's run lists what it unlocked.
		static AppId_t seen[256];
		static int count = 0;
		bool logged = false;
		for (int i = 0; i < count && !logged; i++) {
			logged = seen[i] == appId;
		}
		if (!logged && count < 256) {
			seen[count++] = appId;
			Log("BIsSubscribedApp(%u) -> %d", appId, (int)owned);
		}
		return owned;
	}
	virtual bool BIsDlcInstalled(AppId_t appId) { Slot("Apps", 7, "BIsDlcInstalled"); return Apps() ? Apps()->BIsDlcInstalled(appId) : false; }
	virtual uint32 GetEarliestPurchaseUnixTime(AppId_t appId) { Slot("Apps", 8, "GetEarliestPurchaseUnixTime"); return Apps() ? Apps()->GetEarliestPurchaseUnixTime(appId) : 0; }
	virtual bool BIsSubscribedFromFreeWeekend() { Slot("Apps", 9, "BIsSubscribedFromFreeWeekend"); return Apps() ? Apps()->BIsSubscribedFromFreeWeekend() : false; }
	virtual int GetDLCCount() { Slot("Apps", 10, "GetDLCCount"); return Apps() ? Apps()->GetDLCCount() : 0; }
	virtual bool BGetDLCDataByIndex(int index, AppId_t* appId, bool* available, char* name, int nameSize) { Slot("Apps", 11, "BGetDLCDataByIndex"); return Apps() ? Apps()->BGetDLCDataByIndex(index, appId, available, name, nameSize) : false; }
	virtual void InstallDLC(AppId_t appId) { Slot("Apps", 12, "InstallDLC"); if (Apps()) Apps()->InstallDLC(appId); }
	virtual void UninstallDLC(AppId_t appId) { Slot("Apps", 13, "UninstallDLC"); if (Apps()) Apps()->UninstallDLC(appId); }
	virtual void RequestAppProofOfPurchaseKey(AppId_t appId) { Slot("Apps", 14, "RequestAppProofOfPurchaseKey"); if (Apps()) Apps()->RequestAppProofOfPurchaseKey(appId); }
};

// SDK 1.19 ISteamFriends (SteamFriends011). Slot order is the header's, the exe reaches slot 24.
struct ISteamFriends011 {
	virtual const char* GetPersonaName() { Slot("Friends", 0, "GetPersonaName"); return Friends() ? Friends()->GetPersonaName() : ""; }
	virtual void SetPersonaName(const char* name) { Slot("Friends", 1, "SetPersonaName"); } // Gone from 1.65.
	virtual EPersonaState GetPersonaState() { Slot("Friends", 2, "GetPersonaState"); return Friends() ? Friends()->GetPersonaState() : k_EPersonaStateOffline; }
	virtual int GetFriendCount(int flags) { Slot("Friends", 3, "GetFriendCount"); return Friends() ? Friends()->GetFriendCount(flags) : 0; }
	virtual CSteamID GetFriendByIndex(int index, int flags) { Slot("Friends", 4, "GetFriendByIndex"); return Friends() ? Friends()->GetFriendByIndex(index, flags) : CSteamID(); }
	virtual EFriendRelationship GetFriendRelationship(CSteamID id) { Slot("Friends", 5, "GetFriendRelationship"); return Friends() ? Friends()->GetFriendRelationship(id) : k_EFriendRelationshipNone; }
	virtual EPersonaState GetFriendPersonaState(CSteamID id) { Slot("Friends", 6, "GetFriendPersonaState"); return Friends() ? Friends()->GetFriendPersonaState(id) : k_EPersonaStateOffline; }
	virtual const char* GetFriendPersonaName(CSteamID id) { Slot("Friends", 7, "GetFriendPersonaName"); return Friends() ? Friends()->GetFriendPersonaName(id) : ""; }
	virtual bool GetFriendGamePlayed(CSteamID id, FriendGameInfo_t* info) { Slot("Friends", 8, "GetFriendGamePlayed"); return Friends() ? Friends()->GetFriendGamePlayed(id, info) : false; }
	virtual const char* GetFriendPersonaNameHistory(CSteamID id, int index) { Slot("Friends", 9, "GetFriendPersonaNameHistory"); return Friends() ? Friends()->GetFriendPersonaNameHistory(id, index) : ""; }
	virtual bool HasFriend(CSteamID id, int flags) { Slot("Friends", 10, "HasFriend"); return Friends() ? Friends()->HasFriend(id, flags) : false; }
	virtual int GetClanCount() { Slot("Friends", 11, "GetClanCount"); return Friends() ? Friends()->GetClanCount() : 0; }
	virtual CSteamID GetClanByIndex(int index) { Slot("Friends", 12, "GetClanByIndex"); return Friends() ? Friends()->GetClanByIndex(index) : CSteamID(); }
	virtual const char* GetClanName(CSteamID id) { Slot("Friends", 13, "GetClanName"); return Friends() ? Friends()->GetClanName(id) : ""; }
	virtual const char* GetClanTag(CSteamID id) { Slot("Friends", 14, "GetClanTag"); return Friends() ? Friends()->GetClanTag(id) : ""; }
	virtual bool GetClanActivityCounts(CSteamID id, int* online, int* inGame, int* chatting) { Slot("Friends", 15, "GetClanActivityCounts"); return Friends() ? Friends()->GetClanActivityCounts(id, online, inGame, chatting) : false; }
	virtual SteamAPICall_t DownloadClanActivityCounts(CSteamID* ids, int count) { Slot("Friends", 16, "DownloadClanActivityCounts"); return Friends() ? Friends()->DownloadClanActivityCounts(ids, count) : 0; }
	virtual int GetFriendCountFromSource(CSteamID source) { Slot("Friends", 17, "GetFriendCountFromSource"); return Friends() ? Friends()->GetFriendCountFromSource(source) : 0; }
	virtual CSteamID GetFriendFromSourceByIndex(CSteamID source, int index) { Slot("Friends", 18, "GetFriendFromSourceByIndex"); return Friends() ? Friends()->GetFriendFromSourceByIndex(source, index) : CSteamID(); }
	virtual bool IsUserInSource(CSteamID user, CSteamID source) { Slot("Friends", 19, "IsUserInSource"); return Friends() ? Friends()->IsUserInSource(user, source) : false; }
	virtual void SetInGameVoiceSpeaking(CSteamID user, bool speaking) { Slot("Friends", 20, "SetInGameVoiceSpeaking"); if (Friends()) Friends()->SetInGameVoiceSpeaking(user, speaking); }
	virtual void ActivateGameOverlay(const char* dialog) { Slot("Friends", 21, "ActivateGameOverlay"); if (Friends()) Friends()->ActivateGameOverlay(dialog); }
	virtual void ActivateGameOverlayToUser(const char* dialog, CSteamID id) { Slot("Friends", 22, "ActivateGameOverlayToUser"); if (Friends()) Friends()->ActivateGameOverlayToUser(dialog, id); }
	virtual void ActivateGameOverlayToWebPage(const char* url) { Slot("Friends", 23, "ActivateGameOverlayToWebPage"); if (Friends()) Friends()->ActivateGameOverlayToWebPage(url, k_EActivateGameOverlayToWebPageMode_Default); }
	virtual void ActivateGameOverlayToStore(AppId_t appId) { Slot("Friends", 24, "ActivateGameOverlayToStore"); if (Friends()) Friends()->ActivateGameOverlayToStore(appId, k_EOverlayToStoreFlag_None); }
	virtual void SetPlayedWith(CSteamID id) { Slot("Friends", 25, "SetPlayedWith"); if (Friends()) Friends()->SetPlayedWith(id); }
	virtual void ActivateGameOverlayInviteDialog(CSteamID lobby) { Slot("Friends", 26, "ActivateGameOverlayInviteDialog"); if (Friends()) Friends()->ActivateGameOverlayInviteDialog(lobby); }
	virtual int GetSmallFriendAvatar(CSteamID id) { Slot("Friends", 27, "GetSmallFriendAvatar"); return Friends() ? Friends()->GetSmallFriendAvatar(id) : 0; }
	virtual int GetMediumFriendAvatar(CSteamID id) { Slot("Friends", 28, "GetMediumFriendAvatar"); return Friends() ? Friends()->GetMediumFriendAvatar(id) : 0; }
	virtual int GetLargeFriendAvatar(CSteamID id) { Slot("Friends", 29, "GetLargeFriendAvatar"); return Friends() ? Friends()->GetLargeFriendAvatar(id) : 0; }
	virtual bool RequestUserInformation(CSteamID id, bool nameOnly) { Slot("Friends", 30, "RequestUserInformation"); return Friends() ? Friends()->RequestUserInformation(id, nameOnly) : false; }
	virtual SteamAPICall_t RequestClanOfficerList(CSteamID clan) { Slot("Friends", 31, "RequestClanOfficerList"); return Friends() ? Friends()->RequestClanOfficerList(clan) : 0; }
	virtual CSteamID GetClanOwner(CSteamID clan) { Slot("Friends", 32, "GetClanOwner"); return Friends() ? Friends()->GetClanOwner(clan) : CSteamID(); }
	virtual int GetClanOfficerCount(CSteamID clan) { Slot("Friends", 33, "GetClanOfficerCount"); return Friends() ? Friends()->GetClanOfficerCount(clan) : 0; }
	virtual CSteamID GetClanOfficerByIndex(CSteamID clan, int index) { Slot("Friends", 34, "GetClanOfficerByIndex"); return Friends() ? Friends()->GetClanOfficerByIndex(clan, index) : CSteamID(); }
	virtual uint32 GetUserRestrictions() { Slot("Friends", 35, "GetUserRestrictions"); return 0; } // Gone from 1.65.
	virtual bool SetRichPresence(const char* key, const char* value) { Slot("Friends", 36, "SetRichPresence"); return Friends() ? Friends()->SetRichPresence(key, value) : false; }
	virtual void ClearRichPresence() { Slot("Friends", 37, "ClearRichPresence"); if (Friends()) Friends()->ClearRichPresence(); }
	virtual const char* GetFriendRichPresence(CSteamID id, const char* key) { Slot("Friends", 38, "GetFriendRichPresence"); return Friends() ? Friends()->GetFriendRichPresence(id, key) : ""; }
	virtual int GetFriendRichPresenceKeyCount(CSteamID id) { Slot("Friends", 39, "GetFriendRichPresenceKeyCount"); return Friends() ? Friends()->GetFriendRichPresenceKeyCount(id) : 0; }
	virtual const char* GetFriendRichPresenceKeyByIndex(CSteamID id, int index) { Slot("Friends", 40, "GetFriendRichPresenceKeyByIndex"); return Friends() ? Friends()->GetFriendRichPresenceKeyByIndex(id, index) : ""; }
	virtual void RequestFriendRichPresence(CSteamID id) { Slot("Friends", 41, "RequestFriendRichPresence"); if (Friends()) Friends()->RequestFriendRichPresence(id); }
	virtual bool InviteUserToGame(CSteamID id, const char* connect) { Slot("Friends", 42, "InviteUserToGame"); return Friends() ? Friends()->InviteUserToGame(id, connect) : false; }
	virtual int GetCoplayFriendCount() { Slot("Friends", 43, "GetCoplayFriendCount"); return Friends() ? Friends()->GetCoplayFriendCount() : 0; }
	virtual CSteamID GetCoplayFriend(int index) { Slot("Friends", 44, "GetCoplayFriend"); return Friends() ? Friends()->GetCoplayFriend(index) : CSteamID(); }
	virtual int GetFriendCoplayTime(CSteamID id) { Slot("Friends", 45, "GetFriendCoplayTime"); return Friends() ? Friends()->GetFriendCoplayTime(id) : 0; }
	virtual AppId_t GetFriendCoplayGame(CSteamID id) { Slot("Friends", 46, "GetFriendCoplayGame"); return Friends() ? Friends()->GetFriendCoplayGame(id) : 0; }
	virtual SteamAPICall_t JoinClanChatRoom(CSteamID clan) { Slot("Friends", 47, "JoinClanChatRoom"); return Friends() ? Friends()->JoinClanChatRoom(clan) : 0; }
	virtual bool LeaveClanChatRoom(CSteamID clan) { Slot("Friends", 48, "LeaveClanChatRoom"); return Friends() ? Friends()->LeaveClanChatRoom(clan) : false; }
	virtual int GetClanChatMemberCount(CSteamID clan) { Slot("Friends", 49, "GetClanChatMemberCount"); return Friends() ? Friends()->GetClanChatMemberCount(clan) : 0; }
	virtual CSteamID GetChatMemberByIndex(CSteamID clan, int index) { Slot("Friends", 50, "GetChatMemberByIndex"); return Friends() ? Friends()->GetChatMemberByIndex(clan, index) : CSteamID(); }
	virtual bool SendClanChatMessage(CSteamID chat, const char* text) { Slot("Friends", 51, "SendClanChatMessage"); return Friends() ? Friends()->SendClanChatMessage(chat, text) : false; }
	virtual int GetClanChatMessage(CSteamID chat, int index, void* text, int textMax, EChatEntryType* type, CSteamID* chatter) { Slot("Friends", 52, "GetClanChatMessage"); return Friends() ? Friends()->GetClanChatMessage(chat, index, text, textMax, type, chatter) : 0; }
	virtual bool IsClanChatAdmin(CSteamID chat, CSteamID user) { Slot("Friends", 53, "IsClanChatAdmin"); return Friends() ? Friends()->IsClanChatAdmin(chat, user) : false; }
	virtual bool IsClanChatWindowOpenInSteam(CSteamID chat) { Slot("Friends", 54, "IsClanChatWindowOpenInSteam"); return Friends() ? Friends()->IsClanChatWindowOpenInSteam(chat) : false; }
	virtual bool OpenClanChatWindowInSteam(CSteamID chat) { Slot("Friends", 55, "OpenClanChatWindowInSteam"); return Friends() ? Friends()->OpenClanChatWindowInSteam(chat) : false; }
	virtual bool CloseClanChatWindowInSteam(CSteamID chat) { Slot("Friends", 56, "CloseClanChatWindowInSteam"); return Friends() ? Friends()->CloseClanChatWindowInSteam(chat) : false; }
	virtual bool SetListenForFriendsMessages(bool intercept) { Slot("Friends", 57, "SetListenForFriendsMessages"); return Friends() ? Friends()->SetListenForFriendsMessages(intercept) : false; }
	virtual bool ReplyToFriendMessage(CSteamID id, const char* text) { Slot("Friends", 58, "ReplyToFriendMessage"); return Friends() ? Friends()->ReplyToFriendMessage(id, text) : false; }
	virtual int GetFriendMessage(CSteamID id, int index, void* data, int size, EChatEntryType* type) { Slot("Friends", 59, "GetFriendMessage"); return Friends() ? Friends()->GetFriendMessage(id, index, data, size, type) : 0; }
	virtual SteamAPICall_t GetFollowerCount(CSteamID id) { Slot("Friends", 60, "GetFollowerCount"); return Friends() ? Friends()->GetFollowerCount(id) : 0; }
	virtual SteamAPICall_t IsFollowing(CSteamID id) { Slot("Friends", 61, "IsFollowing"); return Friends() ? Friends()->IsFollowing(id) : 0; }
	virtual SteamAPICall_t EnumerateFollowingList(uint32 start) { Slot("Friends", 62, "EnumerateFollowingList"); return Friends() ? Friends()->EnumerateFollowingList(start) : 0; }
};

ISteamApps005 g_apps005;
ISteamFriends011 g_friends011;

}

// Exported under the SDK 1.19 names through the .def file.
extern "C" {

bool S_CALLTYPE Shim_SteamAPI_Init()
{
	if (!LoadReal()) {
		return false;
	}
	SteamErrMsg error = {};
	ESteamAPIInitResult result = g_realInit(kInterfaceVersions, &error);
	Log("SteamAPI_Init -> %d%s%s", (int)result, result == k_ESteamAPIInitResult_OK ? "" : ": ", result == k_ESteamAPIInitResult_OK ? "" : error);
	return result == k_ESteamAPIInitResult_OK;
}

// Steam's shutdown races its networking thread while peer connections finish closing, the process
// exit that follows in every title tears it down safely.
void S_CALLTYPE Shim_SteamAPI_Shutdown()
{
	Log("SteamAPI_Shutdown ignored, the Steam API stays up until the process exits");
}

void* S_CALLTYPE Shim_SteamApps()
{
	return &g_apps005;
}

void* S_CALLTYPE Shim_SteamFriends()
{
	return &g_friends011;
}

}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) {
		g_self = instance;
		DisableThreadLibraryCalls(instance);
		OpenLog();
		Log("steam_api shim loaded (SDK 1.19 exports over 1.65)");
	}
	return TRUE;
}
