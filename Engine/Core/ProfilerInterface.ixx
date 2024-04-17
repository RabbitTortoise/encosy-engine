module;
export module EncosyEngine.ProfilerInterface;

import EncosyEngine.Profiler;
import <string>;

namespace
{
    Profiler EngineProfiler;
}

export namespace ProfilerInterface
{
    void CaptureTickStart()
    {
        EngineProfiler.CaptureTickStart();
    }

    void CaptureTickSubTiming(const std::string& timingName)
    {
        EngineProfiler.CaptureTickSubTiming(timingName);
    }

    void CaptureTickEnd()
    {
        EngineProfiler.CaptureTickEnd();
    }

    void PrepareToCapture(const size_t tickAmount)
    {
        EngineProfiler.PrepareToCapture(tickAmount);
    }

    void StartCapture()
    {
        EngineProfiler.StartCapture();
    }

    void SaveCapture()
    {
        EngineProfiler.SaveCapture();
    }
}

