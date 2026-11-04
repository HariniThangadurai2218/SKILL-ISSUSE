/**
 * platform.h - Platform module: business logic (users, bookings, sessions, points) plus the *API functions used by the HTTP server. Definitions are in Source/platform.cpp.
 */
#ifndef SKILL_ISSUE_PLATFORM_H
#define SKILL_ISSUE_PLATFORM_H

#include "hashmap.h"
#include "models.h"
#include "storage.h"
#include <vector>

class Platform {
public:
    Platform();
    void run();

    bool registerUserAPI(
        const std::string& id,
        const std::string& name,
        const std::string& password,
        const std::string& skills,
        std::string& message
    );

    bool loginAPI(
        const std::string& id,
        const std::string& password,
        std::string& message
    );

    bool getProfileAPI(
        const std::string& id,
        User& profile
    );
    
    std::vector<User> getLeaderboardAPI() const;

    // Top-k mentors by teaching points, found with a heap (PriorityQueue).
    std::vector<User> getTopMentorsAPI(int k) const;

    std::vector<Session> getMySessionsAPI(const std::string& id,bool teaching) const;

    bool updateSkillsAPI(const std::string& id,const std::string& skills,std::string& message);

    std::vector<BookingRequest> getMentorInboxAPI(const std::string& id) const;

    bool respondToMentorRequestAPI(
        const std::string& mentorId,
        int requestId,
        const std::string& action,
        const std::string& proposedTime,
        std::string& message
    );

    bool gradeQuizAPI(
        const std::string& mentorId,
        int sessionId,
        int score,
        std::string& message
    );

    bool reportOutcomeAPI(
        const std::string& mentorId,
        int sessionId,
        const std::string& outcome,
        std::string& message
    );

    bool rateSessionAPI(
        const std::string& learnerId,
        int sessionId,
        int rating,
        std::string& message
    );

    std::vector<BookingDetails> getMyBookingsAPI(const std::string& learnerId) const;

    bool acceptBookingOfferAPI(
        const std::string& learnerId,
        int requestId,
        const std::string& mentorId,
        std::string& message
    );

    std::vector<User> getMatchingMentorsAPI(const std::string& skill,const std::string& learnerId) const;

    bool bookSessionAPI(
        const std::string& learnerId,
        const std::string& skill,
        const std::string& mentorId,
        const std::string& preference,
        std::string& message
    );

private:
    Storage storage_;
    std::vector<User> users_;
    std::vector<BookingRequest> requests_;
    std::vector<MentorOffer> offers_;
    std::vector<Session> sessions_;
    // O(1) indexes: ID -> position in users_ / requests_ (see hashmap.h).
    HashMap<std::string, int> userIndex_;
    HashMap<int, int> requestIndex_;
    int nextRequestId_ = 1;
    int nextSessionId_ = 1;

    void loadData();
    void rebuildIndexes();
    void saveData() const;
    int findUser(const std::string& id) const;
    int findRequest(int id) const;
    bool userHasSkill(const User& user, const std::string& skill) const;
    void registerUser();
    void login();
    void adminMenu();
    void userMenu(int userIndex);
    void showProfile(int userIndex) const;
    void bookSession(int userIndex);
    void mentorInbox(int userIndex);
    void myBookings(int userIndex);
    void showMySessions(int userIndex, bool teaching) const;
    void updateSkills(int userIndex);
    void showLeaderboard() const;
    void showAllUsers() const;
    void showAllRequestsAndSessions() const;
    void completeSession(int sessionIndex);
    void rateCompletedSession(int userIndex);
    void gradeLearnerQuiz(int userIndex);
    void reportSessionOutcome(int userIndex);
    void applySessionOutcome(int sessionIndex,const std::string& outcome);
    void resolveSessionDispute();
};

#endif
