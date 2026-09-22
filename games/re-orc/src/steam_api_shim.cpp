// steam_api.dll for the Steam build of Resident Evil: Operation Raccoon City. The 2012 exe imports
// the SDK 1.13 flat functions and calls SDK 1.13 vtables (ISteamApps004, ISteamFriends009,
// ISteamUtils005), xlive.dll imports SDK 1.65. This dll answers the old ones itself and forwards
// every other export to the real 1.65 dll, shipped beside it as steam_api_real.dll (see
// steam_api_forwarders.h, link.exe rejects forwarders in a .def here).
//
// Steam only appends to an interface, so a newer vtable's early slots match an older one. ISteamApps005
// and ISteamFriends011 therefore serve the exe's 004 and 009 slots unchanged. Only the slots the exe
// never reaches sit past the end and stay unused. ISteamUtils005 is written out because 1.65 dropped
// its RunFrame slot, so the old order can no longer forward slot for slot.
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
ISteamUtils* g_utils = nullptr;

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
ISteamUtils* Utils() { return Interface(g_utils, STEAMUTILS_INTERFACE_VERSION); }

// Logs the first call of each vtable slot, so a run shows exactly what the exe reaches.
void Slot(char iface, int index, const char* name)
{
	static bool seen[3][96] = {};
	int which = iface == 'A' ? 0 : iface == 'F' ? 1 : 2;
	if (index < 96 && !seen[which][index]) {
		seen[which][index] = true;
		Log("ISteam%s slot %d (%s) first call", iface == 'A' ? "Apps" : iface == 'F' ? "Friends" : "Utils", index, name);
	}
}

// SDK 1.13 ISteamApps (STEAMAPPS_INTERFACE_VERSION004). Slot order is the header's. Version 005 only
// appends RequestAppProofOfPurchaseKey, so this vtable also serves a 005 exe.
struct ISteamApps004 {
	virtual bool BIsSubscribed() { Slot('A', 0, "BIsSubscribed"); return Apps() ? Apps()->BIsSubscribed() : false; }
	virtual bool BIsLowViolence() { Slot('A', 1, "BIsLowViolence"); return Apps() ? Apps()->BIsLowViolence() : false; }
	virtual bool BIsCybercafe() { Slot('A', 2, "BIsCybercafe"); return Apps() ? Apps()->BIsCybercafe() : false; }
	virtual bool BIsVACBanned() { Slot('A', 3, "BIsVACBanned"); return Apps() ? Apps()->BIsVACBanned() : false; }
	virtual const char* GetCurrentGameLanguage()
	{
		Slot('A', 4, "GetCurrentGameLanguage");
		const char* language = Apps() ? Apps()->GetCurrentGameLanguage() : "english";
		Log("GetCurrentGameLanguage -> %s", language);
		return language;
	}
	virtual const char* GetAvailableGameLanguages() { Slot('A', 5, "GetAvailableGameLanguages"); return Apps() ? Apps()->GetAvailableGameLanguages() : "english"; }
	virtual bool BIsSubscribedApp(AppId_t appId)
	{
		Slot('A', 6, "BIsSubscribedApp");
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
	virtual bool BIsDlcInstalled(AppId_t appId) { Slot('A', 7, "BIsDlcInstalled"); return Apps() ? Apps()->BIsDlcInstalled(appId) : false; }
	virtual uint32 GetEarliestPurchaseUnixTime(AppId_t appId) { Slot('A', 8, "GetEarliestPurchaseUnixTime"); return Apps() ? Apps()->GetEarliestPurchaseUnixTime(appId) : 0; }
	virtual bool BIsSubscribedFromFreeWeekend() { Slot('A', 9, "BIsSubscribedFromFreeWeekend"); return Apps() ? Apps()->BIsSubscribedFromFreeWeekend() : false; }
	virtual int GetDLCCount() { Slot('A', 10, "GetDLCCount"); return Apps() ? Apps()->GetDLCCount() : 0; }
	virtual bool BGetDLCDataByIndex(int index, AppId_t* appId, bool* available, char* name, int nameSize) { Slot('A', 11, "BGetDLCDataByIndex"); return Apps() ? Apps()->BGetDLCDataByIndex(index, appId, available, name, nameSize) : false; }
	virtual void InstallDLC(AppId_t appId) { Slot('A', 12, "InstallDLC"); if (Apps()) Apps()->InstallDLC(appId); }
	virtual void UninstallDLC(AppId_t appId) { Slot('A', 13, "UninstallDLC"); if (Apps()) Apps()->UninstallDLC(appId); }
	virtual void RequestAppProofOfPurchaseKey(AppId_t appId) { Slot('A', 14, "RequestAppProofOfPurchaseKey"); if (Apps()) Apps()->RequestAppProofOfPurchaseKey(appId); }
};

// SDK 1.13 ISteamFriends (STEAMFRIENDS_INTERFACE_VERSION009). The exe indexes the 009 slots. 010 and
// 011 only append (clan chat, messaging, following), so those unused slots sit past slot 46 here.
struct ISteamFriends009 {
	virtual const char* GetPersonaName() { Slot('F', 0, "GetPersonaName"); return Friends() ? Friends()->GetPersonaName() : ""; }
	virtual void SetPersonaName(const char* name) { Slot('F', 1, "SetPersonaName"); } // Gone from 1.65.
	virtual EPersonaState GetPersonaState() { Slot('F', 2, "GetPersonaState"); return Friends() ? Friends()->GetPersonaState() : k_EPersonaStateOffline; }
	virtual int GetFriendCount(int flags) { Slot('F', 3, "GetFriendCount"); return Friends() ? Friends()->GetFriendCount(flags) : 0; }
	virtual CSteamID GetFriendByIndex(int index, int flags) { Slot('F', 4, "GetFriendByIndex"); return Friends() ? Friends()->GetFriendByIndex(index, flags) : CSteamID(); }
	virtual EFriendRelationship GetFriendRelationship(CSteamID id) { Slot('F', 5, "GetFriendRelationship"); return Friends() ? Friends()->GetFriendRelationship(id) : k_EFriendRelationshipNone; }
	virtual EPersonaState GetFriendPersonaState(CSteamID id) { Slot('F', 6, "GetFriendPersonaState"); return Friends() ? Friends()->GetFriendPersonaState(id) : k_EPersonaStateOffline; }
	virtual const char* GetFriendPersonaName(CSteamID id) { Slot('F', 7, "GetFriendPersonaName"); return Friends() ? Friends()->GetFriendPersonaName(id) : ""; }
	virtual bool GetFriendGamePlayed(CSteamID id, FriendGameInfo_t* info) { Slot('F', 8, "GetFriendGamePlayed"); return Friends() ? Friends()->GetFriendGamePlayed(id, info) : false; }
	virtual const char* GetFriendPersonaNameHistory(CSteamID id, int index) { Slot('F', 9, "GetFriendPersonaNameHistory"); return Friends() ? Friends()->GetFriendPersonaNameHistory(id, index) : ""; }
	virtual bool HasFriend(CSteamID id, int flags) { Slot('F', 10, "HasFriend"); return Friends() ? Friends()->HasFriend(id, flags) : false; }
	virtual int GetClanCount() { Slot('F', 11, "GetClanCount"); return Friends() ? Friends()->GetClanCount() : 0; }
	virtual CSteamID GetClanByIndex(int index) { Slot('F', 12, "GetClanByIndex"); return Friends() ? Friends()->GetClanByIndex(index) : CSteamID(); }
	virtual const char* GetClanName(CSteamID id) { Slot('F', 13, "GetClanName"); return Friends() ? Friends()->GetClanName(id) : ""; }
	virtual const char* GetClanTag(CSteamID id) { Slot('F', 14, "GetClanTag"); return Friends() ? Friends()->GetClanTag(id) : ""; }
	virtual bool GetClanActivityCounts(CSteamID id, int* online, int* inGame, int* chatting) { Slot('F', 15, "GetClanActivityCounts"); return Friends() ? Friends()->GetClanActivityCounts(id, online, inGame, chatting) : false; }
	virtual SteamAPICall_t DownloadClanActivityCounts(CSteamID* ids, int count) { Slot('F', 16, "DownloadClanActivityCounts"); return Friends() ? Friends()->DownloadClanActivityCounts(ids, count) : 0; }
	virtual int GetFriendCountFromSource(CSteamID source) { Slot('F', 17, "GetFriendCountFromSource"); return Friends() ? Friends()->GetFriendCountFromSource(source) : 0; }
	virtual CSteamID GetFriendFromSourceByIndex(CSteamID source, int index) { Slot('F', 18, "GetFriendFromSourceByIndex"); return Friends() ? Friends()->GetFriendFromSourceByIndex(source, index) : CSteamID(); }
	virtual bool IsUserInSource(CSteamID user, CSteamID source) { Slot('F', 19, "IsUserInSource"); return Friends() ? Friends()->IsUserInSource(user, source) : false; }
	virtual void SetInGameVoiceSpeaking(CSteamID user, bool speaking) { Slot('F', 20, "SetInGameVoiceSpeaking"); if (Friends()) Friends()->SetInGameVoiceSpeaking(user, speaking); }
	virtual void ActivateGameOverlay(const char* dialog) { Slot('F', 21, "ActivateGameOverlay"); if (Friends()) Friends()->ActivateGameOverlay(dialog); }
	virtual void ActivateGameOverlayToUser(const char* dialog, CSteamID id) { Slot('F', 22, "ActivateGameOverlayToUser"); if (Friends()) Friends()->ActivateGameOverlayToUser(dialog, id); }
	virtual void ActivateGameOverlayToWebPage(const char* url) { Slot('F', 23, "ActivateGameOverlayToWebPage"); if (Friends()) Friends()->ActivateGameOverlayToWebPage(url, k_EActivateGameOverlayToWebPageMode_Default); }
	virtual void ActivateGameOverlayToStore(AppId_t appId) { Slot('F', 24, "ActivateGameOverlayToStore"); if (Friends()) Friends()->ActivateGameOverlayToStore(appId, k_EOverlayToStoreFlag_None); }
	virtual void SetPlayedWith(CSteamID id) { Slot('F', 25, "SetPlayedWith"); if (Friends()) Friends()->SetPlayedWith(id); }
	virtual void ActivateGameOverlayInviteDialog(CSteamID lobby) { Slot('F', 26, "ActivateGameOverlayInviteDialog"); if (Friends()) Friends()->ActivateGameOverlayInviteDialog(lobby); }
	virtual int GetSmallFriendAvatar(CSteamID id) { Slot('F', 27, "GetSmallFriendAvatar"); return Friends() ? Friends()->GetSmallFriendAvatar(id) : 0; }
	virtual int GetMediumFriendAvatar(CSteamID id) { Slot('F', 28, "GetMediumFriendAvatar"); return Friends() ? Friends()->GetMediumFriendAvatar(id) : 0; }
	virtual int GetLargeFriendAvatar(CSteamID id) { Slot('F', 29, "GetLargeFriendAvatar"); return Friends() ? Friends()->GetLargeFriendAvatar(id) : 0; }
	virtual bool RequestUserInformation(CSteamID id, bool nameOnly) { Slot('F', 30, "RequestUserInformation"); return Friends() ? Friends()->RequestUserInformation(id, nameOnly) : false; }
	virtual SteamAPICall_t RequestClanOfficerList(CSteamID clan) { Slot('F', 31, "RequestClanOfficerList"); return Friends() ? Friends()->RequestClanOfficerList(clan) : 0; }
	virtual CSteamID GetClanOwner(CSteamID clan) { Slot('F', 32, "GetClanOwner"); return Friends() ? Friends()->GetClanOwner(clan) : CSteamID(); }
	virtual int GetClanOfficerCount(CSteamID clan) { Slot('F', 33, "GetClanOfficerCount"); return Friends() ? Friends()->GetClanOfficerCount(clan) : 0; }
	virtual CSteamID GetClanOfficerByIndex(CSteamID clan, int index) { Slot('F', 34, "GetClanOfficerByIndex"); return Friends() ? Friends()->GetClanOfficerByIndex(clan, index) : CSteamID(); }
	virtual uint32 GetUserRestrictions() { Slot('F', 35, "GetUserRestrictions"); return 0; } // Gone from 1.65.
	virtual bool SetRichPresence(const char* key, const char* value) { Slot('F', 36, "SetRichPresence"); return Friends() ? Friends()->SetRichPresence(key, value) : false; }
	virtual void ClearRichPresence() { Slot('F', 37, "ClearRichPresence"); if (Friends()) Friends()->ClearRichPresence(); }
	virtual const char* GetFriendRichPresence(CSteamID id, const char* key) { Slot('F', 38, "GetFriendRichPresence"); return Friends() ? Friends()->GetFriendRichPresence(id, key) : ""; }
	virtual int GetFriendRichPresenceKeyCount(CSteamID id) { Slot('F', 39, "GetFriendRichPresenceKeyCount"); return Friends() ? Friends()->GetFriendRichPresenceKeyCount(id) : 0; }
	virtual const char* GetFriendRichPresenceKeyByIndex(CSteamID id, int index) { Slot('F', 40, "GetFriendRichPresenceKeyByIndex"); return Friends() ? Friends()->GetFriendRichPresenceKeyByIndex(id, index) : ""; }
	virtual void RequestFriendRichPresence(CSteamID id) { Slot('F', 41, "RequestFriendRichPresence"); if (Friends()) Friends()->RequestFriendRichPresence(id); }
	virtual bool InviteUserToGame(CSteamID id, const char* connect) { Slot('F', 42, "InviteUserToGame"); return Friends() ? Friends()->InviteUserToGame(id, connect) : false; }
	virtual int GetCoplayFriendCount() { Slot('F', 43, "GetCoplayFriendCount"); return Friends() ? Friends()->GetCoplayFriendCount() : 0; }
	virtual CSteamID GetCoplayFriend(int index) { Slot('F', 44, "GetCoplayFriend"); return Friends() ? Friends()->GetCoplayFriend(index) : CSteamID(); }
	virtual int GetFriendCoplayTime(CSteamID id) { Slot('F', 45, "GetFriendCoplayTime"); return Friends() ? Friends()->GetFriendCoplayTime(id) : 0; }
	virtual AppId_t GetFriendCoplayGame(CSteamID id) { Slot('F', 46, "GetFriendCoplayGame"); return Friends() ? Friends()->GetFriendCoplayGame(id) : 0; }
};

// SDK 1.13 ISteamUtils (STEAMUTILS_INTERFACE_VERSION005). Written out because 1.65 dropped RunFrame
// (old slot 14), so each old slot maps to the 1.65 method by name, not by position.
struct ISteamUtils005 {
	virtual uint32 GetSecondsSinceAppActive() { Slot('U', 0, "GetSecondsSinceAppActive"); return Utils() ? Utils()->GetSecondsSinceAppActive() : 0; }
	virtual uint32 GetSecondsSinceComputerActive() { Slot('U', 1, "GetSecondsSinceComputerActive"); return Utils() ? Utils()->GetSecondsSinceComputerActive() : 0; }
	virtual EUniverse GetConnectedUniverse() { Slot('U', 2, "GetConnectedUniverse"); return Utils() ? Utils()->GetConnectedUniverse() : k_EUniverseInvalid; }
	virtual uint32 GetServerRealTime() { Slot('U', 3, "GetServerRealTime"); return Utils() ? Utils()->GetServerRealTime() : 0; }
	virtual const char* GetIPCountry() { Slot('U', 4, "GetIPCountry"); return Utils() ? Utils()->GetIPCountry() : ""; }
	virtual bool GetImageSize(int image, uint32* w, uint32* h) { Slot('U', 5, "GetImageSize"); return Utils() ? Utils()->GetImageSize(image, w, h) : false; }
	virtual bool GetImageRGBA(int image, uint8* dest, int destSize) { Slot('U', 6, "GetImageRGBA"); return Utils() ? Utils()->GetImageRGBA(image, dest, destSize) : false; }
	virtual bool GetCSERIPPort(uint32* ip, uint16* port) { Slot('U', 7, "GetCSERIPPort"); return false; } // Gone from 1.65.
	virtual uint8 GetCurrentBatteryPower() { Slot('U', 8, "GetCurrentBatteryPower"); return Utils() ? Utils()->GetCurrentBatteryPower() : 255; }
	virtual uint32 GetAppID() { Slot('U', 9, "GetAppID"); return Utils() ? Utils()->GetAppID() : 0; }
	virtual void SetOverlayNotificationPosition(ENotificationPosition pos) { Slot('U', 10, "SetOverlayNotificationPosition"); if (Utils()) Utils()->SetOverlayNotificationPosition(pos); }
	virtual bool IsAPICallCompleted(SteamAPICall_t call, bool* failed) { Slot('U', 11, "IsAPICallCompleted"); return Utils() ? Utils()->IsAPICallCompleted(call, failed) : false; }
	virtual ESteamAPICallFailure GetAPICallFailureReason(SteamAPICall_t call) { Slot('U', 12, "GetAPICallFailureReason"); return Utils() ? Utils()->GetAPICallFailureReason(call) : k_ESteamAPICallFailureNone; }
	virtual bool GetAPICallResult(SteamAPICall_t call, void* result, int size, int expected, bool* failed) { Slot('U', 13, "GetAPICallResult"); return Utils() ? Utils()->GetAPICallResult(call, result, size, expected, failed) : false; }
	virtual void RunFrame() { Slot('U', 14, "RunFrame"); } // Gone from 1.65; the process pumps through SteamAPI_RunCallbacks.
	virtual uint32 GetIPCCallCount() { Slot('U', 15, "GetIPCCallCount"); return Utils() ? Utils()->GetIPCCallCount() : 0; }
	virtual void SetWarningMessageHook(SteamAPIWarningMessageHook_t hook) { Slot('U', 16, "SetWarningMessageHook"); if (Utils()) Utils()->SetWarningMessageHook(hook); }
	virtual bool IsOverlayEnabled() { Slot('U', 17, "IsOverlayEnabled"); return Utils() ? Utils()->IsOverlayEnabled() : false; }
	virtual bool BOverlayNeedsPresent() { Slot('U', 18, "BOverlayNeedsPresent"); return Utils() ? Utils()->BOverlayNeedsPresent() : false; }
	virtual SteamAPICall_t CheckFileSignature(const char* fileName) { Slot('U', 19, "CheckFileSignature"); return Utils() ? Utils()->CheckFileSignature(fileName) : 0; }
	virtual bool ShowGamepadTextInput(EGamepadTextInputMode mode, EGamepadTextInputLineMode lineMode, const char* description, uint32 charMax, const char* existingText) { Slot('U', 20, "ShowGamepadTextInput"); return Utils() ? Utils()->ShowGamepadTextInput(mode, lineMode, description, charMax, existingText) : false; }
	virtual uint32 GetEnteredGamepadTextLength() { Slot('U', 21, "GetEnteredGamepadTextLength"); return Utils() ? Utils()->GetEnteredGamepadTextLength() : 0; }
	virtual bool GetEnteredGamepadTextInput(char* text, uint32 textMax) { Slot('U', 22, "GetEnteredGamepadTextInput"); return Utils() ? Utils()->GetEnteredGamepadTextInput(text, textMax) : false; }
};

ISteamApps004 g_apps004;
ISteamFriends009 g_friends009;
ISteamUtils005 g_utils005;

}

// Exported under the SDK 1.13 names through the .def file.
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
	return &g_apps004;
}

void* S_CALLTYPE Shim_SteamFriends()
{
	return &g_friends009;
}

void* S_CALLTYPE Shim_SteamUtils()
{
	return &g_utils005;
}

}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) {
		g_self = instance;
		DisableThreadLibraryCalls(instance);
		OpenLog();
		Log("steam_api shim loaded (SDK 1.13 exports over 1.65)");
	}
	return TRUE;
}
