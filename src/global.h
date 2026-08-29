/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#define ApplicationServiceName  L"winlogfwd"
#define ApplicationDisplayName  L"Windows Log Forwarder"

#define MinQueueSize                2
#define MaxQueueSize                512

#define MinUserCacheSize            0
#define MaxUserCacheSize            8192

#define MinSystemDataBufferSize     2048
#define MaxSystemDataBufferSize     (1024*1024)

#define MinEventDataBufferSize      2048
#define MaxEventDataBufferSize      (1024*1024)

#define MinMaxSyslogMsgLength       480
#define MinMaxSDLength              32

