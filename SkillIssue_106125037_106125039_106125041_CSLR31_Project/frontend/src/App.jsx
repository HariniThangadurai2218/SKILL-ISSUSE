import { useState } from "react";
import "./App.css";

function App() {
  const [page, setPage] = useState("login");
  const [message, setMessage] = useState("");
  const [id, setId] = useState("");
  const [name, setName] = useState("");
  const [phone, setPhone] = useState("");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [skills, setSkills] = useState("");
  const [loginId,setLoginId]=useState("");
  const [loginPassword,setLoginPassword]=useState("");
  const [feature,setFeature]=useState("home");
  const [profile,setProfile]=useState(null);
  const [leaderboard,setLeaderboard]=useState([]);
  const [teachingSessions,setTeachingSessions] = useState([]);
  const [learningSessions,setLearningSessions] = useState([]);  
  const [newSkills,setNewSkills]=useState("");
  const [mentorRequests,setMentorRequests]=useState([]);
  const [proposedTimes,setProposedTimes]=useState([]);
  const [myBookings,setMyBookings] = useState([]);
  const [quizSession,setQuizSession] = useState("");
  const [quizScore,setQuizScore] = useState(""); 
  const [outcomeSession,setOutcomeSession] = useState("");
  const [sessionOutcome,setSessionOutcome] = useState("COMPLETED"); 
  const [selectedSession,setSelectedSession] = useState("");
  const [rating,setRating] = useState(5);
  const [bookingSkill,setBookingSkill] = useState("");
  const [bookingPreference,setBookingPreference] = useState("");
  const [matchingMentors,setMatchingMentors] = useState([]);
  const [selectedMentor,setSelectedMentor] = useState("");

  async function fetchProfile(){
    const response = await fetch("http://localhost:8080/api/profile",{
      method: "POST",
      body: new URLSearchParams({
          id: loginId
      })
    });

    const data = await response.json();
    setProfile(data);
  }

  async function fetchLeaderboard(){
    const response=await fetch("http://localhost:8080/api/leaderboard");
    const data=await response.json();
    setLeaderboard(data);
  }

  async function fetchSessions(type){
    const response=await fetch(
      `http://localhost:8080/api/sessions?id=${encodeURIComponent(loginId)}&type=${type}`
    );
    const data=await response.json();

    if(type==="teaching"){
      setTeachingSessions(data);
    }
    else{
      setLearningSessions(data);
    }
  }

  async function updateSkills(){
    const response=await fetch("http://localhost:8080/api/skills",{
      method: "POST",
      body: new URLSearchParams({
        id: loginId,
        skills: newSkills
      })
    });

    const data=await response.text();
    setMessage(data);
  }

  async function fetchMentorInbox(){
    const response=await fetch(
      `http://localhost:8080/api/mentor-inbox?id=${encodeURIComponent(loginId)}`
    );

    const data=await response.json();
    setMentorRequests(data);
  }

  async function fetchMyBookings(){
    const response=await fetch(
      `http://localhost:8080/api/my-bookings?id=${encodeURIComponent(loginId)}`
    );

    const data=await response.json();
    setMyBookings(data);
  }

  async function submitQuizScore(){
    if(!quizSession || quizScore===""){
      setMessage("Please select a session and enter a score.");
      return;
    }

    const response=await fetch("http://localhost:8080/api/grade-quiz",{
      method:"POST",
      body:new URLSearchParams({
        mentorId:loginId,
        sessionId:quizSession,
        score:quizScore
      })
    });

    const data=await response.text();
    setMessage(data);

    if(response.ok){
      setQuizScore("");
      setQuizSession("");
    }
  }

  async function submitOutcome(){
    if(!outcomeSession){
      setMessage("Please select a session.");
      return;
    }
    const response=await fetch("http://localhost:8080/api/session-outcome",{
      method:"POST",
      body:new URLSearchParams({
        mentorId:loginId,
        sessionId:outcomeSession,
        outcome:sessionOutcome
      })
    });
    const data=await response.text();
    setMessage(data);

    if(response.ok){
      setOutcomeSession("");
      setSessionOutcome("COMPLETED");
    }
  }

  async function submitRating(){
    if(!selectedSession){
      setMessage("Please select a session.");
      return;
    }
    const response=await fetch("http://localhost:8080/api/rate-session",{
      method:"POST",
      body:new URLSearchParams({
        learnerId:loginId,
        sessionId:selectedSession,
        rating:rating
      })
    });
    const data=await response.text();
    setMessage(data);

    if(response.ok){
      setSelectedSession("");
      setRating(5);
      fetchSessions("learning");
    }
  }

  async function acceptOffer(requestId,mentorId){
    const response=await fetch("http://localhost:8080/api/accept-offer",{
      method:"POST",
      body:new URLSearchParams({
        learnerId:loginId,
        requestId:requestId,
        mentorId:mentorId
      })
    });
    const data=await response.text();
    setMessage(data);

    if(response.ok){
      fetchMyBookings();
      fetchSessions("learning");
    }
  }

  async function fetchMatchingMentors(){
    if(!bookingSkill.trim()){
      setMessage("Enter a skill first.");
      return;
    }
    const response=await fetch(
      `http://localhost:8080/api/matching-mentors?skill=${encodeURIComponent(bookingSkill)}&learnerId=${encodeURIComponent(loginId)}`
    );
    const data=await response.json();

    setMatchingMentors(data);
    setSelectedMentor("");

    if(data.length===0){
      setMessage("No mentors found for this skill.");
    }
    else{
      setMessage("");
    }
  }

  async function submitBooking(){
    if(!selectedMentor){
      setMessage("Select a mentor first.");
      return;
    }
    const response=await fetch("http://localhost:8080/api/book-session",{
      method:"POST",
      body:new URLSearchParams({
        learnerId:loginId,
        skill:bookingSkill,
        mentorId:selectedMentor,
        preference:bookingPreference
      })
    });
    const data=await response.text();
    setMessage(data);

    if(response.ok){
      setBookingSkill("");
      setBookingPreference("");
      setMatchingMentors([]);
      setSelectedMentor("");
    }
  }

  async function respondToRequest(requestId,action){
    const response=await fetch("http://localhost:8080/api/mentor-response",{
        method:"POST",
        body:new URLSearchParams({
          mentorId:loginId,
          requestId:requestId,
          action:action,
          proposedTime:proposedTimes[requestId] || ""
        })
    });
    const data=await response.text();
    setMessage(data);

    if(response.ok){
      fetchMentorInbox();
    }
  }

  return (
    <div className="container">
      <h1>Skill Issue</h1>
      <p>Learn a skill. Teach a skill.</p>

      {page==="dashboard"?(
        <>
          {feature === "home" ? (
            <>
              <h2>Welcome, {loginId}!</h2>
              <h3>What would you like to do?</h3>

              <div className="dashboard">

                <button onClick={() => {
                  setFeature("profile");
                  fetchProfile();
                }}>My Profile</button>

                <button onClick={()=>setFeature("book")}>
                  Book a Session
                </button>     

                <button onClick={() => {
                  setFeature("inbox");
                  setMessage("");
                  fetchMentorInbox();
                }}>Mentor Inbox</button>

                <button onClick={() => {
                  setFeature("bookings");
                  setMessage("");
                  fetchMyBookings();
                }}>My Bookings / Accept an Offer</button>
                
                <button onClick={() => {
                  setFeature("teaching");
                  fetchSessions("teaching");
                }}>My Teaching Sessions</button>

                <button onClick={() => {
                  setFeature("learning");
                  fetchSessions("learning");
                }}>My Learning Sessions</button>

                <button onClick={() => {
                  setFeature("leaderboard");
                  fetchLeaderboard();
                }}>Teaching Leaderboard</button>

                <button onClick={() => {
                  setFeature("skills");
                  setMessage("");
                  setNewSkills("");
                }}>Update Teaching Skills</button>

                <button onClick={() => {
                  setFeature("rating");
                  setMessage("");
                  fetchSessions("learning");
                }}>Rate a Completed Session</button>
                
                <button onClick={() => {
                  setFeature("quiz");
                  setMessage("");
                  fetchSessions("teaching");
                }}>Grade Learner Quiz</button>

                <button onClick={() => {
                  setFeature("outcome");
                  setMessage("");
                  fetchSessions("teaching");
                }}> Session Outcome</button>
              </div>
            </>
        ) : feature === "profile" ? (
          <>
            <h2>My Profile</h2>
            {profile && (
              <>
                <p>Name: {profile.name}</p>
                <p>ID: {profile.id}</p>
                <p>Skills: {profile.skills}</p>
                <p>Total Points: {profile.totalPoints}</p>
                <p>Teaching Points: {profile.teachingPoints}</p>
                <p>Teaching Sessions: {profile.teachingSessions}</p>
                <p>Learning Sessions: {profile.learningSessions}</p>
              </>
            )}
            <button onClick={() => setFeature("home")}>Back to Dashboard</button>
          </>
        ) : feature === "leaderboard" ? (
          <>
            <h2>Teaching Leaderboard</h2>
            {leaderboard.length===0 ? (
              <p>No users registered yet.</p>
            ) : (
                leaderboard.map(user=>(
                  <div key={user.id}>
                    <p>Rank: {user.rank}</p>
                    <p>Name: {user.name}</p>
                    <p>ID: {user.id}</p>
                    <p>Teaching Points: {user.teachingPoints}</p>
                    <p>Teaching Sessions: {user.teachingSessions}</p>
                    <hr />
                  </div>
                ))
            )}
            <button onClick={() => setFeature("home")}>Back to Dashboard</button>
          </>
        ) : feature === "teaching" ? (
          <>
            <h2>My Teaching Sessions</h2>
            {teachingSessions.length===0 ? (
              <p>No teaching sessions found.</p>
            ) : (
              teachingSessions.map(s=>(
                <div key={s.id}>
                  <h3>Session #{s.id}</h3>
                  <p>Skill: {s.skill}</p>
                  <p>Learner: {s.learnerId}</p>
                  <p>Mentor: {s.mentorId}</p>
                  <p>Time: {s.scheduledTime}</p>
                  <p>Status: {s.status}</p>
                  <hr />
                </div>
              ))
            )}
            <button onClick={() => setFeature("home")}>Back to Dashboard</button>
          </>
        ) : feature === "learning" ? (
          <>
            <h2>My Learning Sessions</h2>
            {learningSessions.length===0 ? (
              <p>No learning sessions found.</p>
            ) : (
              learningSessions.map(s=>(
                <div key={s.id}>
                  <h3>Session #{s.id}</h3>
                  <p>Skill: {s.skill}</p>
                  <p>Mentor: {s.mentorId}</p>
                  <p>Learner: {s.learnerId}</p>
                  <p>Time: {s.scheduledTime}</p>
                  <p>Status: {s.status}</p>
                  <hr />
                </div>
              ))
            )}

            <button onClick={() => setFeature("home")}>Back to Dashboard</button>
          </>
        ) : feature === "skills" ? (
          <>
            <h2>Update Teaching Skills</h2>
            <p>Enter your complete teaching skills list, separated by semicolons.</p>
            <p style={{color:"red"}}>
              Important: Your new list will replace ALL your existing teaching skills.
              Skills not included in the new list will be removed.
            </p>
            <input
              type="text"
              value={newSkills}
              onChange={(e) => setNewSkills(e.target.value)}
              placeholder="Python; C++; Web Development"
            />
            <button onClick={updateSkills}>Update Skills</button>
            <p>{message}</p>
            <button onClick={() => setFeature("home")}>Back to Dashboard</button>
          </>
        ) : feature === "inbox" ? (
          <>
            <h2>Mentor Inbox</h2>
            {mentorRequests.length === 0 ? (
              <p>No pending requests assigned to you.</p>
            ) : (
              mentorRequests.map((r) => (
                <div key={r.id} className="request-card">
                  <h3>Request #{r.id}</h3>
                  <p><b>Skill:</b> {r.skill}</p>
                  <p><b>Learner:</b> {r.learnerId}</p>
                  <p><b>Preferred time:</b> {r.preference}</p>
                  <p><b>Status:</b> {r.status}</p>
                  <input
                    type="text"
                    placeholder="Enter your available day/time"
                    value={proposedTimes[r.id] || ""}
                    onChange={(e) => setProposedTimes({
                      ...proposedTimes,
                      [r.id]: e.target.value
                    })}
                  />
                  <div>
                    <button onClick={()=>respondToRequest(r.id,"accept")}>
                      Accept and Offer Time
                    </button>
                    <button onClick={()=>respondToRequest(r.id,"reject")}>
                      Reject Request
                    </button>
                  </div>
                </div>
              ))
            )}
            <p>{message}</p>
            <button onClick={() => setFeature("home")}>
                Back to Dashboard
            </button>
          </>
        ) : feature === "bookings" ? (
          <>
            <h2>My Bookings</h2>
            {myBookings.length===0 ? (
              <p>You haven't booked any sessions yet.</p>
            ) : (
              myBookings.map((b)=>(
                <div className="booking-card" key={b.id}>
                  <h3>Request #{b.id}</h3>
                  <p><b>Skill:</b> {b.skill}</p>
                  <p><b>Preferred time:</b> {b.preference}</p>
                  <p><b>Status:</b> {b.status}</p>
                  {b.status==="PENDING" && (
                    <p>Waiting for mentor response...</p>
                  )}
                  {b.status==="OFFERED" && (
                    <>
                      <h4>Mentor Offers</h4>
                      {b.offers.map((o,index)=>(
                        <div key={index}>
                          <p><b>Mentor:</b> {o.mentorId}</p>
                          <p><b>Proposed time:</b> {o.proposedTime}</p>
                          <button onClick={() => acceptOffer(b.id,o.mentorId)}>
                            Accept Offer
                          </button>
                        </div>
                      ))}
                    </>
                  )}
                  {b.status==="CONFIRMED" && (
                    <>
                      <p><b>Confirmed mentor:</b> {b.mentorId}</p>
                      <p><b>Session time:</b> {b.confirmedTime}</p>
                    </>
                  )}
                  {b.status==="REJECTED" && (
                    <p>Your selected mentor rejected this request. You can book again.</p>
                  )}
                </div>
              ))
            )}
            <p>{message}</p>
            <button onClick={() => setFeature("home")}>
              Back to Dashboard
            </button>
          </>
        ) : feature === "quiz" ? (
          <>
            <h2>Grade Learner Quiz</h2>
            <p>Select the teaching session:</p>
            <select
              value={quizSession}
              onChange={(e) => setQuizSession(e.target.value)}
            >
              <option value="">Select a session</option>
              {teachingSessions.map((s) => (
                <option key={s.id} value={s.id}>
                  Session #{s.id} - {s.skill} (Learner: {s.learnerId})
                </option>
              ))}
            </select>
            <p>Enter learner's quiz score:</p>
            <input
              type="number"
              min="0"
              max="100"
              placeholder="Score out of 100"
              value={quizScore}
              onChange={(e) => setQuizScore(e.target.value)}
            />
            <button onClick={submitQuizScore}>
              Submit Score
            </button>
            <p>{message}</p>
            <button onClick={() => setFeature("home")}>
              Back to Dashboard
            </button>
          </>
        ) : feature === "outcome" ? (
          <>
            <h2>Report Session Outcome</h2>
            <p>Select your teaching session:</p>
            <select
              value={outcomeSession}
              onChange={(e) => setOutcomeSession(e.target.value)}
            >
              <option value="">Select a session</option>
              {teachingSessions.map((s) => (
                <option key={s.id} value={s.id}>
                  Session #{s.id} - {s.skill} (Learner: {s.learnerId})
                </option>
              ))}
            </select>
            <p>Session outcome:</p>
            <select
              value={sessionOutcome}
              onChange={(e) => setSessionOutcome(e.target.value)}
            >
              <option value="COMPLETED">Completed</option>
              <option value="MISSED">Missed</option>
              <option value="CANCELLED">Cancelled</option>
            </select>
            <button onClick={submitOutcome}>
              Submit Outcome
            </button>
            <p>{message}</p>
            <button onClick={() => setFeature("home")}>
              Back to Dashboard
            </button>
          </>
        ) : feature === "rating" ? (
          <>
            <h2>Rate a Completed Session</h2>
            <p>Select a learning session:</p>
            <select
              value={selectedSession}
              onChange={(e) => setSelectedSession(e.target.value)}
            >
              <option value="">Select a session</option>
              {learningSessions
                .filter(s => s.status === "SCHEDULED")
                .map(s => (
                  <option key={s.id} value={s.id}>
                    Session #{s.id} - {s.skill} (Mentor: {s.mentorId})
                  </option>
              ))}
            </select>
            <p>Rate your mentor:</p>
            <select
              value={rating}
              onChange={(e) => setRating(Number(e.target.value))}
            >
              <option value={5}>5 - Excellent</option>
              <option value={4}>4 - Good</option>
              <option value={3}>3 - Average</option>
              <option value={2}>2 - Poor</option>
              <option value={1}>1 - Very Poor</option>
            </select>
            <button onClick={submitRating}>
              Submit Rating
            </button>
            <p>{message}</p>
            <button onClick={() => setFeature("home")}>
              Back to Dashboard
            </button>
          </>
        ) : feature === "book" ? (
          <>
            <h2>Book a Session</h2>
            <label>Skill you want to learn</label>
            <input
              type="text"
              value={bookingSkill}
              onChange={(e)=>setBookingSkill(e.target.value)}
              placeholder="e.g. Python, Java, DSA"
            />
            <label>Preferred time</label>
            <input
              type="text"
              value={bookingPreference}
              onChange={(e)=>setBookingPreference(e.target.value)}
              placeholder="e.g. Weekends, 5 PM, Flexible"
            />
            <button onClick={fetchMatchingMentors}>
              Find Mentors
            </button>
            {matchingMentors.length>0 && (
              <>
                <h3>Available Mentors</h3>
                {matchingMentors.map((mentor)=>(
                  <div key={mentor.id} className="mentor-card">
                    <p><b>{mentor.name}</b></p>
                    <p>ID: {mentor.id}</p>
                    <p>Skills: {mentor.skills}</p>
                    <p>Teaching Points: {mentor.teachingPoints}</p>
                    <p>Teaching Sessions: {mentor.teachingSessions}</p>
                    <button
                      onClick={()=>setSelectedMentor(mentor.id)}
                    >
                      {selectedMentor===mentor.id ? "Selected" : "Select Mentor"}
                    </button>
                  </div>
                ))}
                <button onClick={submitBooking}>
                  Confirm Booking
                </button>
              </>
            )}
            <p>{message}</p>
            <button onClick={()=>setFeature("home")}>
              Back to Dashboard
            </button>
          </>
        ) : (
          <>
            <h2>{feature}</h2>
            <p>This feature will be implemented soon.</p>
            <button onClick={() => setFeature("home")}>Back to Dashboard</button>
          </>
          )}
          <button onClick={() => {
            setPage("login");
            setFeature("home");
            setLoginId("");
            setLoginPassword("");
          }}>Logout</button>
        </>
      ) : page === "login" ? (
        <>
          <form onSubmit={async (e) => {
            e.preventDefault();

            const response = await fetch("http://localhost:8080/api/login", {
              method: "POST",
              headers: {
                "Content-Type": "application/x-www-form-urlencoded"
              },
              body: new URLSearchParams({
                id: loginId,
                password: loginPassword,
              })
            });

            const result = await response.text();
            if(response.ok){
              setMessage("");
              setPage("dashboard");
            }else{
              setMessage(result);
            }
          }}>
            <h2>Login</h2>
            <input placeholder="User ID" value={loginId} onChange={(e) => setLoginId(e.target.value)} required />
            <input type="password" placeholder="Password" value={loginPassword} onChange={(e) => setLoginPassword(e.target.value)} required />
            <button type="submit">Login</button>
            <p>{message}</p>
          </form>
          <p>
            New here?{" "}
            <a href="#" onClick={(e) => {
              e.preventDefault();
              setMessage("");
              setPage("register");
            }}>
              Create an account
            </a>
          </p>
        </>
      ) : (
        <>
          <form onSubmit={async (e) => {
            e.preventDefault();

            const response = await fetch("http://localhost:8080/api/register", {
              method: "POST",
              headers: {
                "Content-Type": "application/x-www-form-urlencoded"
              },
              body: new URLSearchParams({
                id: id,
                name: name,
                password: password,
                skills: skills
              })
            });

            const result = await response.text();
            
            if(response.ok){
              setMessage("Registration successful! Please login.");
              setId("");
              setName("");
              setPhone("");
              setEmail("");
              setPassword("");
              setConfirmPassword("");
              setSkills("");

              setPage("login");
            } else {
              setMessage(result);
            }
          }}>
            <h2>Create Account</h2>
            <input placeholder="User ID" value={id} onChange={(e) => setId(e.target.value)} required />
            <input placeholder="Full Name" value={name} onChange={(e) => setName(e.target.value)} required />
            <input placeholder="Phone Number" value={phone} onChange={(e) => setPhone(e.target.value)} required />
            <input type="email" placeholder="Email" value={email} onChange={(e) => setEmail(e.target.value)} required />
            <input type="password" placeholder="Password" value={password} onChange={(e) => setPassword(e.target.value)} required />
            <input type="password" placeholder="Confirm Password" value={confirmPassword} onChange={(e) => setConfirmPassword(e.target.value)} required />
            <input placeholder="Skills (separate with semicolons)" value={skills} onChange={(e) => setSkills(e.target.value)} />
            <button type="submit">Register</button>
            <p>{message}</p>
          </form>
          <p>
            Already have an account?{" "}
            <a href="#" onClick={(e) => {
              e.preventDefault();
              setMessage("");
              setPage("login");
            }}>
              Login
            </a>
          </p>
        </>
      )}
    </div>
  );
}

export default App;
