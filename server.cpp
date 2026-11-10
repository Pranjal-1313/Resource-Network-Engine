// =====================================================
// RESOURCE NETWORK ENGINE - server.cpp
// A tiny web server written in C++.
//
// How it works:
//   1. You run this program. It waits for the browser.
//   2. The browser opens http://localhost:8080 and gets page.html.
//   3. When you click Analyze, the page sends your answers to /analyze.
//   4. This C++ code scores the ventures and sends the results back.
//
// Compile (Mac / Linux):  g++ -std=c++11 -o server server.cpp
// Compile (Windows):      g++ -std=c++11 -o server.exe server.cpp -lws2_32
// Run:                    ./server        (Windows: server.exe)
// Then open:              http://localhost:8080
// =====================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
using namespace std;

// ---------- Networking headers (different on Windows and Mac/Linux) ----------
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET socket_t;
  #define CLOSE_SOCKET closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  typedef int socket_t;
  #define CLOSE_SOCKET close
#endif


// =====================================================
// PART A: THE ENGINE (same logic as the JavaScript and console versions)
// =====================================================

struct Venture {
    string name;
    string about;
    double minAcres;
    vector<string> zones;
    double minBudget;
    vector<string> utilities;
    vector<string> contacts;
    vector<string> labour;
    int minPolitical;
    int minBusiness;
    bool subsidy;
};

struct User {
    double acres;
    string zone;
    double budget;
    int political;
    int business;
    vector<string> utilities;
    vector<string> contacts;
    vector<string> labour;
    bool subsidy;
};

struct Result {
    Venture venture;
    int score;
    vector<string> gaps;
};

map<string, string> zoneNames;
map<string, string> advice;
map<string, string> contactNames;
map<string, string> labourNames;
vector<Venture> ventures;

void setupNames() {
    zoneNames["highway"]  = "Highway/Industrial Corridor";
    zoneNames["city"]     = "City/Prime Urban Area";
    zoneNames["suburban"] = "Sub Urban/Outer Ring Road";
    zoneNames["village"]  = "Village/Rural Zone";
    zoneNames["coastal"]  = "Coastal/Port Access Area";
    zoneNames["vacant"]   = "Open Vacant Plot/Land";

    advice["power"]     = "Apply for a 3-phase power connection from the local electricity board (MPEB / DISCOM).";
    advice["water"]     = "Arrange a reliable water source (borewell, canal or municipal connection).";
    advice["road"]      = "Get road access widened or choose a plot with a wide approach road.";
    advice["fiber"]     = "Check for fiber internet providers in the area (Jio, Airtel, BSNL) and request a connection.";
    advice["legal"]     = "Find a lawyer or consultant who knows land-use, licensing and company registration.";
    advice["finance"]   = "Build contacts with bank managers, NBFCs or investors for loans and funding.";
    advice["mentors"]   = "Find a mentor or an experienced person in this industry (try local industry associations).";
    advice["unskilled"] = "Tie up with a local labour contractor for daily-wage workers.";
    advice["skilled"]   = "Hire or partner with skilled workers such as technicians, drivers or sales staff.";
    advice["agri"]      = "Connect with agriculture experts, a local KVK (Krishi Vigyan Kendra) or farmer groups.";
    advice["tech"]      = "Hire or partner with engineers or technical managers.";

    contactNames["legal"]   = "Legal/Regulatory advisor";
    contactNames["finance"] = "Financial contact";
    contactNames["mentors"] = "Mentor/Tech executive";

    labourNames["unskilled"] = "unskilled labour";
    labourNames["skilled"]   = "skilled labour";
    labourNames["agri"]      = "agri-specialists";
    labourNames["tech"]      = "technical/engineering talent";
}

void addVenture(string name, string about, double minAcres, vector<string> zones,
                double minBudget, vector<string> utilities, vector<string> contacts,
                vector<string> labour, int minPolitical, int minBusiness, bool subsidy) {
    Venture v;
    v.name = name;
    v.about = about;
    v.minAcres = minAcres;
    v.zones = zones;
    v.minBudget = minBudget;
    v.utilities = utilities;
    v.contacts = contacts;
    v.labour = labour;
    v.minPolitical = minPolitical;
    v.minBusiness = minBusiness;
    v.subsidy = subsidy;
    ventures.push_back(v);
}

void setupVentures() {
    addVenture("Warehouse & Logistics Hub",
               "Storage and distribution space for traders, e-commerce and manufacturers.",
               2, {"highway", "suburban", "coastal"}, 50,
               {"power", "road"}, {"legal"}, {"unskilled", "skilled"}, 1, 1, false);

    addVenture("Cold Storage",
               "Temperature-controlled storage for fruits, vegetables and dairy.",
               1, {"highway", "suburban", "village"}, 150,
               {"power", "water", "road"}, {"finance"}, {"skilled", "agri"}, 1, 1, true);

    addVenture("Agro-Processing Unit",
               "Process local crops into flour, oil, pulses or packaged food.",
               1, {"village", "highway", "suburban"}, 100,
               {"power", "water"}, {"legal", "finance"}, {"agri", "unskilled"}, 0, 1, true);

    addVenture("Solar Power Plant",
               "Generate solar power and sell it to the grid or to industries.",
               5, {"vacant", "village", "highway"}, 300,
               {"road"}, {"legal", "finance"}, {"tech"}, 1, 0, true);

    addVenture("Organic Farm & Agri-Tourism",
               "Organic farming with farm stays, direct sales and visitor experiences.",
               3, {"village", "suburban"}, 30,
               {"water", "road"}, {"mentors"}, {"agri", "unskilled"}, 0, 0, true);

    addVenture("Commercial Complex / Retail Mall",
               "Shops and offices on prime land, earning rent.",
               0.5, {"city"}, 500,
               {"power", "road", "water"}, {"legal", "finance"}, {"skilled"}, 2, 1, false);

    addVenture("Co-working Space / IT Hub",
               "Shared offices and small IT companies, needs good internet and talent.",
               0.25, {"city", "suburban"}, 200,
               {"power", "fiber"}, {"mentors", "finance"}, {"tech"}, 0, 1, false);

    addVenture("EV Charging Station",
               "Fast-charging stations for electric cars, bikes and trucks.",
               0.25, {"highway", "city", "suburban"}, 40,
               {"power", "road"}, {"legal"}, {"tech"}, 1, 0, true);

    addVenture("Aquaculture / Fish Farming",
               "Fish or shrimp farming ponds, supplying local and city markets.",
               2, {"coastal", "village"}, 40,
               {"water", "power"}, {"mentors"}, {"agri", "unskilled"}, 0, 1, true);
}

bool has(const vector<string>& list, const string& item) {
    return find(list.begin(), list.end(), item) != list.end();
}

double nice(double n) {
    return floor(n * 10 + 0.5) / 10;
}

string numText(double n) {
    ostringstream out;
    out << nice(n);
    return out.str();
}

// Score ONE venture (out of 100) and collect the gaps
Result scoreVenture(const Venture& v, const User& user) {
    double score = 0;
    vector<string> gaps;

    // Zone (25)
    if (has(v.zones, user.zone)) {
        score += 25;
    } else {
        string goodZones = "";
        for (size_t z = 0; z < v.zones.size(); z++) {
            if (z > 0) goodZones += ", ";
            goodZones += zoneNames[v.zones[z]];
        }
        gaps.push_back("Location: this works best in: " + goodZones +
                       ". Consider leasing or buying land there, or partnering with someone who owns it.");
    }

    // Land (15)
    if (user.acres >= v.minAcres) {
        score += 15;
    } else {
        score += 15 * (user.acres / v.minAcres);
        gaps.push_back("Land: you need about " + numText(v.minAcres) + " acres but have " + numText(user.acres) +
                       ". Lease or buy about " + numText(v.minAcres - user.acres) + " more acres.");
    }

    // Budget (20)
    if (user.budget >= v.minBudget) {
        score += 20;
    } else {
        score += 20 * (user.budget / v.minBudget);
        gaps.push_back("Capital: you need about Rs " + numText(v.minBudget) + " lakh but have Rs " +
                       numText(user.budget) + " lakh. Shortfall is Rs " + numText(v.minBudget - user.budget) +
                       " lakh. Consider a bank loan, a partner or an investor.");
    }

    // Utilities (15)
    double utilityPoints = 15.0 / v.utilities.size();
    for (size_t u = 0; u < v.utilities.size(); u++) {
        string needed = v.utilities[u];
        if (has(user.utilities, needed)) {
            score += utilityPoints;
        } else {
            gaps.push_back("Site: missing " + needed + ". " + advice[needed]);
        }
    }

    // Network (15 = 5 political + 5 business + 5 contacts)
    if (user.political >= v.minPolitical) {
        score += 5;
    } else {
        score += 5.0 * user.political / v.minPolitical;
        gaps.push_back("Political/administrative access: you need a stronger link to government offices for licences and approvals. "
                       "Build contacts with the local municipal office, the District Industries Centre (DIC) or industry associations.");
    }

    if (user.business >= v.minBusiness) {
        score += 5;
    } else {
        score += 5.0 * user.business / v.minBusiness;
        gaps.push_back("Business network: you need better supplier/buyer connections. "
                       "Meet local traders, wholesalers and distributors, or join a trade association.");
    }

    double contactPoints = 5.0 / v.contacts.size();
    for (size_t c = 0; c < v.contacts.size(); c++) {
        string neededContact = v.contacts[c];
        if (has(user.contacts, neededContact)) {
            score += contactPoints;
        } else {
            gaps.push_back("Contact: you need a " + contactNames[neededContact] + ". " + advice[neededContact]);
        }
    }

    // Labour (10)
    double labourPoints = 10.0 / v.labour.size();
    for (size_t l = 0; l < v.labour.size(); l++) {
        string neededLabour = v.labour[l];
        if (has(user.labour, neededLabour)) {
            score += labourPoints;
        } else {
            gaps.push_back("People: you need " + labourNames[neededLabour] + ". " + advice[neededLabour]);
        }
    }

    // Subsidy bonus
    if (v.subsidy) {
        if (user.subsidy) {
            score += 5;
        } else {
            gaps.push_back("Subsidy: government schemes exist for this business. "
                           "Check eligibility with the District Industries Centre or the relevant ministry.");
        }
    }

    if (score > 100) {
        score = 100;
    }

    Result r;
    r.venture = v;
    r.score = (int)floor(score + 0.5);
    r.gaps = gaps;
    return r;
}

bool higherScoreFirst(const Result& a, const Result& b) {
    return a.score > b.score;
}


// =====================================================
// PART B: TURNING RESULTS INTO HTML (what the browser shows)
// =====================================================

// Make text safe for HTML (for example & becomes &amp;)
string htmlSafe(const string& text) {
    string out = "";
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '&')      out += "&amp;";
        else if (text[i] == '<') out += "&lt;";
        else if (text[i] == '>') out += "&gt;";
        else                     out += text[i];
    }
    return out;
}

string makeResultsHtml(const vector<Result>& results) {
    string html = "<h2 align='center'>Top Recommended Ventures</h2>";

    size_t howMany = 3;
    if (results.size() < howMany) {
        howMany = results.size();
    }

    for (size_t i = 0; i < howMany; i++) {
        const Result& r = results[i];

        string label = "Weak fit";
        string colour = "weak";
        if (r.score >= 75) {
            label = "Strong fit";
            colour = "good";
        } else if (r.score >= 50) {
            label = "Possible fit, with some gaps";
            colour = "ok";
        }

        html += "<div class='card'>";
        html += "<h3>#" + to_string(i + 1) + " " + htmlSafe(r.venture.name) + " &mdash; " +
                to_string(r.score) + "/100 (" + label + ")</h3>";
        html += "<div class='bar-outer'><div class='bar-inner " + colour + "' style='width:" +
                to_string(r.score) + "%'></div></div>";
        html += "<p>" + htmlSafe(r.venture.about) + "</p>";

        if (r.gaps.empty()) {
            html += "<p><b>No gaps found. You have everything this venture needs!</b></p>";
        } else {
            html += "<p><b>Gap Analysis &ndash; what to add or acquire:</b></p><ul>";
            for (size_t g = 0; g < r.gaps.size(); g++) {
                html += "<li>" + htmlSafe(r.gaps[g]) + "</li>";
            }
            html += "</ul>";
        }
        html += "</div>";
    }

    if (results.size() > howMany) {
        html += "<div class='card'><b>Other ventures checked:</b><ul>";
        for (size_t j = howMany; j < results.size(); j++) {
            html += "<li>" + htmlSafe(results[j].venture.name) + " &mdash; " + to_string(results[j].score) + "/100</li>";
        }
        html += "</ul></div>";
    }

    return html;
}


// =====================================================
// PART C: READING THE BROWSER'S REQUEST
// =====================================================

// Turn %20 into a space, + into a space, and so on
string urlDecode(const string& text) {
    string out = "";
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '+') {
            out += ' ';
        } else if (text[i] == '%' && i + 2 < text.size()) {
            string hex = text.substr(i + 1, 2);
            out += (char)strtol(hex.c_str(), NULL, 16);
            i += 2;
        } else {
            out += text[i];
        }
    }
    return out;
}

// "acres=5&zone=highway" -> { "acres": "5", "zone": "highway" }
map<string, string> parseQuery(const string& query) {
    map<string, string> values;
    size_t start = 0;
    while (start <= query.size()) {
        size_t end = query.find('&', start);
        if (end == string::npos) {
            end = query.size();
        }
        string pair = query.substr(start, end - start);
        size_t eq = pair.find('=');
        if (eq != string::npos) {
            values[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
        }
        start = end + 1;
    }
    return values;
}

// Read a number from the values; if missing or wrong, use 0. Never below 0.
double getNumber(map<string, string>& values, const string& key) {
    if (values.count(key) == 0) {
        return 0;
    }
    double n = atof(values[key].c_str());
    if (n < 0) {
        return 0;
    }
    return n;
}

// Checkbox: the page sends 1 when it is ticked
bool isOn(map<string, string>& values, const string& key) {
    return values.count(key) > 0 && values[key] == "1";
}

// Build a User from the values the browser sent
User makeUser(map<string, string>& values) {
    User user;
    user.acres = getNumber(values, "acres");
    user.budget = getNumber(values, "budget");
    user.zone = values.count("zone") ? values["zone"] : "";
    user.political = (int)getNumber(values, "political");
    user.business = (int)getNumber(values, "business");
    user.subsidy = isOn(values, "subsidy");

    string utilityIds[4] = {"power", "water", "road", "fiber"};
    for (int i = 0; i < 4; i++) {
        if (isOn(values, utilityIds[i])) user.utilities.push_back(utilityIds[i]);
    }
    string contactIds[3] = {"legal", "finance", "mentors"};
    for (int i = 0; i < 3; i++) {
        if (isOn(values, contactIds[i])) user.contacts.push_back(contactIds[i]);
    }
    string labourIds[4] = {"unskilled", "skilled", "agri", "tech"};
    for (int i = 0; i < 4; i++) {
        if (isOn(values, labourIds[i])) user.labour.push_back(labourIds[i]);
    }
    return user;
}

// The full job: query text in, results HTML out
string analyze(const string& query) {
    map<string, string> values = parseQuery(query);
    User user = makeUser(values);

    if (zoneNames.count(user.zone) == 0) {
        return "<p><b>Please select a Geographical zone / Area type first.</b></p>";
    }

    vector<Result> results;
    for (size_t i = 0; i < ventures.size(); i++) {
        results.push_back(scoreVenture(ventures[i], user));
    }
    stable_sort(results.begin(), results.end(), higherScoreFirst);

    return makeResultsHtml(results);
}


// =====================================================
// PART D: THE WEB SERVER
// =====================================================

// Read a whole file into a string. Returns false if it can't be opened.
bool readFile(const string& filename, string& content) {
    ifstream file(filename.c_str(), ios::binary);
    if (!file) {
        return false;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    return true;
}

// Send a full HTTP answer back to the browser
void sendResponse(socket_t client, const string& status, const string& type, const string& body) {
    string answer = "HTTP/1.1 " + status + "\r\n";
    answer += "Content-Type: " + type + "; charset=utf-8\r\n";
    answer += "Content-Length: " + to_string(body.size()) + "\r\n";
    answer += "Connection: close\r\n\r\n";
    answer += body;
    send(client, answer.c_str(), (int)answer.size(), 0);
}

// Handle ONE request from the browser
void handleClient(socket_t client) {
    char buffer[8192];
    int got = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (got <= 0) {
        return;
    }
    buffer[got] = 0;
    string request(buffer);

    // First line looks like:  GET /analyze?acres=5&zone=highway HTTP/1.1
    size_t lineEnd = request.find("\r\n");
    string firstLine = request.substr(0, lineEnd);
    size_t space1 = firstLine.find(' ');
    size_t space2 = firstLine.find(' ', space1 + 1);
    if (space1 == string::npos || space2 == string::npos) {
        sendResponse(client, "400 Bad Request", "text/plain", "Bad request");
        return;
    }
    string method = firstLine.substr(0, space1);
    string target = firstLine.substr(space1 + 1, space2 - space1 - 1);

    // Split the address into the path and the query
    string path = target;
    string query = "";
    size_t questionMark = target.find('?');
    if (questionMark != string::npos) {
        path = target.substr(0, questionMark);
        query = target.substr(questionMark + 1);
    }

    cout << method << " " << path << "\n";

    if (method != "GET") {
        sendResponse(client, "405 Method Not Allowed", "text/plain", "Only GET is supported");
    } else if (path == "/" || path == "/index.html") {
        string page;
        if (readFile("page.html", page)) {
            sendResponse(client, "200 OK", "text/html", page);
        } else {
            sendResponse(client, "500 Server Error", "text/plain",
                         "page.html not found. Put page.html in the same folder you run the server from.");
        }
    } else if (path == "/analyze") {
        sendResponse(client, "200 OK", "text/html", analyze(query));
    } else {
        sendResponse(client, "404 Not Found", "text/plain", "Not found");
    }
}

int main() {
    setupNames();
    setupVentures();

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Could not start Windows networking.\n";
        return 1;
    }
#endif

    int port = 8080;

    socket_t server = socket(AF_INET, SOCK_STREAM, 0);

#ifndef _WIN32
    // Lets you restart the program quickly without "address in use" errors
    int yes = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#endif

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // only this computer can connect
    address.sin_port = htons(port);
    for (size_t i = 0; i < sizeof(address.sin_zero); i++) address.sin_zero[i] = 0;

    if (bind(server, (sockaddr*)&address, sizeof(address)) != 0) {
        cout << "Could not use port " << port << ". Is the server already running?\n";
        return 1;
    }
    listen(server, 10);

    cout << "========================================\n";
    cout << " Resource Network Engine is running!\n";
    cout << " Open this in your browser:\n";
    cout << "   http://localhost:" << port << "\n";
    cout << " Press Ctrl+C here to stop.\n";
    cout << "========================================\n";

    while (true) {
        socket_t client = accept(server, NULL, NULL);
        handleClient(client);
        CLOSE_SOCKET(client);
    }

    return 0;
}
