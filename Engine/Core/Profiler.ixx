// Currently this file generates a .csv file that can be used to analyze performance.

module;
#include <fmt/core.h>
#include <fmt/os.h>

export module EncosyEngine.Profiler;

import <chrono>;
import <string>;
import <map>;
import <vector>;


struct Tick
{
    std::chrono::steady_clock::duration startTime;
	std::vector<std::pair<std::string, std::chrono::steady_clock::duration>> subTimes;
    std::chrono::steady_clock::duration endTime;
};

export class Profiler
{
public:
    Profiler() {}
    ~Profiler() {}

    void CaptureTickStart()
    {
        if (!CaptureOngoing_) { return; }
        auto timePoint = (MainClock_.now() - CaptureStart);
        CaptureTicks_[CurrentTick_].startTime = timePoint;
    }

    void CaptureTickSubTiming(std::string timingName)
    {
        if (!CaptureOngoing_) { return; }
        auto timePoint = (MainClock_.now() - CaptureStart);
        CaptureTicks_[CurrentTick_].subTimes.push_back(std::make_pair(timingName, timePoint));
    }

    void CaptureTickEnd()
    {
        if (!CaptureOngoing_) { return; }
        auto timePoint = (MainClock_.now() - CaptureStart);
        CaptureTicks_[CurrentTick_].endTime = timePoint;
        CurrentTick_++;
        if(CurrentTick_ == CaptureTicksTotal)
        {
            CaptureOngoing_ = false;
            SaveCapture();
        }
    }

    void PrepareToCapture(const size_t tickAmount)
    {
        CaptureTicks_.clear();
        CaptureTicks_.resize(tickAmount);
        CaptureTicksTotal = tickAmount;
        CurrentTick_ = 0;
        CaptureOngoing_ = false;
    }

    void StartCapture()
    {
        fmt::println("Profiler capture started.");
        CaptureStart = MainClock_.now();
        CaptureOngoing_ = true;
    }

    void SaveCapture()
    {
        fmt::println("Saving Profiler capture results.");
        auto out = fmt::output_file("Results.csv");
        out.print("All Ticks:");
        double total = 0;

        // First row, all update tick durations
        for (auto tick : CaptureTicks_)
        {
            double tickTime = std::chrono::duration<double>((tick.endTime - tick.startTime)).count();
            total += tickTime;
            out.print(",{}", tickTime);
        }
        out.print("\n");

        // Collect all subtimings to printable form
        std::vector<std::pair<std::string, std::string>> subTimings;
        std::vector<std::pair<std::string, double>> subTimingAverages;

        for (auto tick : CaptureTicks_)
        {
            auto previousSubTime = std::chrono::duration<double>(tick.startTime);
            for (auto subTime : tick.subTimes)
            {

                auto subTickTimePoint = std::chrono::duration<double>(subTime.second);
                double subTickTime = (subTickTimePoint - previousSubTime).count();
               
                auto it = std::ranges::find(subTimings, subTime.first, &std::pair<std::string, std::string>::first);
                if(it == subTimings.end())
                {
                    subTimings.push_back(std::make_pair(subTime.first, std::format("{}", subTickTime)));
                    subTimingAverages.push_back(std::make_pair(subTime.first, subTickTime));
                }
                else
                {
                    it->second += ",";
                    it->second += std::format("{}", subTickTime);
                    auto it2 = std::ranges::find(subTimingAverages, subTime.first, &std::pair<std::string, double>::first);
                    it2->second += subTickTime;
                }
                previousSubTime = std::chrono::duration<double>(subTime.second);
            }
        }

        // Calculate averages for each subTime
        for (auto& subTime : subTimingAverages)
        {
            subTime.second /= CaptureTicksTotal;
        }

        // Second row, average of all updates
        double average = total / CaptureTicksTotal;
        out.print("Tick average,{},", average);
        out.print("\n");

        // Titles
        out.print("System Batches,Batch Average,Every Recorded Subtime\n");

        // Third row downwards, all subtimings recorded
        for (auto subTime : subTimings)
        {
            auto it = std::ranges::find(subTimingAverages, subTime.first, &std::pair<std::string, double>::first);
            out.print("{},{},{}", subTime.first, it->second, subTime.second);
            out.print(",");
            out.print("\n");
        }
        
        out.close();
    }

private:
    std::chrono::steady_clock MainClock_;
    std::chrono::steady_clock::time_point CaptureStart;
    size_t CurrentTick_ = 0;
    size_t CaptureTicksTotal = 0;
    std::vector<Tick> CaptureTicks_;
    bool CaptureOngoing_ = false;
};
