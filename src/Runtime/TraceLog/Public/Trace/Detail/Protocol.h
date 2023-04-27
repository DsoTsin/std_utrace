// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/TraceLog/Public/Trace/Config.h"

////////////////////////////////////////////////////////////////////////////////
#if UE_TRACE_ENABLED
#	define TRACE_PRIVATE_PROTOCOL_6
#endif

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable : 4200) // non-standard zero-sized array
#endif

#include "Protocols/Protocol0.h"
#include "Protocols/Protocol1.h"
#include "Protocols/Protocol2.h"
#include "Protocols/Protocol3.h"
#include "Protocols/Protocol4.h"
#include "Protocols/Protocol5.h"
#include "Protocols/Protocol6.h"

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif
