/**
 * models.h - Plain data records (User, BookingRequest, MentorOffer, BookingDetails, Session) shared by every module.
 */
#ifndef SKILL_ISSUE_MODELS_H
#define SKILL_ISSUE_MODELS_H

#include <vector>
#include <string>

struct User {
    std::string id;
    std::string name;
    std::string password; // Demo only; use password hashing in production.
    std::string skills;   // Skills separated by semicolons.
    double totalPoints = 30.0;
    double teachingPoints = 0.0;
    int teachingSessions = 0;
    int learningSessions = 0;
};

struct BookingRequest {
    int id = 0;
    std::string learnerId;
    std::string targetMentorId;
    std::string skill;
    std::string preference;

    // PENDING, OFFERED, REJECTED, CONFIRMED, CLOSED
    std::string status = "PENDING";

    std::string confirmedMentorId;
    std::string confirmedTime;
};

struct MentorOffer {
    int requestId = 0;
    std::string mentorId;
    std::string proposedTime;
};

struct BookingDetails {
    BookingRequest request;
    std::vector<MentorOffer> offers;
};

struct Session {
    int id = 0;
    int requestId = 0;
    std::string learnerId;
    std::string mentorId;
    std::string skill;
    std::string scheduledTime;
    std::string status = "SCHEDULED";
    std::string learnerOutcome = "PENDING";
    std::string mentorOutcome = "PENDING";

    // -1 means the learner has not rated the mentor yet.
    int learnerRating = -1;

    // -1 means the mentor has not graded the learner's quiz yet.
    int quizScore = -1;
};


#endif
