#pragma once
#include <Arduino.h>
#include <vector>

struct ScheduleItem {
    String time;
    String name;
};

struct ScheduleDay {
    int dayOffset; // 0 = Today, 1 = Tomorrow, 2..4 = Next days (up to 5 days max)
    String title;  // "Schedule (today)", "Schedule (Tomorrow)", "Schedule (Wed)"
    std::vector<ScheduleItem> items;
};

class ScheduleManager {
public:
    ScheduleManager();

    void init();
    void loop();

    // Physical gesture handlers
    void handleTap();
    void handleShake();

    // Queries
    bool hasIcs() const { return _hasIcs; }
    bool hasEvents() const;
    String getCurrentDayTitle() const;
    std::vector<ScheduleItem> getCurrentPageItems() const;

    int getCurrentDayIndex() const { return _currentDayIndex; }
    int getCurrentSubPageIndex() const { return _currentSubPage; }
    int getTotalDays() const { return (int)_days.size(); }

    // ICS Management
    void setIcsUrl(const String& url);
    String getIcsUrl() const { return _icsUrl; }
    void setIcsContent(const String& content);
    void clearIcs();
    void loadSavedIcs();

    // Sample data loader for testing
    void loadSampleSchedule();

private:
    void parseIcs(const String& icsContent);
    void refreshDayTitles();
    int getDaysFromCivil(int y, int m, int d);

    String _icsUrl;
    bool _hasIcs;
    std::vector<ScheduleDay> _days;
    int _currentDayIndex;
    int _currentSubPage;
};

extern ScheduleManager schedule;
