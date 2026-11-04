# Skill Issue — User Manual (README)

Peer-to-peer skill exchange: teach what you know, learn what you don't, earn points on a leaderboard.

## 1. Requirements
- Linux/macOS/Windows with **g++ 9+** (C++17) and **make** (Windows: MinGW or WSL)
- For the web UI only: **Node.js 18+**

## 2. Build
```bash
cd SkillIssue_REG1_REG2_REG3_CSLR31_Project
make            # builds ./a.out (console app) and ./server (web API)
make test       # optional: runs the 59 automated tests
```

## 3. Run
**Console version**
```bash
./a.out
```
**Web version** (two terminals, both started from the project folder)
```bash
./server                       # API on http://localhost:8080
cd frontend && npm install && npm run dev   # UI on http://localhost:5173
```
Keep the `data/` folder next to where you start the program (it holds the CSV files).

## 4. Using the app
1. **Register** with an ID, name, password and your teachable skills separated by `;` (e.g. `C++;Python;Guitar`).
2. **Log in.** (Console admin: ID `admin`, password `admin123`.)
3. **Learn:** choose a skill → pick a mentor from the ranked list → enter your preferred time → *Book*.
4. **Teach:** open **Mentor Inbox**, then *Accept* with a proposed time, or *Reject*.
5. **Confirm:** under **My Bookings**, accept the mentor's offer → a session is scheduled.
6. **After the session:** the mentor reports *Completed / Missed / Cancelled*; the learner rates the mentor (1–5); the mentor grades the learner's quiz.
7. **Leaderboard** shows mentors ranked by teaching points. **Profile** shows your points and session counts; you can edit your skills there.

## 5. Points
Learner rating × 5 → mentor teaching points (console). Quiz marks → learner points. A confirmed no-show loses 5 points. New users start with 30 points.

## 6. Troubleshooting
| Problem | Fix |
|---|---|
| "Invalid user ID or password" | IDs/passwords are case-sensitive |
| Web UI cannot reach API | Start `./server` first; port 8080 must be free |
| Data looks empty | Run from the folder that contains `data/` |
| Reset everything | Delete the four CSV files in `data/` (keep a backup) |

*Prototype notice:* passwords are stored in plain text — do not use real passwords.
