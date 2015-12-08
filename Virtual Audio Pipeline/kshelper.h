/*
Module Name:
  kshelper.h

Abstract:
  Helper functions for msvad
*/
#ifndef __KSHELPER_H_
#define __KSHELPER_H_

#include <portcls.h>
#include <ksdebug.h>

PWAVEFORMATEX GetWaveFormatEx(IN PKSDATAFORMAT pDataFormat);

NTSTATUS PropertyHandler_BasicSupport(IN PPCPROPERTY_REQUEST PropertyRequest, IN ULONG Flags, IN DWORD PropTypeSetId);

NTSTATUS ValidatePropertyParams(IN PPCPROPERTY_REQUEST PropertyRequest, IN ULONG cbValueSize, IN ULONG cbInstanceSize = 0);
#endif
