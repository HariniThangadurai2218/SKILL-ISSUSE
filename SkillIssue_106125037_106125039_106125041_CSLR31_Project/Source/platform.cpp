/**
 * platform.cpp - Core logic: registration/login, mentor matching (merge sort), booking workflow, attendance/dispute handling, ratings, quiz points, leaderboard.
 */
#include "platform.h"
#include "priority_queue.h"
#include "sorting.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

using namespace std;

namespace {
const string ADMIN_ID = "admin";
const string ADMIN_PASSWORD = "admin123";

string trimText(const string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

string lowerText(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(tolower(c));
    });
    return s;
}

string readText(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin >> ws, value);
    return trimText(value);
}

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        int value;
        if (cin >> value) return value;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Please enter a valid number.\n";
    }
}

bool containsSkill(const string& list, const string& skill) {
    stringstream ss(list);
    string item;
    while (getline(ss, item, ';')) {
        if (lowerText(trimText(item)) == lowerText(trimText(skill))) return true;
    }
    return false;
}
}

Platform::Platform() : storage_("data") { loadData(); }

void Platform::loadData() {
    storage_.loadUsers(users_);
    storage_.loadRequests(requests_);
    storage_.loadOffers(offers_);
    storage_.loadSessions(sessions_);
    rebuildIndexes();
    for (const auto& r : requests_) nextRequestId_ = max(nextRequestId_, r.id + 1);
    for (const auto& s : sessions_) nextSessionId_ = max(nextSessionId_, s.id + 1);
}

void Platform::saveData() const {
    bool ok = storage_.saveUsers(users_)
        && storage_.saveRequests(requests_)
        && storage_.saveOffers(offers_)
        && storage_.saveSessions(sessions_);
    if (!ok) cerr << "Warning: Could not save one or more CSV files. Check folder permissions.\n";
}

// Rebuilds both hash indexes from the vectors (called after loading the CSVs).
void Platform::rebuildIndexes() {
    userIndex_.clear();
    requestIndex_.clear();
    for (size_t i = 0; i < users_.size(); ++i)
        if (!userIndex_.contains(users_[i].id))      // keep the first of any duplicates
            userIndex_.put(users_[i].id, static_cast<int>(i));
    for (size_t i = 0; i < requests_.size(); ++i)
        if (!requestIndex_.contains(requests_[i].id))
            requestIndex_.put(requests_[i].id, static_cast<int>(i));
}

// O(1) average look-up through the hash index; -1 when the ID is unknown.
int Platform::findUser(const string& id) const {
    int pos = -1;
    return userIndex_.get(id, pos) ? pos : -1;
}

int Platform::findRequest(int id) const {
    int pos = -1;
    return requestIndex_.get(id, pos) ? pos : -1;
}

bool Platform::userHasSkill(const User& user, const string& skill) const {
    return containsSkill(user.skills, skill);
}

void Platform::run() {
    cout << "===========================================\n"
         << "       SKILL-ISSUE: PEER SKILL EXCHANGE\n"
         << "===========================================\n"
         << "Data is stored in CSV files in the data folder.\n";
    bool running = true;
    while (running) {
        cout << "\nMAIN MENU\n1. Register as user\n2. Login\n0. Exit\n";
        int choice = readInt("Choose: ");
        switch (choice) {
            case 1: registerUser(); break;
            case 2: login(); break;
            case 0: saveData(); running = false; break;
            default: cout << "Invalid option.\n";
        }
    }
    cout << "Data saved. Goodbye!\n";
}

bool Platform::registerUserAPI(
    const string& id,
    const string& name,
    const string& password,
    const string& skills,
    string& message
) {
    if(id.empty() || id == ADMIN_ID || findUser(id) >= 0){
        message = "ID is empty, reserved, or already in use.";
        return false;
    }

    if(name.empty() || password.empty()){
        message = "Name and password cannot be empty.";
        return false;
    }

    User u;
    u.id = id;
    u.name = name;
    u.password = password;
    u.skills = skills;

    users_.push_back(u);
    userIndex_.put(u.id, static_cast<int>(users_.size()) - 1);
    saveData();

    message = "Registration successful.";
    return true;
}

bool Platform::loginAPI(
    const string& id,
    const string& password,
    string& message
) {
    int idx = findUser(id);

    if(idx < 0 || users_[idx].password != password){
        message = "Invalid user ID or password.";
        return false;
    }

    message = "Login successful.";
    return true;
}

bool Platform::getProfileAPI(
    const string& id,
    User& profile
) {
    int idx=findUser(id);
    if(idx==-1){
        return false;
    }
    profile=users_[idx];
    profile.password="";
    return true;
}

vector<User> Platform::getLeaderboardAPI() const {
    vector<User> ranked=users_;

    mergeSort(ranked,[](const User& a,const User& b){
        if(a.teachingPoints!=b.teachingPoints)
            return a.teachingPoints>b.teachingPoints;
        return a.id<b.id;
    });
    for(User& u:ranked){
        u.password="";
    }
    return ranked;
}

// Returns the k users with the most teaching points (best first).
// Keeps a heap of at most k users whose top() is the WORST user kept so far,
// so each of the n users costs O(log k): total O(n log k) instead of sorting all n.
vector<User> Platform::getTopMentorsAPI(int k) const {
    vector<User> result;
    if (k <= 0) return result;

    // comp(a,b) == "a is better than b" for the final ranking.
    auto better = [](const User& a, const User& b) {
        if (a.teachingPoints != b.teachingPoints)
            return a.teachingPoints > b.teachingPoints;
        return a.id < b.id;
    };
    // The heap's "highest priority" element must be the worst user, so flip it.
    auto worstFirst = [better](const User& a, const User& b) { return better(b, a); };
    PriorityQueue<User, decltype(worstFirst)> heap(worstFirst);

    for (const User& u : users_) {
        heap.push(u);
        if (heap.size() > static_cast<size_t>(k)) heap.pop();   // drop the worst
    }
    while (!heap.empty()) {            // pops worst -> best, so reverse afterwards
        User u = heap.top();
        u.password = "";
        result.push_back(u);
        heap.pop();
    }
    reverse(result.begin(), result.end());
    return result;
}

vector<Session> Platform::getMySessionsAPI(const string& id,bool teaching) const {
    vector<Session> result;

    for(const auto& s:sessions_){
        if((teaching && s.mentorId==id) || (!teaching && s.learnerId==id)){
            result.push_back(s);
        }
    }

    return result;
}

bool Platform::updateSkillsAPI(
    const string& id,
    const string& skills,
    string& message
) {
    int idx=findUser(id);
    if(idx==-1){
        message="User doesn't exist.";
        return false;
    }
    cout << "Received skills: " << skills << '\n';
    users_[idx].skills=skills;
    saveData();
    message="Skills updated successfully";
    return true;
}

vector<BookingRequest> Platform::getMentorInboxAPI(const string& id) const {
    vector<BookingRequest> result;
    int idx=findUser(id);
    if(idx==-1){
        return {};
    }
    const User& user=users_[idx];
    for(const auto& r:requests_){
        if ((r.status == "PENDING" || r.status == "OFFERED") &&
        r.learnerId != user.id &&
        r.targetMentorId == user.id &&
        userHasSkill(user, r.skill)) {
            result.push_back(r);
        }
    }
    return result;
}

bool Platform::respondToMentorRequestAPI(
    const string& mentorId,
    int requestId,
    const string& action,
    const string& proposedTime,
    string& message
) {
    int mentorIndex=findUser(mentorId);
    int pos=findRequest(requestId);

    if(mentorIndex==-1 || pos==-1){
        message="User or request not found.";
        return false;
    }

    User& mentor=users_[mentorIndex];
    BookingRequest& r=requests_[pos];

    if(r.learnerId==mentorId ||
       r.targetMentorId!=mentorId ||
       !userHasSkill(mentor,r.skill) ||
       (r.status!="PENDING" && r.status!="OFFERED")){
        message="You cannot respond to this request.";
        return false;
    }

    if(action=="reject"){
        r.status="REJECTED";

        offers_.erase(
            remove_if(
                offers_.begin(),
                offers_.end(),
                [requestId](const MentorOffer& offer){
                    return offer.requestId==requestId;
                }
            ),
            offers_.end()
        );

        saveData();
        message="Request rejected successfully.";
        return true;
    }

    if(action=="accept"){
        if(proposedTime.empty()){
            message="Proposed time cannot be empty.";
            return false;
        }

        for(const auto& offer:offers_){
            if(offer.requestId==requestId &&
               offer.mentorId==mentorId){
                message="You have already offered a time.";
                return false;
            }
        }

        offers_.push_back({requestId,mentorId,proposedTime});
        r.status="OFFERED";

        saveData();
        message="Offer sent successfully.";
        return true;
    }

    message="Invalid action.";
    return false;
}

bool Platform::gradeQuizAPI(
    const string& mentorId,
    int sessionId,
    int score,
    string& message
) {
    if(score<0 || score>100){
        message="Score must be between 0 and 100.";
        return false;
    }

    for(auto& s:sessions_){
        if(s.id==sessionId){
            if(s.mentorId!=mentorId){
                message="You are not the mentor of this session.";
                return false;
            }

            s.quizScore=score;
            saveData();

            message="Quiz score submitted successfully.";
            return true;
        }
    }

    message="Session not found.";
    return false;
}

// bool Platform::gradeQuizAPI(
//     const string& mentorId,
//     int sessionId,
//     int score,
//     string& message
// ) {
//     // Validate quiz score
//     if (score < 0 || score > 5) {
//         message = "Score must be between 0 and 5.";
//         return false;
//     }

//     for (auto& s : sessions_) {
//         if (s.id == sessionId) {

//             // Check whether the logged-in user is the mentor
//             if (s.mentorId != mentorId) {
//                 message = "You are not the mentor of this session.";
//                 return false;
//             }

//             // Only completed sessions can be graded
//             if (s.status != "COMPLETED") {
//                 message = "Only completed sessions can be graded.";
//                 return false;
//             }

//             // Prevent duplicate grading
//             if (s.quizScore != -1) {
//                 message = "This session has already been graded.";
//                 return false;
//             }

//             // Find learner
//             int learnerIndex = findUser(s.learnerId);

//             if (learnerIndex == -1) {
//                 message = "Learner not found.";
//                 return false;
//             }

//             // Store quiz score and update learner points
//             s.quizScore = score;
//             users_[learnerIndex].totalPoints += score;

//             saveData();

//             message = "Quiz score submitted successfully. Learner points updated.";
//             return true;
//         }
//     }

//     message = "Session not found.";
//     return false;
// }

bool Platform::reportOutcomeAPI(
    const string& mentorId,
    int sessionId,
    const string& outcome,
    string& message
) {
    if(outcome!="COMPLETED" && outcome!="MISSED" && outcome!="CANCELLED"){
        message="Invalid session outcome.";
        return false;
    }
    for(auto& s:sessions_){
        if(s.id==sessionId){
            if(s.mentorId!=mentorId){
                message="You are not the mentor of this session.";
                return false;
            }

            if(s.mentorOutcome!="PENDING"){
                message="Outcome has already been reported.";
                return false;
            }

            s.mentorOutcome=outcome;
            saveData();

            message="Session outcome reported successfully.";
            return true;
        }
    }
    message="Session not found.";
    return false;
}

bool Platform::rateSessionAPI(
    const string& learnerId,
    int sessionId,
    int rating,
    string& message
) {
    if(rating<1 || rating>5){
        message="Rating must be between 1 and 5.";
        return false;
    }
    for(auto& s:sessions_){
        if(s.id==sessionId){
            if(s.learnerId!=learnerId){
                message="You are not the learner of this session.";
                return false;
            }
            if(s.mentorOutcome!="COMPLETED"){
                message="Only completed sessions can be rated.";
                return false;
            }
            if(s.learnerRating!=-1){
                message="This session has already been rated.";
                return false;
            }

            s.learnerRating=rating;
            saveData();

            message="Rating submitted successfully.";
            return true;
        }
    }
    message="Session not found.";
    return false;
}

// bool Platform::rateSessionAPI(
//     const string& learnerId,
//     int sessionId,
//     int rating,
//     string& message
// ) {
//     // Validate rating
//     if (rating < 1 || rating > 5) {
//         message = "Rating must be between 1 and 5.";
//         return false;
//     }

//     for (auto& s : sessions_) {
//         if (s.id == sessionId) {

//             // Check whether the logged-in user is the learner
//             if (s.learnerId != learnerId) {
//                 message = "You are not the learner of this session.";
//                 return false;
//             }

//             // Only completed sessions can be rated
//             if (s.status != "COMPLETED") {
//                 message = "Only completed sessions can be rated.";
//                 return false;
//             }

//             // Prevent duplicate rating
//             if (s.learnerRating != -1) {
//                 message = "This session has already been rated.";
//                 return false;
//             }

//             // Find mentor
//             int mentorIndex = findUser(s.mentorId);

//             if (mentorIndex == -1) {
//                 message = "Mentor not found.";
//                 return false;
//             }

//             // Store rating and update mentor teaching points
//             s.learnerRating = rating;
//             users_[mentorIndex].teachingPoints += rating * 5.0;

//             saveData();

//             message = "Rating submitted successfully. Mentor teaching points updated.";
//             return true;
//         }
//     }

//     message = "Session not found.";
//     return false;
// }

vector<BookingDetails> Platform::getMyBookingsAPI(const string& learnerId) const {
    vector<BookingDetails> result;

    for(const auto& r:requests_){
        if(r.learnerId!=learnerId){
            continue;
        }
        BookingDetails details;
        details.request=r;

        for(const auto& offer:offers_){
            if(offer.requestId==r.id){
                details.offers.push_back(offer);
            }
        }
        result.push_back(details);
    }
    return result;
}

bool Platform::acceptBookingOfferAPI(
    const string& learnerId,
    int requestId,
    const string& mentorId,
    string& message
) {
    int pos=findRequest(requestId);

    if(pos<0 || requests_[pos].learnerId!=learnerId){
        message="Booking request not found.";
        return false;
    }
    BookingRequest& r=requests_[pos];

    if(r.status!="PENDING" && r.status!="OFFERED"){
        message="This booking cannot be accepted.";
        return false;
    }
    if(mentorId!=r.targetMentorId){
        message="This is not your selected mentor.";
        return false;
    }
    int offerPos=-1;

    for(size_t i=0;i<offers_.size();i++){
        if(offers_[i].requestId==requestId &&
           offers_[i].mentorId==mentorId){
            offerPos=static_cast<int>(i);
            break;
        }
    }
    if(offerPos<0){
        message="No offer from this mentor was found.";
        return false;
    }
    r.status="CONFIRMED";
    r.confirmedMentorId=offers_[offerPos].mentorId;
    r.confirmedTime=offers_[offerPos].proposedTime;

    Session s;
    s.id=nextSessionId_++;
    s.requestId=r.id;
    s.learnerId=r.learnerId;
    s.mentorId=r.confirmedMentorId;
    s.skill=r.skill;
    s.scheduledTime=r.confirmedTime;
    s.status="SCHEDULED";

    sessions_.push_back(s);

    int mentorIndex=findUser(r.confirmedMentorId);
    if(mentorIndex>=0){
        ++users_[mentorIndex].teachingSessions;
    }
    int learnerIndex=findUser(learnerId);

    if(learnerIndex>=0){
        ++users_[learnerIndex].learningSessions;
    }
    saveData();

    message="Booking confirmed successfully. Session ID: "+to_string(s.id);
    return true;
}

vector<User> Platform::getMatchingMentorsAPI(const string& skill,const string& learnerId) const {
    vector<User> mentors;

    int learnerIndex=findUser(learnerId);
    if(learnerIndex==-1){
        return mentors;
    }
    for(const auto& user:users_){
        if(user.id!=learnerId && userHasSkill(user,skill)){
            User mentor=user;
            mentor.password="";
            mentors.push_back(mentor);
        }
    }
    // Sort by teaching points, teaching sessions, then ID.
    mergeSort(mentors,[](const User& a,const User& b){
        if(a.teachingPoints!=b.teachingPoints)
            return a.teachingPoints>b.teachingPoints;

        if(a.teachingSessions!=b.teachingSessions)
            return a.teachingSessions>b.teachingSessions;

        return a.id<b.id;
    });
    return mentors;
}

bool Platform::bookSessionAPI(
    const string& learnerId,
    const string& skill,
    const string& mentorId,
    const string& preference,
    string& message
) {
    int learnerIndex=findUser(learnerId);
    int mentorIndex=findUser(mentorId);

    if(learnerIndex==-1 || mentorIndex==-1){
        message="Invalid learner or mentor.";
        return false;
    }
    if(learnerId==mentorId){
        message="You cannot book yourself.";
        return false;
    }
    if(skill.empty() || !userHasSkill(users_[mentorIndex],skill)){
        message="This mentor does not teach that skill.";
        return false;
    }
    BookingRequest request;
    request.id=nextRequestId_++;
    request.learnerId=learnerId;
    request.targetMentorId=mentorId;
    request.skill=skill;
    request.preference=preference.empty()?"Flexible":preference;
    request.status="PENDING";

    requests_.push_back(request);
    requestIndex_.put(request.id, static_cast<int>(requests_.size()) - 1);

    saveData();

    message="Booking request created successfully. Request ID: "+to_string(request.id);
    return true;
}

void Platform::registerUser() {
    User u;
    u.id = readText("Create user ID (no spaces): ");
    if (u.id.empty() || u.id == ADMIN_ID || findUser(u.id) >= 0) {
        cout << "That ID is empty, reserved, or already in use.\n";
        return;
    }
    u.name = readText("Name: ");
    u.password = readText("Create password: ");
    u.skills = readText("Skills you can teach (separate multiple skills with semicolons): ");
    if (u.name.empty() || u.password.empty()) {
        cout << "Name and password cannot be empty. Registration cancelled.\n";
        return;
    }
    // Registration always creates a normal user; role cannot be selected.
    users_.push_back(u);
    userIndex_.put(u.id, static_cast<int>(users_.size()) - 1);
    saveData();
    cout << "Registration successful. You are registered as a USER.\n";
}

void Platform::login() {
    string id = readText("User ID: ");
    string password = readText("Password: ");
    if (id == ADMIN_ID && password == ADMIN_PASSWORD) {
        adminMenu();
        return;
    }
    int idx = findUser(id);
    if (idx < 0 || users_[idx].password != password) {
        cout << "Invalid user ID or password.\n";
        return;
    }
    userMenu(idx);
}

void Platform::adminMenu() {
    bool active = true;
    while (active) {
        cout << "\n========== ADMIN DASHBOARD ==========\n"
             << "1. View all users\n"
             << "2. View all booking requests and sessions\n"
             << "3. View leaderboard\n"
             << "4. Resolve disputed session outcomes\n"
             << "0. Logout\n";
        int choice = readInt("Choose: ");
        switch (choice) {
            case 1: showAllUsers(); break;
            case 2: showAllRequestsAndSessions(); break;
            case 3: showLeaderboard(); break;
            case 4: resolveSessionDispute(); break;
            case 0: active = false; break;
            default: cout << "Invalid option.\n";
        }
    }
}

void Platform::userMenu(int userIndex) {
    bool active = true;
    while (active) {
        cout << "\n========== USER DASHBOARD =========="
             << "\nWelcome, " << users_[userIndex].name << " (" << users_[userIndex].id << ")\n"
             << "1. My profile\n2. Book a session\n3. Mentor inbox\n"
             << "4. My bookings / accept an offer\n5. My teaching sessions\n"
             << "6. My learning sessions\n7. Teaching leaderboard\n"
             << "8. Update teaching skills\n"
             << "9. Rate a completed session (learner)\n"
             << "10. Grade learner quiz (mentor)\n"
             << "11. Report session outcome / attendance\n"
             <<"0. Logout\n";
        int choice = readInt("Choose: ");
        switch (choice) {
            case 1: showProfile(userIndex); break;
            case 2: bookSession(userIndex); break;
            case 3: mentorInbox(userIndex); break;
            case 4: myBookings(userIndex); break;
            case 5: showMySessions(userIndex, true); break;
            case 6: showMySessions(userIndex, false); break;
            case 7: showLeaderboard(); break;
            case 8: updateSkills(userIndex); break;
            case 9: rateCompletedSession(userIndex); break;
            case 10: gradeLearnerQuiz(userIndex); break;
            case 11: reportSessionOutcome(userIndex); break;
            case 0: active = false; break;
            default: cout << "Invalid option.\n";
        }
    }
}

void Platform::showProfile(int i) const {
    const User& u = users_[i];
    cout << "\n--- MY PROFILE ---\nID: " << u.id << "\nName: " << u.name
         << "\nTeaching skills: " << (u.skills.empty() ? "(none)" : u.skills)
         << "\nTotal points: " << u.totalPoints
         << "\nTeaching points: " << u.teachingPoints
         << "\nTeaching sessions: " << u.teachingSessions
         << "\nLearning sessions: " << u.learningSessions << '\n';
}

void Platform::bookSession(int userIndex) {
    const User& learner = users_[userIndex];

    string skill = readText("Skill you want to learn: ");
    if (skill.empty()) {
        cout << "Skill cannot be empty.\n";
        return;
    }

    // Find users who can teach the requested skill.
    vector<int> matching;

    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i].id != learner.id &&
            userHasSkill(users_[i], skill)) {
            matching.push_back(static_cast<int>(i));
        }
    }

    if (matching.empty()) {
        cout << "No mentors currently teach this skill.\n";
        return;
    }

    // Rank mentors by teaching points and sessions.
    mergeSort(matching,[&](int a,int b){
        const User& u1=users_[a];
        const User& u2=users_[b];

        if(u1.teachingPoints!=u2.teachingPoints)
            return u1.teachingPoints>u2.teachingPoints;
        if(u1.teachingSessions!=u2.teachingSessions)
            return u1.teachingSessions>u2.teachingSessions;
        return u1.id<u2.id;
    });

    cout << "\n========== MATCHING MENTORS ==========\n";
    cout << left << setw(8) << "Rank"
         << setw(16) << "Mentor ID"
         << setw(22) << "Name"
         << setw(18) << "Teaching Points"
         << "Sessions Taught\n";

    for (size_t i = 0; i < matching.size(); ++i) {
        const User& mentor = users_[matching[i]];

        cout << left << setw(8) << i + 1
             << setw(16) << mentor.id
             << setw(22) << mentor.name
             << setw(18) << fixed << setprecision(1)
             << mentor.teachingPoints
             << mentor.teachingSessions << '\n';
    }

    // Let the learner choose one specific mentor.
    string mentorId = readText(
        "\nEnter the Mentor ID you want (0 to cancel): ");

    if (mentorId == "0") {
        cout << "Booking cancelled.\n";
        return;
    }

    int chosenIndex = -1;

    for (int idx : matching) {
        if (users_[idx].id == mentorId) {
            chosenIndex = idx;
            break;
        }
    }

    if (chosenIndex == -1) {
        cout << "Invalid mentor ID. Please book again.\n";
        return;
    }

    string preference = readText(
        "Preferred day/time (or Flexible): ");

    BookingRequest request;
    request.id = nextRequestId_++;
    request.learnerId = learner.id;
    request.targetMentorId = users_[chosenIndex].id;
    request.skill = skill;
    request.preference = preference.empty()
                         ? "Flexible" : preference;
    request.status = "PENDING";

    requests_.push_back(request);
    requestIndex_.put(request.id, static_cast<int>(requests_.size()) - 1);
    saveData();

    cout << "\nRequest #" << request.id
         << " sent to " << users_[chosenIndex].name
         << " (" << users_[chosenIndex].id << ").\n";

    cout << "Only your selected mentor will receive this request.\n";
}

void Platform::mentorInbox(int userIndex) {
    const User& me = users_[userIndex];

    cout << "\n--- MENTOR INBOX ---\n";
    bool any = false;

    for (const auto& r : requests_) {
        if ((r.status == "PENDING" || r.status == "OFFERED") &&
            r.learnerId != me.id &&
            r.targetMentorId == me.id &&
            userHasSkill(me, r.skill)) {

            cout << "Request #" << r.id
                 << " | Skill: " << r.skill
                 << " | Learner: " << r.learnerId
                 << " | Preferred time: " << r.preference
                 << " | Status: " << r.status << '\n';

            any = true;
        }
    }

    if (!any) {
        cout << "No pending requests assigned to you.\n";
        return;
    }

    int requestId = readInt("Enter request ID (0 to cancel): ");
    if (requestId == 0) return;

    int pos = findRequest(requestId);

    if (pos < 0) {
        cout << "Request not found.\n";
        return;
    }

    BookingRequest& r = requests_[pos];

    if (r.learnerId == me.id ||
        r.targetMentorId != me.id ||
        !userHasSkill(me, r.skill) ||
        (r.status != "PENDING" && r.status != "OFFERED")) {

        cout << "You cannot respond to this request.\n";
        return;
    }

    cout << "\n1. Accept request and offer a time\n";
    cout << "2. Reject request\n";

    int action = readInt("Choose an action: ");

    if (action == 2) {
        r.status = "REJECTED";

        // Remove any previous offers for this request.
        offers_.erase(
            remove_if(
                offers_.begin(),
                offers_.end(),
                [requestId](const MentorOffer& offer) {
                    return offer.requestId == requestId;
                }
            ),
            offers_.end()
        );

        saveData();

        cout << "\nRequest #" << r.id << " rejected.\n";
        cout << "The learner will see the rejection "
             << "in My bookings.\n";
        return;
    }

    if (action != 1) {
        cout << "Invalid action.\n";
        return;
    }

    for (const auto& offer : offers_) {
        if (offer.requestId == requestId &&
            offer.mentorId == me.id) {

            cout << "You have already offered a time.\n";
            return;
        }
    }

    string time = readText("Your available day/time: ");

    if (time.empty()) {
        cout << "Time cannot be empty.\n";
        return;
    }

    offers_.push_back({requestId, me.id, time});
    r.status = "OFFERED";

    saveData();

    cout << "Offer sent successfully!\n";
    cout << "The learner can now view and accept your timing.\n";
}

void Platform::myBookings(int userIndex) {
    const string learnerId = users_[userIndex].id;

    cout << "\n--- MY BOOKINGS ---\n";

    bool any = false;
    bool hasRejected = false;

    for (const auto& r : requests_) {
        if (r.learnerId != learnerId)
            continue;

        any = true;

        cout << "\nRequest #" << r.id
             << " | Skill: " << r.skill
             << " | Mentor: " << r.targetMentorId
             << " | Status: " << r.status
             << " | Preferred time: " << r.preference
             << '\n';

        if (r.status == "REJECTED") {
            cout << "  NOTIFICATION: Your selected mentor ("
                 << r.targetMentorId
                 << ") rejected your request.\n";

            cout << "  You can book again and choose another mentor.\n";

            hasRejected = true;
        }

        for (const auto& offer : offers_) {
            if (offer.requestId == r.id) {
                cout << "  Mentor " << offer.mentorId
                     << " offered: " << offer.proposedTime << '\n';
            }
        }

        if (r.status == "CONFIRMED") {
            cout << "  Confirmed mentor: "
                 << r.confirmedMentorId
                 << " | Time: " << r.confirmedTime << '\n';
        }
    }

    if (!any) {
        cout << "You have not booked a session yet.\n";
        return;
    }

    if (hasRejected) {
        cout << "\nRejected requests are saved, so "
             << "you can see this notice again later.\n";
    }

    int requestId = readInt(
        "\nRequest ID to accept an offer (0 to return): "
    );

    if (requestId == 0)
        return;

    int pos = findRequest(requestId);

    if (pos < 0 ||
        requests_[pos].learnerId != learnerId ||
        (requests_[pos].status != "PENDING" &&
         requests_[pos].status != "OFFERED")) {

        cout << "This request cannot be accepted.\n";
        return;
    }

    string mentorId = readText(
        "Mentor ID whose offer you accept: "
    );

    int offerPos = -1;

    for (size_t i = 0; i < offers_.size(); ++i) {
        if (offers_[i].requestId == requestId &&
            offers_[i].mentorId == mentorId) {

            offerPos = static_cast<int>(i);
            break;
        }
    }

    if (offerPos < 0) {
        cout << "No offer from that mentor was found.\n";
        return;
    }

    BookingRequest& r = requests_[pos];

    // Only the mentor selected by the learner may be accepted.
    if (mentorId != r.targetMentorId) {
        cout << "This offer is not from your selected mentor.\n";
        return;
    }

    r.status = "CONFIRMED";
    r.confirmedMentorId = offers_[offerPos].mentorId;
    r.confirmedTime = offers_[offerPos].proposedTime;

    Session s;
    s.id = nextSessionId_++;
    s.requestId = r.id;
    s.learnerId = r.learnerId;
    s.mentorId = r.confirmedMentorId;
    s.skill = r.skill;
    s.scheduledTime = r.confirmedTime;
    s.status = "SCHEDULED";

    sessions_.push_back(s);

    int mentorIndex = findUser(r.confirmedMentorId);
    if (mentorIndex >= 0) {
         ++users_[mentorIndex].teachingSessions;
    }   

    ++users_[userIndex].learningSessions;

    saveData();

    cout << "\nBooking confirmed!\n";
    cout << "Session ID: " << s.id << '\n';
    cout << "Mentor: " << s.mentorId << '\n';
    cout << "Scheduled time: " << s.scheduledTime << '\n';
}

void Platform::showMySessions(int userIndex, bool teaching) const {
    const string& id = users_[userIndex].id;
    cout << (teaching ? "\n--- MY TEACHING SESSIONS ---\n" : "\n--- MY LEARNING SESSIONS ---\n");
    bool any = false;
    for (const auto& s : sessions_) {
        if ((teaching && s.mentorId == id) || (!teaching && s.learnerId == id)) {
            cout << "Session #" << s.id << " | " << s.skill
                 << " | learner: " << s.learnerId << " | mentor: " << s.mentorId
                 << " | time: " << s.scheduledTime << " | status: " << s.status << '\n';
            any = true;
        }
    }
    if (!any) cout << "No sessions found.\n";
}

void Platform::updateSkills(int userIndex) {
    
    users_[userIndex].skills = readText("Teaching skills (separate with semicolons): ");
    saveData();
    cout << "Skills updated.\n";
}

void Platform::showLeaderboard() const {
    vector<User> ranked=users_;

    mergeSort(ranked,[](const User& a,const User& b){
        if(a.teachingPoints!=b.teachingPoints)
            return a.teachingPoints>b.teachingPoints;
        return a.id<b.id;
    });

    cout << "\n--- TEACHING LEADERBOARD ---\n";
    cout << left << setw(8) << "Rank" << setw(16) << "User ID"
        << setw(24) << "Name" << setw(18) << "Teaching Points" << "Sessions\n";

    for(size_t i=0;i<ranked.size();++i){
        cout << left << setw(8) << i+1 << setw(16) << ranked[i].id
            << setw(24) << ranked[i].name << setw(18) << fixed << setprecision(1)
            << ranked[i].teachingPoints << ranked[i].teachingSessions << '\n';
    }
    if(ranked.empty()) cout << "No users registered yet.\n";
}

void Platform::showAllUsers() const {
    cout << "\n--- ALL USERS ---\n";
    for (const auto& u : users_)
        cout << u.id << " | " << u.name << " | skills: "
             << (u.skills.empty() ? "(none)" : u.skills)
             << " | teaching points: " << u.teachingPoints << '\n';
    if (users_.empty()) cout << "No users registered.\n";
}

void Platform::showAllRequestsAndSessions() const {
    cout << "\n--- ALL BOOKING REQUESTS ---\n";
    for (const auto& r : requests_)
        cout << "Request #" << r.id << " | learner: " << r.learnerId
             << " | skill: " << r.skill << " | status: " << r.status
             << " | preference: " << r.preference << '\n';
    if (requests_.empty()) cout << "No booking requests.\n";
    cout << "\n--- ALL SESSIONS ---\n";
    for (const auto& s : sessions_)
        cout << "Session #" << s.id << " | request #" << s.requestId
             << " | learner: " << s.learnerId << " | mentor: " << s.mentorId
             << " | skill: " << s.skill << " | time: " << s.scheduledTime
             << " | status: " << s.status << '\n';
    if (sessions_.empty()) cout << "No confirmed sessions.\n";
}

void Platform::completeSession(int sessionIndex) {
    Session& s = sessions_[sessionIndex];
    s.status = "COMPLETED";
    int reqIndex = findRequest(s.requestId);
    if (reqIndex >= 0) requests_[reqIndex].status = "CLOSED";
    saveData();
    cout << "Session marked completed.\n"<< "The learner can now rate the mentor, and the mentor can grade the learner's quiz.\n";
}

void Platform::applySessionOutcome(
    int sessionIndex,
    const std::string& outcome
) {
    if (sessionIndex < 0 ||
        sessionIndex >= static_cast<int>(sessions_.size())) {
        std::cout << "Invalid session index.\n";
        return;
    }

    Session& session = sessions_[sessionIndex];

    // Prevent applying an outcome more than once.
    if (session.status == "COMPLETED" ||
        session.status == "NO_SHOW_LEARNER" ||
        session.status == "NO_SHOW_MENTOR" ||
        session.status == "RESCHEDULED") {
        std::cout << "This session already has a final outcome.\n";
        return;
    }

    if (outcome == "COMPLETED") {
        session.status = "COMPLETED";
        std::cout << "Session marked as completed.\n";
    }
    else if (outcome == "NO_SHOW_LEARNER") {
        int learnerIndex = findUser(session.learnerId);

        if (learnerIndex < 0 ||
            learnerIndex >= static_cast<int>(users_.size())) {
            std::cout << "Learner not found.\n";
            return;
        }

        users_[learnerIndex].totalPoints =
            std::max(0.0, users_[learnerIndex].totalPoints - 5.0);

        session.status = "NO_SHOW_LEARNER";

        std::cout << "Learner no-show confirmed.\n";
        std::cout << "5 learner points deducted.\n";
    }
    else if (outcome == "NO_SHOW_MENTOR") {
        int mentorIndex = findUser(session.mentorId);

        if (mentorIndex < 0 ||
            mentorIndex >= static_cast<int>(users_.size())) {
            std::cout << "Mentor not found.\n";
            return;
        }

        users_[mentorIndex].teachingPoints =
            std::max(0.0, users_[mentorIndex].teachingPoints - 5.0);

        session.status = "NO_SHOW_MENTOR";

        std::cout << "Mentor no-show confirmed.\n";
        std::cout << "5 teaching points deducted.\n";
    }
    else if (outcome == "RESCHEDULED") {
        session.status = "RESCHEDULED";
        std::cout << "Session marked for rescheduling.\n";
    }
    else {
        std::cout << "Invalid session outcome.\n";
        return;
    }

    saveData();
}

void Platform::resolveSessionDispute() {
    bool found = false;

    for (int i = 0; i < static_cast<int>(sessions_.size()); ++i) {
        if (sessions_[i].status != "DISPUTED") {
            continue;
        }

        found = true;
        const Session& session = sessions_[i];

        std::cout << "\n================================\n";
        std::cout << "Disputed Session\n";
        std::cout << "================================\n";
        std::cout << "Session ID: " << session.id << '\n';
        std::cout << "Learner: " << session.learnerId << '\n';
        std::cout << "Mentor: " << session.mentorId << '\n';
        std::cout << "Skill: " << session.skill << '\n';
        std::cout << "Scheduled time: "
                  << session.scheduledTime << '\n';

        std::cout << "\nChoose the final outcome:\n";
        std::cout << "1. Session completed\n";
        std::cout << "2. Learner was absent\n";
        std::cout << "3. Mentor was absent\n";
        std::cout << "4. Reschedule session\n";
        std::cout << "5. Leave disputed for later\n";
        std::cout << "Choice: ";

        int choice;

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(
                std::numeric_limits<std::streamsize>::max(), '\n'
            );
            std::cout << "Invalid input.\n";
            return;
        }

        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(), '\n'
        );

        std::string outcome;

        switch (choice) {
            case 1:
                outcome = "COMPLETED";
                break;

            case 2:
                outcome = "NO_SHOW_LEARNER";
                break;

            case 3:
                outcome = "NO_SHOW_MENTOR";
                break;

            case 4:
                outcome = "RESCHEDULED";
                break;

            case 5:
                std::cout << "Dispute left unresolved.\n";
                continue;

            default:
                std::cout << "Invalid choice. Dispute left unresolved.\n";
                continue;
        }

        applySessionOutcome(i, outcome);
    }

    if (!found) {
        std::cout << "No disputed sessions found.\n";
    }
}

void Platform::rateCompletedSession(int userIndex) {
    const string learnerId = users_[userIndex].id;

    cout << "\n--- RATE A COMPLETED SESSION ---\n";
    bool any = false;

    for (const auto& s : sessions_) {
        if (s.learnerId == learnerId &&
            s.status == "COMPLETED" &&
            s.learnerRating == -1) {
            cout << "Session #" << s.id
                 << " | Skill: " << s.skill
                 << " | Mentor: " << s.mentorId
                 << " | Time: " << s.scheduledTime << '\n';
            any = true;
        }
    }

    if (!any) {
        cout << "No unrated completed sessions available.\n";
        return;
    }

    int sessionId = readInt("Session ID to rate (0 to cancel): ");
    if (sessionId == 0) return;

    int pos = -1;
    for (size_t i = 0; i < sessions_.size(); ++i) {
        if (sessions_[i].id == sessionId) {
            pos = static_cast<int>(i);
            break;
        }
    }

    if (pos < 0 ||
        sessions_[pos].learnerId != learnerId ||
        sessions_[pos].status != "COMPLETED" ||
        sessions_[pos].learnerRating != -1) {
        cout << "Invalid session or rating already submitted.\n";
        return;
    }

    int rating;
    do {
        rating = readInt("Rate your mentor from 1 to 5: ");
        if (rating < 1 || rating > 5)
            cout << "Rating must be between 1 and 5.\n";
    } while (rating < 1 || rating > 5);

    sessions_[pos].learnerRating = rating;

    int mentorIndex = findUser(sessions_[pos].mentorId);
    if (mentorIndex >= 0) {
        double earnedPoints = rating * 5.0;
        users_[mentorIndex].teachingPoints += earnedPoints;

        cout << "Rating submitted successfully!\n"
             << "Rating: " << rating << "/5\n"
             << "Teaching points awarded: " << earnedPoints << '\n';
    }

    saveData();
}

void Platform::gradeLearnerQuiz(int userIndex) {
    const string mentorId = users_[userIndex].id;

    cout << "\n--- GRADE LEARNER QUIZ ---\n";
    bool any = false;

    for (const auto& s : sessions_) {
        if (s.mentorId == mentorId &&
            s.status == "COMPLETED" &&
            s.quizScore == -1) {
            cout << "Session #" << s.id
                 << " | Skill: " << s.skill
                 << " | Learner: " << s.learnerId
                 << " | Time: " << s.scheduledTime << '\n';
            any = true;
        }
    }

    if (!any) {
        cout << "No completed sessions waiting for quiz grading.\n";
        return;
    }

    int sessionId = readInt("Session ID to grade (0 to cancel): ");
    if (sessionId == 0) return;

    int pos = -1;
    for (size_t i = 0; i < sessions_.size(); ++i) {
        if (sessions_[i].id == sessionId) {
            pos = static_cast<int>(i);
            break;
        }
    }

    if (pos < 0 ||
        sessions_[pos].mentorId != mentorId ||
        sessions_[pos].status != "COMPLETED" ||
        sessions_[pos].quizScore != -1) {
        cout << "Invalid session or quiz already graded.\n";
        return;
    }

    int score;
    do {
        score = readInt("Enter learner's quiz marks (0 to 5): ");
        if (score < 0 || score > 5)
            cout << "Quiz marks must be between 0 and 5.\n";
    } while (score < 0 || score > 5);

    sessions_[pos].quizScore = score;

    int learnerIndex = findUser(sessions_[pos].learnerId);
    if (learnerIndex >= 0) {
        users_[learnerIndex].totalPoints += score;

        cout << "Quiz graded successfully!\n"
             << "Quiz score: " << score << "/5\n"
             << "Overall points added to learner: " << score << '\n';
    }

    saveData();
}

void Platform::reportSessionOutcome(int userIndex) {
    const string userId = users_[userIndex].id;

    cout << "\n--- SESSION ATTENDANCE ---\n";

    bool any = false;

    for (const auto& s : sessions_) {
        bool isLearner = (s.learnerId == userId);
        bool isMentor = (s.mentorId == userId);

        if (!isLearner && !isMentor)
            continue;

        string myOutcome = isLearner
            ? s.learnerOutcome : s.mentorOutcome;

        string otherOutcome = isLearner
            ? s.mentorOutcome : s.learnerOutcome;

        bool canReport =
            s.status == "SCHEDULED" &&
            myOutcome == "PENDING" &&
            otherOutcome == "PENDING";

        bool canRespond =
            s.status == "AWAITING_CONFIRMATION" &&
            myOutcome == "PENDING" &&
            (otherOutcome == "NO_SHOW_LEARNER" ||
             otherOutcome == "NO_SHOW_MENTOR");

        if (canReport || canRespond) {
            cout << "Session #" << s.id
                 << " | Skill: " << s.skill
                 << " | Learner: " << s.learnerId
                 << " | Mentor: " << s.mentorId
                 << " | Time: " << s.scheduledTime;

            if (canRespond) {
                cout << "\n  NOTIFICATION: The other participant reported "
                     << otherOutcome;
            }

            cout << '\n';
            any = true;
        }
    }

    if (!any) {
        cout << "No sessions need an attendance report or response.\n";
        return;
    }

    int sessionId = readInt("Enter session ID (0 to cancel): ");

    if (sessionId == 0)
        return;

    int pos = -1;

    for (size_t i = 0; i < sessions_.size(); ++i) {
        if (sessions_[i].id == sessionId) {
            pos = static_cast<int>(i);
            break;
        }
    }

    if (pos < 0) {
        cout << "Session not found.\n";
        return;
    }

    Session& s = sessions_[pos];

    bool isLearner = (s.learnerId == userId);
    bool isMentor = (s.mentorId == userId);

    if (!isLearner && !isMentor) {
        cout << "You are not part of this session.\n";
        return;
    }

    string& myOutcome = isLearner
        ? s.learnerOutcome : s.mentorOutcome;

    string& otherOutcome = isLearner
        ? s.mentorOutcome : s.learnerOutcome;

    // Respond to an existing no-show report.
    if (s.status == "AWAITING_CONFIRMATION" &&
        myOutcome == "PENDING" &&
        (otherOutcome == "NO_SHOW_LEARNER" ||
         otherOutcome == "NO_SHOW_MENTOR")) {

        cout << "\nThe other participant reported: "
             << otherOutcome << '\n'
             << "1. Agree with the report\n"
             << "2. Disagree and raise a dispute\n";

        int choice = readInt("Choose: ");

        if (choice == 1) {
            string confirmedOutcome = otherOutcome;
            myOutcome = "AGREE";

            applySessionOutcome(pos, confirmedOutcome);

            cout << "Report accepted. Final outcome recorded.\n";
        }
        else if (choice == 2) {
            myOutcome = "DISAGREE";
            s.status = "DISPUTED";

            saveData();

            cout << "Dispute sent to admin.\n"
                 << "No points have been deducted.\n";
        }
        else {
            cout << "Invalid choice.\n";
        }

        return;
    }

    // A new report can only be made for a scheduled session.
    if (s.status != "SCHEDULED" ||
        myOutcome != "PENDING" ||
        otherOutcome != "PENDING") {
        cout << "This session is no longer available for reporting.\n";
        return;
    }

    cout << "\n1. Mark session as completed\n";

    if (isMentor) {
        cout << "2. Report that the learner did not attend\n";
    }
    else {
        cout << "2. Report that the mentor did not attend\n";
    }

    int choice = readInt("Choose: ");

    if (choice == 1) {
        myOutcome = "COMPLETED";

        applySessionOutcome(pos, "COMPLETED");
    }
    else if (choice == 2) {
        string noShowOutcome = isMentor
            ? "NO_SHOW_LEARNER"
            : "NO_SHOW_MENTOR";

        myOutcome = noShowOutcome;
        s.status = "AWAITING_CONFIRMATION";

        saveData();

        cout << "No-show report submitted.\n"
             << "The other participant will be notified at login.\n"
             << "No points have been deducted yet.\n";
    }
    else {
        cout << "Invalid choice.\n";
    }
}

