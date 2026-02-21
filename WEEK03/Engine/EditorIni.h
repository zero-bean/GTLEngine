#pragma once
#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

class CEditorIni {
public:
    static CEditorIni& Get() { static CEditorIni s; return s;
    }

    // 경로 설정(원하면 앱 시작 시 1번 호출)
    void SetPath(const std::filesystem::path& p) { path = p; }

    bool Load() {
        data.clear();
        std::ifstream fin(path);
        if (!fin.is_open()) return false;

        std::string line, section = "Default";
        while (std::getline(fin, line)) {
            Trim(line);
            if (line.empty() || line[0] == ';' || line.rfind("//", 0) == 0 || line[0] == '#') continue;
            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                Trim(section);
                continue;
            }
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            Trim(key); Trim(val);
            data[section][key] = val;
        }
        return true;
    }

    bool Save() const {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream fout(path, std::ios::trunc);
        if (!fout.is_open()) return false;
        for (const auto& sec : data) {
            fout << "[" << sec.first << "]\n";
            for (const auto& kv : sec.second) {
                fout << kv.first << "=" << kv.second << "\n";
            }
            fout << "\n";
        }
        return true;
    }

    // ----- Get/Set -----
    std::string GetString(const std::string& sec, const std::string& key, const std::string& def = "") const {
        if (auto sit = data.find(sec); sit != data.end()) {
            if (auto kit = sit->second.find(key); kit != sit->second.end()) return kit->second;
        }
        return def;
    }
    int    GetInt(const std::string& s, const std::string& k, int def = 0) const {
        try { return std::stoi(GetString(s, k, std::to_string(def))); }
        catch (...) { return def; }
    }
    float  GetFloat(const std::string& s, const std::string& k, float def = 0.f) const {
        try { return std::stof(GetString(s, k, std::to_string(def))); }
        catch (...) { return def; }
    }
    bool   GetBool(const std::string& s, const std::string& k, bool def = false) const {
        std::string v = GetString(s, k, def ? "true" : "false");
        ToLower(v);
        return (v == "true" || v == "1" || v == "yes" || v == "on");
    }

    void SetString(const std::string& s, const std::string& k, const std::string& v) { data[s][k] = v; }
    void SetInt(const std::string& s, const std::string& k, int    v) { data[s][k] = std::to_string(v); }
    void SetFloat(const std::string& s, const std::string& k, float  v) { data[s][k] = FloatToStr(v); }
    void SetBool(const std::string& s, const std::string& k, bool   v) { data[s][k] = v ? "true" : "false"; }

private:
    std::filesystem::path path = "./config/editor.ini";
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data;

    static void LTrim(std::string& s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {return !std::isspace(ch); })); }
    static void RTrim(std::string& s) { s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {return !std::isspace(ch); }).base(), s.end()); }
    static void Trim(std::string& s) { LTrim(s); RTrim(s); }
    static void ToLower(std::string& s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {return (char)std::tolower(c); }); }

    static std::string FloatToStr(float v) {
        std::ostringstream oss; oss.setf(std::ios::fixed); oss.precision(3); oss << v; return oss.str();
    }
    CEditorIni() = default;
};
