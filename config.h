#pragma once
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <cstdlib>
#include "language.h"

struct ObserverConfig {
    double latitude;   // degree, north positive
    double longitude;  // degree, east positive
    std::string starsFile;
    std::string messierFile;
    std::string satellitesFile;
    LanguageId language;  // UI language
    bool showHelp;        // show help overlay
    bool showLabels;      // show object labels
    double labelMagLimit; // only show labels for stars brighter than this mag
};

static inline void trim_string(std::string &s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
        ++start;
    if (start > 0) s = s.substr(start);
}

static inline ObserverConfig load_config(const std::string &filename = "config.ini") {
    ObserverConfig cfg;
    cfg.latitude = 39.9;
    cfg.longitude = 116.4;
    cfg.starsFile = "stars.txt";
    cfg.messierFile = "messier.txt";
    cfg.satellitesFile = "satellites.txt";
    cfg.language = LANGID_ENGLISH;
    cfg.showHelp = true;
    cfg.showLabels = true;
    cfg.labelMagLimit = 4.0;

    std::ifstream file(filename);
    if (!file.is_open()) return cfg;

    std::string line, section;
    while (std::getline(file, line)) {
        trim_string(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        trim_string(key);
        trim_string(val);

        if (section == "Observer") {
            if (key == "latitude") cfg.latitude = atof(val.c_str());
            else if (key == "longitude") cfg.longitude = atof(val.c_str());
        } else if (section == "Catalog") {
            if (key == "stars") cfg.starsFile = val;
            else if (key == "messier") cfg.messierFile = val;
            else if (key == "satellites") cfg.satellitesFile = val;
        } else if (section == "Display") {
            if (key == "language") {
                if (val == "zh" || val == "chinese" || val == "Chinese" || val == "1")
                    cfg.language = LANGID_CHINESE;
                else
                    cfg.language = LANGID_ENGLISH;
            } else if (key == "show_help") {
                cfg.showHelp = (val == "1" || val == "true" || val == "yes");
            } else if (key == "show_labels") {
                cfg.showLabels = (val == "1" || val == "true" || val == "yes");
            } else if (key == "label_mag_limit") {
                cfg.labelMagLimit = atof(val.c_str());
            }
        }
    }
    return cfg;
}
