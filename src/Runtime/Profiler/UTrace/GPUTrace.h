#pragma once

#include "Runtime/TraceLog/Public/Trace/Config.h"
#include "Runtime/TraceLog/Public/Trace/Detail/LogScope.h"

struct FGPUTimingCalibrationTimestamp
{
	UInt64 GPUMicroseconds = 0;
	UInt64 CPUMicroseconds = 0;
};

struct FGpuProfilerTrace
{
	static void BeginFrame(struct FGPUTimingCalibrationTimestamp& Calibration);
	static void SpecifyEventByName(const core::string& Name);
	static void BeginEventByName(const core::string& Name, UInt32 FrameNumber, UInt64 TimestampMicroseconds);
	static void EndEvent(UInt64 TimestampMicroseconds);
	static void EndFrame(UInt32 GPUIndex);
	static void Deinitialize();
};
