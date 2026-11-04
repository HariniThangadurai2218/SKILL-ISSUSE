/**
 * storage.cpp - CSV persistence: parses/escapes quoted CSV fields and loads/saves the four data files.
 */
#include "storage.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
std::string csvEscape(const std::string& value) {
    std::string out = "\"";
    for (char c : value) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\r') continue;   // ignore CR from Windows (CRLF) line endings
        if (c == '"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (c == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

bool openForRead(const std::string& filename, std::ifstream& in) {
    in.open(filename);
    if (!in) return false; // Missing file is treated as empty data.
    std::string header;
    std::getline(in, header);
    return true;
}
}

Storage::Storage(const std::string& dataDirectory) : dataDirectory_(dataDirectory) {
    ensureDirectory();
}

std::string Storage::path(const std::string& filename) const {
    return (std::filesystem::path(dataDirectory_) / filename).string();
}

bool Storage::ensureDirectory() const {
    std::error_code ec;
    std::filesystem::create_directories(dataDirectory_, ec);
    return !ec;
}

bool Storage::loadUsers(std::vector<User>& users) const {
    users.clear();
    std::ifstream in;
    if (!openForRead(path("users.csv"), in)) return true;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        if (f.size() < 8) continue;
        try {
            User u;
            u.id = f[0]; u.name = f[1]; u.password = f[2]; u.skills = f[3];
            u.totalPoints = std::stod(f[4]);
            u.teachingPoints = std::stod(f[5]);
            u.teachingSessions = std::stoi(f[6]);
            u.learningSessions = std::stoi(f[7]);
            users.push_back(u);
        } catch (...) { /* Skip malformed rows. */ }
    }
    return true;
}

bool Storage::loadRequests(std::vector<BookingRequest>& requests) const {
    requests.clear();
    std::ifstream in;
    if (!openForRead(path("booking_requests.csv"), in)) return true;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        if (f.size() < 7) continue;
        try {
            BookingRequest r;
            r.id = std::stoi(f[0]); r.learnerId = f[1]; r.skill = f[2];
            r.preference = f[3]; r.status = f[4];
            r.confirmedMentorId = f[5]; r.confirmedTime = f[6];
            if (f.size() >= 8) r.targetMentorId = f[7];
            requests.push_back(r);
        } catch (...) { /* Skip malformed rows. */ }
    }
    return true;
}

bool Storage::loadOffers(std::vector<MentorOffer>& offers) const {
    offers.clear();
    std::ifstream in;
    if (!openForRead(path("mentor_offers.csv"), in)) return true;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = parseCsvLine(line);
        if (f.size() < 3) continue;
        try { offers.push_back({std::stoi(f[0]), f[1], f[2]}); }
        catch (...) { /* Skip malformed rows. */ }
    }
    return true;
}

bool Storage::loadSessions(std::vector<Session>& sessions) const {
    sessions.clear();
    std::ifstream in;

    if (!openForRead(path("sessions.csv"), in))
        return true;

    std::string line;

    while (std::getline(in, line)) {
        if (line.empty()) continue;

        auto f = parseCsvLine(line);
        if (f.size() < 7) continue;

        try {
            Session s;

            s.id = std::stoi(f[0]);
            s.requestId = std::stoi(f[1]);
            s.learnerId = f[2];
            s.mentorId = f[3];
            s.skill = f[4];
            s.scheduledTime = f[5];
            s.status = f[6];

            if (f.size() >= 8)
                s.learnerRating = std::stoi(f[7]);

            if (f.size() >= 9)
                s.quizScore = std::stoi(f[8]);

            // NEW: Load attendance reports
            if (f.size() >= 10 && !f[9].empty())
                s.learnerOutcome = f[9];

            if (f.size() >= 11 && !f[10].empty())
                s.mentorOutcome = f[10];

            sessions.push_back(s);

        } catch (...) {
            // Skip malformed rows.
        }
    }

    return true;
}

bool Storage::saveUsers(const std::vector<User>& users) const {
    if (!ensureDirectory()) return false;
    std::ofstream out(path("users.csv"));
    if (!out) return false;
    out << "id,name,password,skills,total_points,teaching_points,teaching_sessions,learning_sessions\n";
    out << std::fixed << std::setprecision(1);
    for (const auto& u : users) {
        out << csvEscape(u.id) << ',' << csvEscape(u.name) << ','
            << csvEscape(u.password) << ',' << csvEscape(u.skills) << ','
            << u.totalPoints << ',' << u.teachingPoints << ','
            << u.teachingSessions << ',' << u.learningSessions << '\n';
    }
    return static_cast<bool>(out);
}

bool Storage::saveRequests(const std::vector<BookingRequest>& requests) const {
    if (!ensureDirectory()) return false;
    std::ofstream out(path("booking_requests.csv"));
    if (!out) return false;
    out << "request_id,learner_id,skill,preference,status,confirmed_mentor_id,confirmed_time,target_mentor_id\n";
    for (const auto& r : requests) {
        out << r.id << ',' << csvEscape(r.learnerId) << ',' << csvEscape(r.skill)
            << ',' << csvEscape(r.preference) << ',' << csvEscape(r.status)
            << ',' << csvEscape(r.confirmedMentorId) << ','
            << csvEscape(r.confirmedTime) << ',' << csvEscape(r.targetMentorId) << '\n';
    }
    return static_cast<bool>(out);
}

bool Storage::saveOffers(const std::vector<MentorOffer>& offers) const {
    if (!ensureDirectory()) return false;
    std::ofstream out(path("mentor_offers.csv"));
    if (!out) return false;
    out << "request_id,mentor_id,proposed_time\n";
    for (const auto& o : offers) {
        out << o.requestId << ',' << csvEscape(o.mentorId) << ','
            << csvEscape(o.proposedTime) << '\n';
    }
    return static_cast<bool>(out);
}


bool Storage::saveSessions(const std::vector<Session>& sessions) const {
    if (!ensureDirectory()) return false;

    std::ofstream out(path("sessions.csv"));
    if (!out) return false;

    out << "session_id,request_id,learner_id,mentor_id,"
           "skill,scheduled_time,status,learner_rating,quiz_score,"
           "learner_outcome,mentor_outcome\n";

    for (const auto& s : sessions) {
        out << s.id << ','
            << s.requestId << ','
            << csvEscape(s.learnerId) << ','
            << csvEscape(s.mentorId) << ','
            << csvEscape(s.skill) << ','
            << csvEscape(s.scheduledTime) << ','
            << csvEscape(s.status) << ','
            << s.learnerRating << ','
            << s.quizScore << ','
            << csvEscape(s.learnerOutcome) << ','
            << csvEscape(s.mentorOutcome) << '\n';
    }

    return static_cast<bool>(out);
}