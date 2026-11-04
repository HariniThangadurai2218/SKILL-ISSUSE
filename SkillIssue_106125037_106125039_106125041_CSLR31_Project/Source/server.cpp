/**
 * server.cpp - HTTP entry point. Exposes Platform functions as REST endpoints on port 8080 for the React frontend (uses third-party cpp-httplib, include/httplib.h).
 */

#include "httplib.h"
#include "platform.h"
#include <cstdio>
#include <iostream>

using namespace std;

// Escapes a string so it is safe inside a JSON "..." value (quotes, backslashes
// and control characters such as \r or \n would otherwise break the response).
static string jsonEscape(const string& in){
    string out;
    for(unsigned char c:in){
        switch(c){
            case '"':  out+="\\\""; break;
            case '\\': out+="\\\\"; break;
            case '\n': out+="\\n"; break;
            case '\r': out+="\\r"; break;
            case '\t': out+="\\t"; break;
            default:
                if(c<0x20){
                    char buf[8];
                    snprintf(buf,sizeof(buf),"\\u%04x",c);
                    out+=buf;
                }
                else out+=static_cast<char>(c);
        }
    }
    return out;
}

int main(){
    Platform app;
    httplib::Server svr;

    // Allow requests from the React frontend
    svr.set_default_headers({
        {"Access-Control-Allow-Origin","http://localhost:5173"}
    });

    // Test route
    svr.Get("/",[](const httplib::Request&,httplib::Response& res){
        res.set_content("Skill Issue API is running!","text/plain");
    });

    // Registration route
    svr.Post("/api/register",[&app](const httplib::Request& req,httplib::Response& res){
        string message;

        bool success=app.registerUserAPI(
            req.get_param_value("id"),
            req.get_param_value("name"),
            req.get_param_value("password"),
            req.get_param_value("skills"),
            message
        );

        res.status=success?201:400;
        res.set_content(message,"text/plain");
    });

    // Login route
    svr.Post("/api/login",[&app](const httplib::Request& req,httplib::Response& res){
        string message;

        bool success=app.loginAPI(
            req.get_param_value("id"),
            req.get_param_value("password"),
            message
        );

        res.status=success?200:401;
        res.set_content(message,"text/plain");
    });

    svr.Post("/api/profile",[&app](const httplib::Request& req,httplib::Response& res){
        User profile;

        bool success=app.getProfileAPI(
            req.get_param_value("id"),
            profile
        );
        if(!success){
            res.status=404;
            res.set_content("User not found","text/plain");
            return;
        }
        string response = "{";
        response += "\"id\":\""+jsonEscape(profile.id)+"\",";
        response += "\"name\":\""+jsonEscape(profile.name)+"\",";
        response += "\"skills\":\""+jsonEscape(profile.skills)+"\",";
        response += "\"totalPoints\":" + to_string(profile.totalPoints) + ",";
        response += "\"teachingPoints\":" + to_string(profile.teachingPoints) + ",";
        response += "\"teachingSessions\":" + to_string(profile.teachingSessions) + ",";
        response += "\"learningSessions\":" + to_string(profile.learningSessions);
        response += "}";

        res.set_content(response, "application/json");
    });

    svr.Get("/api/leaderboard",[&app](const httplib::Request&,httplib::Response& res){
        vector<User> ranked=app.getLeaderboardAPI();

        string response="[";

        for(size_t i=0;i<ranked.size();i++){
            if(i>0) response+=",";

            response+="{";
            response+="\"rank\":"+to_string(i+1)+",";
            response+="\"id\":\""+jsonEscape(ranked[i].id)+"\",";
            response+="\"name\":\""+jsonEscape(ranked[i].name)+"\",";
            response+="\"teachingPoints\":"+to_string(ranked[i].teachingPoints)+",";
            response+="\"teachingSessions\":"+to_string(ranked[i].teachingSessions);
            response+="}";
        }

        response+="]";

        res.set_content(response,"application/json");
    });

    // Top-k mentors (heap-based). Example: /api/top-mentors?k=3
    svr.Get("/api/top-mentors",[&app](const httplib::Request& req,httplib::Response& res){
        int k=3;
        if(req.has_param("k")){
            try{ k=stoi(req.get_param_value("k")); }catch(...){ k=3; }
        }
        vector<User> top=app.getTopMentorsAPI(k);

        string response="[";
        for(size_t i=0;i<top.size();i++){
            if(i>0) response+=",";
            response+="{";
            response+="\"rank\":"+to_string(i+1)+",";
            response+="\"id\":\""+jsonEscape(top[i].id)+"\",";
            response+="\"name\":\""+jsonEscape(top[i].name)+"\",";
            response+="\"teachingPoints\":"+to_string(top[i].teachingPoints);
            response+="}";
        }
        response+="]";
        res.set_content(response,"application/json");
    });

    svr.Get("/api/sessions",[&app](const httplib::Request& req,httplib::Response& res){
        string id=req.get_param_value("id");
        string type=req.get_param_value("type");

        if(type!="teaching" && type!="learning"){
            res.status=400;
            res.set_content("Invalid session type","text/plain");
            return;
        }
        vector<Session> sessions=app.getMySessionsAPI(id,type=="teaching");

        string response="[";

        for(size_t i=0;i<sessions.size();i++){
            if(i>0) response+=",";

            const Session& s=sessions[i];

            response+="{";
            response+="\"id\":"+to_string(s.id)+",";
            response+="\"skill\":\""+jsonEscape(s.skill)+"\",";
            response+="\"learnerId\":\""+jsonEscape(s.learnerId)+"\",";
            response+="\"mentorId\":\""+jsonEscape(s.mentorId)+"\",";
            response+="\"scheduledTime\":\""+jsonEscape(s.scheduledTime)+"\",";
            response+="\"status\":\""+jsonEscape(s.status)+"\"";
            response+="}";
        }

        response+="]";

        res.set_content(response,"application/json");
    });

    svr.Post("/api/skills",[&app](const httplib::Request& req,httplib::Response& res){
        string message;

        bool success=app.updateSkillsAPI(
            req.get_param_value("id"),
            req.get_param_value("skills"),
            message
        );

        res.status=success?200:404;
        res.set_content(message,"text/plain");
    });

    svr.Get("/api/mentor-inbox",[&app](const httplib::Request& req,httplib::Response& res){
        string id=req.get_param_value("id");

        vector<BookingRequest> requests=app.getMentorInboxAPI(id);

        string json="[";

        for(size_t i=0;i<requests.size();i++){
            const auto& r=requests[i];

            if(i>0) json+=",";

            json+="{";
            json+="\"id\":"+to_string(r.id)+",";
            json+="\"skill\":\""+jsonEscape(r.skill)+"\",";
            json+="\"learnerId\":\""+jsonEscape(r.learnerId)+"\",";
            json+="\"preference\":\""+jsonEscape(r.preference)+"\",";
            json+="\"status\":\""+jsonEscape(r.status)+"\"";
            json+="}";
        }

        json+="]";

        res.set_content(json,"application/json");
    });

    svr.Post("/api/mentor-response",[&app](const httplib::Request& req,httplib::Response& res){
        string message;

        bool success=app.respondToMentorRequestAPI(
            req.get_param_value("mentorId"),
            stoi(req.get_param_value("requestId")),
            req.get_param_value("action"),
            req.get_param_value("proposedTime"),
            message
        );

        res.status=success?200:400;
        res.set_content(message,"text/plain");
    });

    svr.Get("/api/my-bookings",[&app](const httplib::Request& req,httplib::Response& res){
        string id=req.get_param_value("id");

        vector<BookingDetails> bookings=app.getMyBookingsAPI(id);

        string json="[";

        for(size_t i=0;i<bookings.size();i++){
            const auto& b=bookings[i];
            const auto& r=b.request;

            if(i>0) json+=",";

            json+="{";
            json+="\"id\":"+to_string(r.id)+",";
            json+="\"skill\":\""+jsonEscape(r.skill)+"\",";
            json+="\"preference\":\""+jsonEscape(r.preference)+"\",";
            json+="\"status\":\""+jsonEscape(r.status)+"\",";
            json+="\"mentorId\":\""+jsonEscape(r.confirmedMentorId)+"\",";
            json+="\"confirmedTime\":\""+jsonEscape(r.confirmedTime)+"\",";

            json+="\"offers\":[";

            for(size_t j=0;j<b.offers.size();j++){
                const auto& o=b.offers[j];

                if(j>0) json+=",";

                json+="{";
                json+="\"mentorId\":\""+jsonEscape(o.mentorId)+"\",";
                json+="\"proposedTime\":\""+jsonEscape(o.proposedTime)+"\"";
                json+="}";
            }

            json+="]";
            json+="}";
        }

        json+="]";

        res.set_content(json,"application/json");
    });

    svr.Post("/api/grade-quiz",[&app](const httplib::Request& req,httplib::Response& res){
        string mentorId=req.get_param_value("mentorId");
        int sessionId=stoi(req.get_param_value("sessionId"));
        int score=stoi(req.get_param_value("score"));

        string message;

        bool success=app.gradeQuizAPI(
            mentorId,
            sessionId,
            score,
            message
        );

        res.status=success?200:400;
        res.set_content(message,"text/plain");
    });


    svr.Post("/api/session-outcome",[&app](const httplib::Request& req,httplib::Response& res){
        string mentorId=req.get_param_value("mentorId");
        int sessionId=stoi(req.get_param_value("sessionId"));
        string outcome=req.get_param_value("outcome");

        string message;

        bool success=app.reportOutcomeAPI(
            mentorId,
            sessionId,
            outcome,
            message
        );
        res.status=success?200:400;
        res.set_content(message,"text/plain");
    });

    svr.Post("/api/rate-session",[&app](const httplib::Request& req,httplib::Response& res){
        string learnerId=req.get_param_value("learnerId");
        int sessionId=stoi(req.get_param_value("sessionId"));
        int rating=stoi(req.get_param_value("rating"));

        string message;

        bool success=app.rateSessionAPI(
            learnerId,
            sessionId,
            rating,
            message
        );

        res.status=success?200:400;
        res.set_content(message,"text/plain");
    });

    svr.Post("/api/accept-offer",[&app](const httplib::Request& req,httplib::Response& res){
        string learnerId=req.get_param_value("learnerId");
        int requestId=stoi(req.get_param_value("requestId"));
        string mentorId=req.get_param_value("mentorId");

        string message;

        bool success=app.acceptBookingOfferAPI(
            learnerId,
            requestId,
            mentorId,
            message
        );

        res.status=success?200:400;
        res.set_content(message,"text/plain");
    });

    svr.Get("/api/matching-mentors", [&](const httplib::Request& req, httplib::Response& res) {
        string skill=req.get_param_value("skill");
        string learnerId=req.get_param_value("learnerId");

        vector<User> mentors=app.getMatchingMentorsAPI(skill,learnerId);

        string json="[";

        for(size_t i=0;i<mentors.size();i++){
            if(i>0) json+=",";

            const User& u=mentors[i];

            json+="{";
            json+="\"id\":\""+jsonEscape(u.id)+"\",";
            json+="\"name\":\""+jsonEscape(u.name)+"\",";
            json+="\"skills\":\""+jsonEscape(u.skills)+"\",";
            json+="\"teachingPoints\":"+to_string(u.teachingPoints)+",";
            json+="\"teachingSessions\":"+to_string(u.teachingSessions);
            json+="}";
        }

        json+="]";
        res.set_content(json,"application/json");
    });

    svr.Post("/api/book-session", [&](const httplib::Request& req, httplib::Response& res) {
        string learnerId=req.get_param_value("learnerId");
        string skill=req.get_param_value("skill");
        string mentorId=req.get_param_value("mentorId");
        string preference=req.get_param_value("preference");

        string message;

        bool success=app.bookSessionAPI(
            learnerId,skill,mentorId,preference,message
        );

        res.status=success?200:400;
        res.set_content(message,"text/plain");
    });

    cout<<"Server running at http://localhost:8080\n";
    svr.listen("0.0.0.0",8080);

    return 0;
}
