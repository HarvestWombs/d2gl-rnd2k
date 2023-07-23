#include "pch.h"
#include "DllNotify.h"
#include "patch.h"
#include "helpers.h"

using namespace d2gl;

DllNotify::DllNotify() 
{
}

VOID CALLBACK DllNotify::LdrDllNotification(
	_In_      ULONG NotificationReason,
	_In_      PCLDR_DLL_NOTIFICATION_DATA NotificationData,
	_In_opt_  PVOID Context
)
{
	if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
	{
		// D2Expres.dll want load?
		if (lstrcmpiW(NotificationData->Loaded.BaseDllName->Buffer, L"d2expres.dll") == 0) {
			void* D2ExpresBase = NotificationData->Loaded.DllBase;

			// need to bypass patch that makes d2expres for Sven's glide3x.dll, to prevent crash
			uint8_t d2expres_new_code[] = { 0xEB, 0x34 };
			Patch::setBytes((uintptr_t)D2ExpresBase + 0x20D0, sizeof d2expres_new_code, d2expres_new_code);

			// this mod uses d2expres
			App.is_d2expres = true;
		}
	}
}

static PVOID cookie;
static DllNotify::PLDR_REGISTER_DLL_NOTIFICATION    p_LdrRegisterDllNotification;
static DllNotify::PLDR_UNREGISTER_DLL_NOTIFICATION  p_LdrUnRegisterDllNotification;

BOOL DllNotify::Init_Dllnotify()
{
	NTSTATUS status = 1;
	HMODULE ntdll = GetModuleHandle(L"ntdll.dll");

	p_LdrRegisterDllNotification = (PLDR_REGISTER_DLL_NOTIFICATION)GetProcAddress(ntdll, "LdrRegisterDllNotification");
	p_LdrUnRegisterDllNotification = (PLDR_UNREGISTER_DLL_NOTIFICATION)GetProcAddress(ntdll, "LdrUnRegisterDllNotification");

	if (p_LdrRegisterDllNotification)
	{
		status = p_LdrRegisterDllNotification(
			0, // must be zero
			LdrDllNotification,
			0, // context,
			&cookie
		);
	}

	return status == 0;
}


BOOL DllNotify::Uninit_Dllnotify()
{
	NTSTATUS status = 1;

	if (p_LdrUnRegisterDllNotification)
	{
		status = p_LdrUnRegisterDllNotification(cookie);
		cookie = 0;
	}

	return status == 0;
}
