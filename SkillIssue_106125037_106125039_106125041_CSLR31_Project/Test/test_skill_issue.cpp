/**
 * test_skill_issue.cpp - Automated tests for the Skill Issue backend.
 *
 * Runs against the CSVs in Test/testdata (copied into ./data), so the
 * real project data is never touched. Each CHECK prints PASS/FAIL; the process
 * exit code is the number of failures (0 = all passed).
 */
#include "hashmap.h"
#include "platform.h"
#include "priority_queue.h"
#include "sorting.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

static int passed = 0, failed = 0;

// Records one test result. `name` is the test-case ID shown in the report.
static void check(const string& name, bool ok) {
    cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n';
    ok ? ++passed : ++failed;
}

int main() {
    string msg;

    // Work in a throw-away folder so the real ./data is never modified.
    // Works on Windows, Linux and macOS (no shell script needed).
    namespace fs = std::filesystem;
    fs::path workDir = fs::temp_directory_path() / "skill_issue_test_run";
    fs::path testData = fs::absolute("Test/testdata");
    fs::remove_all(workDir);
    fs::create_directories(workDir / "data");
    fs::copy(testData, workDir / "data", fs::copy_options::recursive);
    fs::current_path(workDir);

    // ---------------------------------------------------------------- Auth
    {
        Platform p;
        check("TC01 register new user succeeds", p.registerUserAPI("n1", "New", "pw", "Go", msg));
        check("TC02 duplicate ID rejected", !p.registerUserAPI("u1", "X", "pw", "Go", msg));
        check("TC03 reserved ID 'admin' rejected", !p.registerUserAPI("admin", "X", "pw", "", msg));
        check("TC04 empty ID rejected", !p.registerUserAPI("", "X", "pw", "", msg));
        check("TC05 empty password rejected", !p.registerUserAPI("n2", "X", "", "", msg));
        check("TC06 login correct password", p.loginAPI("u1", "pw1", msg));
        check("TC07 login wrong password", !p.loginAPI("u1", "bad", msg));
        check("TC08 login unknown user", !p.loginAPI("ghost", "pw", msg));

        User prof;
        check("TC09 profile found, password hidden",
              p.getProfileAPI("u1", prof) && prof.name == "Asha" && prof.password.empty());
        check("TC10 profile unknown user", !p.getProfileAPI("ghost", prof));
        check("TC11 update skills", p.updateSkillsAPI("u4", "Dance;Music", msg));
        check("TC12 update skills unknown user", !p.updateSkillsAPI("ghost", "X", msg));
    }

    // ----------------------------------------------- Matching + leaderboard
    {
        Platform p;
        vector<User> m = p.getMatchingMentorsAPI("python", "u5");  // case-insensitive
        // u1 and u2 tie on points (40) -> more sessions first (u2: 5 > u1: 3); u3 last.
        check("TC13 matching mentors sorted by points, then sessions",
              m.size() == 3 && m[0].id == "u2" && m[1].id == "u1" && m[2].id == "u3");
        check("TC14 matching excludes the learner themself",
              p.getMatchingMentorsAPI("python", "u1").size() == 2);
        check("TC15 matching with unknown learner is empty",
              p.getMatchingMentorsAPI("python", "ghost").empty());
        check("TC16 matching for untaught skill is empty",
              p.getMatchingMentorsAPI("Cooking", "u5").empty());

        vector<User> lb = p.getLeaderboardAPI();
        check("TC17 leaderboard sorted by teaching points desc, ID tiebreak",
              lb.size() >= 5 && lb[0].id == "u1" && lb[1].id == "u2" && lb[2].id == "u5");
        check("TC18 leaderboard hides passwords", lb[0].password.empty());
    }

    // ------------------------------------------------- Booking full workflow
    {
        Platform p;
        check("TC19 book unknown mentor fails", !p.bookSessionAPI("u5", "Python", "ghost", "Mon", msg));
        check("TC20 book yourself fails", !p.bookSessionAPI("u1", "Python", "u1", "Mon", msg));
        check("TC21 book skill mentor lacks fails", !p.bookSessionAPI("u5", "Dance", "u1", "Mon", msg));
        check("TC22 book valid request", p.bookSessionAPI("u5", "Python", "u1", "Mon", msg));

        check("TC23 mentor inbox shows request to target mentor", p.getMentorInboxAPI("u1").size() == 1);
        check("TC24 other mentor inbox empty", p.getMentorInboxAPI("u2").empty());
        check("TC25 non-target mentor cannot respond",
              !p.respondToMentorRequestAPI("u2", 1, "accept", "Tue", msg));
        check("TC26 invalid action rejected",
              !p.respondToMentorRequestAPI("u1", 1, "maybe", "Tue", msg));
        check("TC27 accept needs a proposed time",
              !p.respondToMentorRequestAPI("u1", 1, "accept", "", msg));
        check("TC28 mentor offers a time", p.respondToMentorRequestAPI("u1", 1, "accept", "Tue 5pm", msg));
        check("TC29 duplicate offer rejected", !p.respondToMentorRequestAPI("u1", 1, "accept", "Wed", msg));

        auto bookings = p.getMyBookingsAPI("u5");
        check("TC30 learner sees request with its offer",
              bookings.size() == 1 && bookings[0].request.status == "OFFERED" && bookings[0].offers.size() == 1);

        check("TC31 wrong learner cannot accept", !p.acceptBookingOfferAPI("u3", 1, "u1", msg));
        check("TC32 accept offer from non-selected mentor fails", !p.acceptBookingOfferAPI("u5", 1, "u2", msg));
        check("TC33 learner accepts offer", p.acceptBookingOfferAPI("u5", 1, "u1", msg));
        check("TC34 accepting twice fails", !p.acceptBookingOfferAPI("u5", 1, "u1", msg));

        User mentor, learner;
        p.getProfileAPI("u1", mentor);
        p.getProfileAPI("u5", learner);
        check("TC35 session counters incremented",
              mentor.teachingSessions == 4 && learner.learningSessions == 1);

        auto sess = p.getMySessionsAPI("u1", true);
        check("TC36 mentor sees scheduled session", sess.size() == 1 && sess[0].status == "SCHEDULED");

        // ---- post-session: outcome, rating, quiz
        check("TC37 rating before completion fails", !p.rateSessionAPI("u5", 1, 5, msg));
        check("TC38 invalid outcome rejected", !p.reportOutcomeAPI("u1", 1, "DONE", msg));
        check("TC39 learner cannot report as mentor", !p.reportOutcomeAPI("u5", 1, "COMPLETED", msg));
        check("TC40 mentor reports COMPLETED", p.reportOutcomeAPI("u1", 1, "COMPLETED", msg));
        check("TC41 outcome cannot be reported twice", !p.reportOutcomeAPI("u1", 1, "COMPLETED", msg));
        check("TC42 rating out of range (0) fails", !p.rateSessionAPI("u5", 1, 0, msg));
        check("TC43 rating out of range (6) fails", !p.rateSessionAPI("u5", 1, 6, msg));
        check("TC44 other learner cannot rate", !p.rateSessionAPI("u3", 1, 5, msg));
        check("TC45 learner rates session", p.rateSessionAPI("u5", 1, 5, msg));
        check("TC46 double rating fails", !p.rateSessionAPI("u5", 1, 4, msg));
        check("TC47 quiz score out of range fails", !p.gradeQuizAPI("u1", 1, 101, msg));
        check("TC48 non-mentor cannot grade quiz", !p.gradeQuizAPI("u5", 1, 80, msg));
        check("TC49 mentor grades quiz", p.gradeQuizAPI("u1", 1, 80, msg));
        check("TC50 grade quiz unknown session fails", !p.gradeQuizAPI("u1", 99, 80, msg));
    }

    // ---------------------------------------------------- Reject + persistence
    {
        Platform p;
        p.bookSessionAPI("u4", "Python", "u3", "Fri", msg);
        // Request IDs continue from the CSV written by the previous block.
        int rid = static_cast<int>(p.getMyBookingsAPI("u4").back().request.id);
        check("TC51 mentor rejects request", p.respondToMentorRequestAPI("u3", rid, "reject", "", msg));
        check("TC52 rejected request leaves inbox", p.getMentorInboxAPI("u3").empty());
        check("TC53 rejected request cannot be accepted",
              !p.acceptBookingOfferAPI("u4", rid, "u3", msg));
    }
    {
        Platform reloaded;  // fresh object reads the CSV files written above
        User u;
        check("TC54 registered user persisted across restart", reloaded.getProfileAPI("n1", u));
        check("TC55 sessions persisted across restart", reloaded.getMySessionsAPI("u5", false).size() == 1);
    }


    // ------------------------------------------- Hash map + priority queue
    {
        HashMap<string, int> hm(4);
        for (int i = 0; i < 1000; ++i) hm.put("key" + to_string(i), i);   // forces many resizes
        int v = -1;
        bool allFound = true;
        for (int i = 0; i < 1000; ++i)
            allFound = allFound && hm.get("key" + to_string(i), v) && v == i;
        check("TC60 hashmap stores and finds 1000 keys after resizing", allFound && hm.size() == 1000);
        hm.put("key5", 55);
        check("TC61 hashmap put overwrites existing key", hm.get("key5", v) && v == 55 && hm.size() == 1000);
        check("TC62 hashmap missing key not found", !hm.get("nope", v) && !hm.contains("nope"));
        check("TC63 hashmap remove works", hm.remove("key7") && !hm.contains("key7") && !hm.remove("key7") && hm.size() == 999);
        HashMap<int, int> im;
        im.put(42, 1); im.put(-3, 2);
        check("TC64 hashmap works with int keys", im.get(42, v) && v == 1 && im.get(-3, v) && v == 2);

        auto higher = [](int a, int b) { return a > b; };
        PriorityQueue<int, decltype(higher)> pq(higher);
        check("TC65 empty priority queue", pq.empty() && pq.size() == 0);
        bool threw = false;
        try { pq.top(); } catch (const out_of_range&) { threw = true; }
        check("TC66 top() on empty queue throws", threw);
        for (int x : {5, 1, 9, 3, 9, 7, 2}) pq.push(x);
        vector<int> out;
        while (!pq.empty()) { out.push_back(pq.top()); pq.pop(); }
        check("TC67 priority queue pops in priority order", out == vector<int>({9, 9, 7, 5, 3, 2, 1}));

        Platform p;
        vector<User> top2 = p.getTopMentorsAPI(2);
        check("TC68 top-2 mentors by teaching points", top2.size() == 2 && top2[0].id == "u1" && top2[1].id == "u2");
        check("TC69 top-k with k larger than users returns all users",
              p.getTopMentorsAPI(100).size() == p.getLeaderboardAPI().size());
        check("TC70 top-k with k<=0 is empty", p.getTopMentorsAPI(0).empty());
        check("TC71 top-k matches leaderboard order",
              p.getTopMentorsAPI(3)[2].id == p.getLeaderboardAPI()[2].id);
    }

    // ------------------------------------------- CRLF (Windows) CSV files
    {
        // Rewrite the CSV files with Windows line endings AND a stray CR inside
        // the quoted values (this is what broke the learner's bookings page).
        {
            ofstream rq("data/booking_requests.csv", ios::binary);
            rq << "request_id,learner_id,skill,preference,status,confirmed_mentor_id,confirmed_time,target_mentor_id\r\n"
               << "7,\"u5\",\"Python\",\"Mon\",\"OFFERED\",\"\",\"\",\"u1\r\"\r\n";
            ofstream of("data/mentor_offers.csv", ios::binary);
            of << "request_id,mentor_id,proposed_time\r\n" << "7,\"u1\",\"Tue 6pm\r\"\r\n";
        }
        Platform p;
        auto bk = p.getMyBookingsAPI("u5");
        check("TC72 CRLF/CR in CSV values is ignored when loading",
              bk.size() == 1 && bk[0].request.targetMentorId == "u1" &&
              bk[0].offers.size() == 1 && bk[0].offers[0].proposedTime == "Tue 6pm");
        check("TC73 mentor inbox works with CRLF data", p.getMentorInboxAPI("u1").size() == 1);
    }

    // ------------------------------------------------------------ Merge sort
    {
        vector<int> empty, one{7}, v{5, 2, 9, 1, 5, 6};
        mergeSort(empty, [](int a, int b) { return a < b; });
        mergeSort(one,   [](int a, int b) { return a < b; });
        mergeSort(v,     [](int a, int b) { return a < b; });
        check("TC56 mergeSort empty/single vector", empty.empty() && one.size() == 1 && one[0] == 7);
        check("TC57 mergeSort ascending", v == vector<int>({1, 2, 5, 5, 6, 9}));

        // Stability: sort pairs by .first only; equal keys must keep original order.
        vector<pair<int,int>> pr{{1,0},{0,1},{1,2},{0,3},{1,4}};
        mergeSort(pr, [](const pair<int,int>& a, const pair<int,int>& b) { return a.first < b.first; });
        check("TC58 mergeSort is stable",
              pr == vector<pair<int,int>>({{0,1},{0,3},{1,0},{1,2},{1,4}}));

        // Randomised comparison against std::stable_sort on 2000 elements.
        mt19937 rng(42);
        vector<int> big(2000), ref;
        for (int& x : big) x = static_cast<int>(rng() % 100);
        ref = big;
        mergeSort(big, [](int a, int b) { return a > b; });
        stable_sort(ref.begin(), ref.end(), [](int a, int b) { return a > b; });
        check("TC59 mergeSort matches std::stable_sort on 2000 random ints", big == ref);
    }

    cout << "\nSummary: " << passed << " passed, " << failed << " failed\n";
    fs::current_path(fs::temp_directory_path());
    fs::remove_all(workDir);
    return failed;
}
