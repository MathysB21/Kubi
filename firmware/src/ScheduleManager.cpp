#include "ScheduleManager.h"
#include <Preferences.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include <sstream>

#if defined(ESP32)
#include <LittleFS.h>
#else
#include <fstream>
#endif

ScheduleManager schedule;

static const char* const DAY_NAMES[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

ScheduleManager::ScheduleManager()
    : _icsUrl(""),
      _hasIcs(false),
      _currentDayIndex(0),
      _currentSubPage(0) {
    _days.resize(5);
    for (int i = 0; i < 5; i++) {
        _days[i].dayOffset = i;
    }
    refreshDayTitles();
}

int ScheduleManager::getDaysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

void ScheduleManager::refreshDayTitles() {
    struct tm timeinfo;
    int nowWday = 0;
    if (getLocalTime(&timeinfo)) {
        nowWday = timeinfo.tm_wday;
    }

    if (_days.size() < 5) _days.resize(5);

    for (size_t i = 0; i < _days.size() && i < 5; i++) {
        _days[i].dayOffset = (int)i;
        if (i == 0) {
            _days[i].title = "Schedule (today)";
        } else if (i == 1) {
            _days[i].title = "Schedule (Tomorrow)";
        } else {
            int wday = (nowWday + (int)i) % 7;
            if (wday < 0) wday += 7;
            _days[i].title = "Schedule (" + String(DAY_NAMES[wday]) + ")";
        }
    }
}

void ScheduleManager::init() {
    refreshDayTitles();
    loadSavedIcs();
}

void ScheduleManager::loop() {
    // Background refresh or day transition check can be added if needed
}

bool ScheduleManager::hasEvents() const {
    if (!_hasIcs) return false;
    for (const auto& day : _days) {
        if (!day.items.empty()) return true;
    }
    return false;
}

String ScheduleManager::getCurrentDayTitle() const {
    if (_days.empty() || _currentDayIndex < 0 || _currentDayIndex >= (int)_days.size()) {
        return "Schedule (today)";
    }
    return _days[_currentDayIndex].title;
}

std::vector<ScheduleItem> ScheduleManager::getCurrentPageItems() const {
    std::vector<ScheduleItem> pageItems;
    if (_days.empty() || _currentDayIndex < 0 || _currentDayIndex >= (int)_days.size()) {
        return pageItems;
    }

    const auto& allItems = _days[_currentDayIndex].items;
    int startIdx = _currentSubPage * 4;
    for (int i = startIdx; i < (int)allItems.size() && i < startIdx + 4; i++) {
        pageItems.push_back(allItems[i]);
    }
    return pageItems;
}

void ScheduleManager::handleTap() {
    if (!_hasIcs || _days.empty()) return;

    const auto& currentItems = _days[_currentDayIndex].items;
    int totalSubPages = ((int)currentItems.size() + 3) / 4;
    if (totalSubPages < 1) totalSubPages = 1;

    if (_currentSubPage + 1 < totalSubPages) {
        // Move to next 4 items of current day
        _currentSubPage++;
    } else {
        // Advance to next day, reset subpage to 0
        _currentSubPage = 0;
        _currentDayIndex = (_currentDayIndex + 1) % _days.size();
    }
}

void ScheduleManager::handleShake() {
    // Shake jumps back to today
    _currentDayIndex = 0;
    _currentSubPage = 0;
}

void ScheduleManager::setIcsUrl(const String& url) {
    _icsUrl = url;
    Preferences prefs;
    prefs.begin("kubi_settings", false);
    prefs.putString("icalUrl", _icsUrl);
    prefs.end();

    if (_icsUrl.length() == 0) {
        clearIcs();
    } else {
        _hasIcs = true;
    }
}

void ScheduleManager::clearIcs() {
    _hasIcs = false;
    _icsUrl = "";
    _currentDayIndex = 0;
    _currentSubPage = 0;

    for (auto& day : _days) {
        day.items.clear();
    }
    refreshDayTitles();

#if defined(ESP32)
    if (LittleFS.exists("/calendar.ics")) {
        LittleFS.remove("/calendar.ics");
    }
#else
    remove("kubi_sim_calendar.ics");
#endif

    Preferences prefs;
    prefs.begin("kubi_settings", false);
    prefs.remove("icalUrl");
    prefs.end();
}

void ScheduleManager::setIcsContent(const String& content) {
    if (content.length() == 0) {
        clearIcs();
        return;
    }

    _hasIcs = true;

#if defined(ESP32)
    File f = LittleFS.open("/calendar.ics", "w");
    if (f) {
        f.print(content);
        f.close();
    }
#else
    std::ofstream out("kubi_sim_calendar.ics", std::ios::trunc);
    if (out.is_open()) {
        out << content.c_str();
        out.close();
    }
#endif

    parseIcs(content);
}

void ScheduleManager::loadSavedIcs() {
    Preferences prefs;
    prefs.begin("kubi_settings", false);
    _icsUrl = prefs.getString("icalUrl", "");
    prefs.end();

    String icsContent = "";

#if defined(ESP32)
    if (LittleFS.exists("/calendar.ics")) {
        File f = LittleFS.open("/calendar.ics", "r");
        if (f) {
            icsContent = f.readString();
            f.close();
        }
    }
#else
    std::ifstream in("kubi_sim_calendar.ics");
    if (in.is_open()) {
        std::stringstream ss;
        ss << in.rdbuf();
        icsContent = ss.str().c_str();
        in.close();
    }
#endif

    if (icsContent.length() > 0) {
        _hasIcs = true;
        parseIcs(icsContent);
    } else if (_icsUrl.length() > 0) {
        _hasIcs = true;
        // URL is saved but file not yet downloaded: calendar starts with 0 items until synced
        for (auto& day : _days) day.items.clear();
    } else {
        _hasIcs = false;
        for (auto& day : _days) day.items.clear();
    }
}

void ScheduleManager::parseIcs(const String& rawContent) {
    refreshDayTitles();

    for (auto& day : _days) {
        day.items.clear();
    }

    struct tm nowInfo;
    int nowYear = 2026, nowMonth = 9, nowDay = 6;
    if (getLocalTime(&nowInfo)) {
        nowYear = nowInfo.tm_year + 1900;
        nowMonth = nowInfo.tm_mon + 1;
        nowDay = nowInfo.tm_mday;
    }
    int nowCivil = getDaysFromCivil(nowYear, nowMonth, nowDay);

    // RFC 5545 line unfolding
    std::string unfolded;
    unfolded.reserve(rawContent.length());
    const char* p = rawContent.c_str();
    size_t len = rawContent.length();

    for (size_t i = 0; i < len; i++) {
        if (p[i] == '\r' && i + 1 < len && p[i + 1] == '\n') {
            if (i + 2 < len && (p[i + 2] == ' ' || p[i + 2] == '\t')) {
                i += 2; // skip CRLF + space/tab
                continue;
            }
        } else if (p[i] == '\n') {
            if (i + 1 < len && (p[i + 1] == ' ' || p[i + 1] == '\t')) {
                i += 1; // skip LF + space/tab
                continue;
            }
        }
        unfolded += p[i];
    }

    // Process line by line
    std::istringstream stream(unfolded);
    std::string line;
    bool inVEvent = false;
    std::string dtStart = "";
    std::string summary = "";

    auto finalizeEvent = [&]() {
        if (dtStart.empty()) return;

        // Parse dtStart: find colon
        size_t colon = dtStart.find(':');
        std::string val = (colon != std::string::npos) ? dtStart.substr(colon + 1) : dtStart;

        // Clean trailing whitespace/\r
        while (!val.empty() && (val.back() == '\r' || val.back() == ' ')) val.pop_back();

        if (val.length() >= 8) {
            int evYear = 0, evMonth = 0, evDay = 0;
            try {
                evYear = std::stoi(val.substr(0, 4));
                evMonth = std::stoi(val.substr(4, 2));
                evDay = std::stoi(val.substr(6, 2));
            } catch (...) {
                return;
            }

            int evCivil = getDaysFromCivil(evYear, evMonth, evDay);
            int diffDays = evCivil - nowCivil;

            // Strict ceiling: only up to 5 days (offsets 0 to 4)
            if (diffDays >= 0 && diffDays < 5) {
                std::string timeStr = "All day";
                size_t tPos = val.find('T');
                if (tPos != std::string::npos && val.length() >= tPos + 5) {
                    timeStr = val.substr(tPos + 1, 2) + ":" + val.substr(tPos + 3, 2);
                }

                // Unescape summary
                std::string cleanSum = "";
                for (size_t k = 0; k < summary.length(); k++) {
                    if (summary[k] == '\\' && k + 1 < summary.length()) {
                        char next = summary[k + 1];
                        if (next == ',' || next == ';' || next == '\\') {
                            cleanSum += next;
                            k++;
                            continue;
                        }
                    }
                    cleanSum += summary[k];
                }
                while (!cleanSum.empty() && (cleanSum.back() == '\r' || cleanSum.back() == ' ')) cleanSum.pop_back();
                if (cleanSum.empty()) cleanSum = "Busy";

                ScheduleItem item;
                item.time = timeStr.c_str();
                item.name = cleanSum.c_str();
                _days[diffDays].items.push_back(item);
            }
        }
    };

    while (std::getline(stream, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line == "BEGIN:VEVENT") {
            inVEvent = true;
            dtStart = "";
            summary = "";
        } else if (line == "END:VEVENT") {
            if (inVEvent) {
                finalizeEvent();
                inVEvent = false;
            }
        } else if (inVEvent) {
            if (line.rfind("DTSTART", 0) == 0) {
                dtStart = line;
            } else if (line.rfind("SUMMARY:", 0) == 0) {
                summary = line.substr(8);
            }
        }
    }

    // Sort events by time within each day
    for (auto& day : _days) {
        std::sort(day.items.begin(), day.items.end(), [](const ScheduleItem& a, const ScheduleItem& b) {
            return strcmp(a.time.c_str(), b.time.c_str()) < 0;
        });
    }

    _currentDayIndex = 0;
    _currentSubPage = 0;
}

void ScheduleManager::loadSampleSchedule() {
    _hasIcs = true;
    refreshDayTitles();

    for (auto& day : _days) {
        day.items.clear();
    }

    // Day 0: Today (6 events to demonstrate 4-item screen pagination and finance ticker!)
    _days[0].items.push_back({ "10:00", "Team Sync" });
    _days[0].items.push_back({ "11:30", "Design Jam Session" });
    _days[0].items.push_back({ "14:00", "Design Review with Mobile Engineering Team" }); // Long title -> finance ticker!
    _days[0].items.push_back({ "15:30", "Product Sync" });
    _days[0].items.push_back({ "17:00", "Architecture Q&A Workshop" }); // 5th item -> page 1 of Today
    _days[0].items.push_back({ "18:30", "Desk Clean & Wrap-up" });

    // Day 1: Tomorrow
    _days[1].items.push_back({ "09:30", "Sprint Planning" });
    _days[1].items.push_back({ "13:00", "Lunch with Sarah" });
    _days[1].items.push_back({ "16:00", "Code Review" });

    // Day 2: Day+2
    _days[2].items.push_back({ "11:00", "Client Demonstration" });
    _days[2].items.push_back({ "18:00", "Wilhelm's 21st Party!" });

    // Day 3: Day+3
    _days[3].items.push_back({ "14:00", "Deep Focus Work" });

    // Day 4: Day+4
    _days[4].items.push_back({ "15:00", "Team Weekly Retrospective" });

    _currentDayIndex = 0;
    _currentSubPage = 0;
}
