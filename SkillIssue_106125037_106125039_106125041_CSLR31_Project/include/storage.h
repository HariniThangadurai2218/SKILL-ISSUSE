/**
 * storage.h - Storage module: declares CSV load/save functions. Definitions are in Source/storage.cpp.
 */
#ifndef SKILL_ISSUE_STORAGE_H
#define SKILL_ISSUE_STORAGE_H

#include "models.h"
#include <string>
#include <vector>

class Storage {
public:
    explicit Storage(const std::string& dataDirectory = "data");

    bool loadUsers(std::vector<User>& users) const;
    bool loadRequests(std::vector<BookingRequest>& requests) const;
    bool loadOffers(std::vector<MentorOffer>& offers) const;
    bool loadSessions(std::vector<Session>& sessions) const;

    bool saveUsers(const std::vector<User>& users) const;
    bool saveRequests(const std::vector<BookingRequest>& requests) const;
    bool saveOffers(const std::vector<MentorOffer>& offers) const;
    bool saveSessions(const std::vector<Session>& sessions) const;

private:
    std::string dataDirectory_;
    std::string path(const std::string& filename) const;
    bool ensureDirectory() const;
};

#endif
