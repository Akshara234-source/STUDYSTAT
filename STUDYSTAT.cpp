//You need to install the SDL library to run this code 
//try to install the latest version of it 

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <cmath>
#include <thread>
#include <atomic>
#include <filesystem>
using namespace std;
//For file functions
namespace fs = filesystem;
//For audio Fuctions
atomic<bool> breakActive(false);
thread musicThread;
Mix_Music* backgroundMusic = nullptr;

//-----------TO SHOW TIME,DATE AND DURATION------------------
namespace Utils {
    string formatDate(time_t timestamp) {
        struct tm* timeInfo = localtime(&timestamp);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeInfo);
        return string(buffer);
    }
   
    string formatDateTime(time_t timestamp) {
        struct tm* timeInfo = localtime(&timestamp);
        char buffer[25];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);
        return string(buffer);
    }
   
    string formatDuration(int seconds) {
        if (seconds < 60) return to_string(seconds) + "s";
        else if (seconds < 3600) return to_string(seconds / 60) + "m";
        else return to_string(seconds / 3600) + "h " + to_string((seconds % 3600) / 60) + "m";
    }
   
    time_t parseDateTime(const string& dateTimeStr) {
        struct tm timeInfo = {0};
        if (dateTimeStr.length() >= 10) {
            if (sscanf(dateTimeStr.c_str(), "%d-%d-%d",
                   &timeInfo.tm_year, &timeInfo.tm_mon, &timeInfo.tm_mday) == 3) {
                timeInfo.tm_year -= 1900;
                timeInfo.tm_mon -= 1;
                return mktime(&timeInfo);
            }
        }
        return 0;
    }
}

// ---------------CLASSES-------------------------------
//-------------------STUDYSESSION CLASS------------------
class StudySession {
private:
    int id;
    string subject;
    time_t startTime;
    time_t endTime;
    int duration;
    string notes;
    int breakTime;
    string attachedImagePath;
    vector<string> attachedFiles;

public:
    StudySession(int id, const string& subject, time_t startTime, time_t endTime, const string& notes = "", int breakTime = 0)
        : id(id), subject(subject), startTime(startTime), endTime(endTime), notes(notes), breakTime(breakTime) {
        duration = difftime(endTime, startTime) - breakTime;
    }
   
    int getBreakTime() const { return breakTime; }
    void setBreakTime(int time) {
        breakTime = time;
        //---------TO CALCULATE FINAL DURATION AFTER BREAKS---------------
        duration = difftime(endTime, startTime) - breakTime;
    }
   
    //
    int getId() const { return id; }
    string getSubject() const { return subject; }
    time_t getStartTime() const { return startTime; }
    time_t getEndTime() const { return endTime; }
    int getDuration() const { return duration; }
    string getNotes() const { return notes; }
   
    //-------FOR IMAGE HANDLING-----------
    void attachImage(const string& path) {
        attachedImagePath = path;
    }
    string getAttachedImage() const {
        return attachedImagePath;
    }

    
//-----------FOR HANDLING THE FILES-------
    void attachFile(const string& path) {
        attachedFiles.push_back(path);
    }
    vector<string> getAttachedFiles() const {
        return attachedFiles;
    }

    string toString() const {
        stringstream ss;
        ss << "Session #" << id << ": " << subject << " | "
           << "Start: " << Utils::formatDateTime(startTime) << " | "
           << "End: " << Utils::formatDateTime(endTime) << " | "
           << "Duration: " << Utils::formatDuration(duration);
        if (breakTime > 0) {
            ss << " | Break time: " << Utils::formatDuration(breakTime);
        }
        if (!notes.empty()) ss << " | Notes: " << notes;
       
       
        if (!attachedImagePath.empty()) ss << " | [Has Image]";
        if (!attachedFiles.empty()) ss << " | [Has " << attachedFiles.size() << " Files]";
       
        return ss.str();
    }
   
    string serialize() const {
        stringstream ss;
        ss << id << "|" << subject << "|" << startTime << "|" << endTime << "|" << notes << "|" << breakTime;
        ss << "|" << attachedImagePath;
       
        // Serialize attached files
        ss << "|";
        for (size_t i = 0; i < attachedFiles.size(); i++) {
            if (i > 0) ss << ",";
            ss << attachedFiles[i];
        }
       
        return ss.str();
    }
   
    static StudySession deserialize(const string& data) {
        stringstream ss(data);
        string item;
        vector<string> parts;
       
        while (getline(ss, item, '|')) parts.push_back(item);
       
        if (parts.size() >= 4) {
            int breakTime = (parts.size() > 5) ? stoi(parts[5]) : 0;
            StudySession session(stoi(parts[0]), parts[1], stoll(parts[2]),
                             stoll(parts[3]), parts.size() > 4 ? parts[4] : "", breakTime);
           
            // Load attached image if available
            if (parts.size() > 6 && !parts[6].empty()) {
                session.attachImage(parts[6]);
            }
           
            // Load attached files if available
            if (parts.size() > 7 && !parts[7].empty()) {
                stringstream filesSS(parts[7]);
                string file;
                while (getline(filesSS, file, ',')) {
                    session.attachFile(file);
                }
            }
           
            return session;
        }
        return StudySession(0, "Unknown", 0, 0);
    }
};

// -------------------- TODOITEM CLASS ------------------------------
class TodoItem {
private:
    int id;
    string description;
    bool completed;
    int priority;
    time_t dueDate;
    string subject;

public:
    TodoItem(int id, const string& desc, int priority = 2,
           time_t dueDate = 0, const string& subject = "", bool completed = false)
        : id(id), description(desc), completed(completed),
          priority(priority), dueDate(dueDate), subject(subject) {}
   
    int getId() const { return id; }
    string getDescription() const { return description; }
    bool isCompleted() const { return completed; }
    int getPriority() const { return priority; }
    time_t getDueDate() const { return dueDate; }
    string getSubject() const { return subject; }
    void setCompleted(bool status) { completed = status; }
   
    string getPriorityString() const {
        switch (priority) {
            case 1: return "High";
            case 2: return "Medium";
            case 3: return "Low";
            default: return "Unknown";
        }
    }
   
    string toString() const {
        stringstream ss;
        ss << "[" << (completed ? "X" : " ") << "] " << description;
        if (!subject.empty()) ss << " (" << subject << ")";
        ss << " - Priority: " << getPriorityString();
        if (dueDate > 0) ss << ", Due: " << Utils::formatDate(dueDate);
        return ss.str();
    }
   
    string serialize() const {
        stringstream ss;
        ss << id << "|" << description << "|" << (completed ? "1" : "0") << "|"
           << priority << "|" << dueDate << "|" << subject;
        return ss.str();
    }
   
    static TodoItem deserialize(const string& data) {
        stringstream ss(data);
        string item;
        vector<string> parts;
       
        while (getline(ss, item, '|')) parts.push_back(item);
       
        if (parts.size() >= 5) {
            return TodoItem(stoi(parts[0]), parts[1], stoi(parts[3]),
                          stoll(parts[4]), parts.size() > 5 ? parts[5] : "", parts[2] == "1");
        }
        return TodoItem(0, "Unknown task");
    }
};

// ==================== TODOLIST CLASS ====================
class TodoList {
private:
    vector<TodoItem> items;
    int nextId;

public:
    TodoList() : nextId(1) {}
   
    void addItem(const string& description, int priority = 2,
                time_t dueDate = 0, const string& subject = "") {
        items.push_back(TodoItem(nextId++, description, priority, dueDate, subject));
    }
   
    bool removeItem(int id) {
        auto it = find_if(items.begin(), items.end(),
                             [id](const TodoItem& item) { return item.getId() == id; });
        if (it != items.end()) {
            items.erase(it);
            return true;
        }
        return false;
    }
   
    bool markAsCompleted(int id, bool status = true) {
        for (auto& item : items) {
            if (item.getId() == id) {
                item.setCompleted(status);
                return true;
            }
        }
        return false;
    }
   
    const vector<TodoItem>& getAllItems() const { return items; }
   
    vector<TodoItem> getIncompleteItems() const {
        vector<TodoItem> result;
        for (const auto& item : items)
            if (!item.isCompleted()) result.push_back(item);
        return result;
    }
   
    vector<TodoItem> getItemsSortedByPriority() const {
        vector<TodoItem> result = items;
        sort(result.begin(), result.end(),
                [](const TodoItem& a, const TodoItem& b) {
                    return a.getPriority() < b.getPriority();
                });
        return result;
    }
   
    vector<TodoItem> getItemsSortedByDueDate() const {
        vector<TodoItem> result = items;
        sort(result.begin(), result.end(),
                [](const TodoItem& a, const TodoItem& b) {
                    if (a.getDueDate() == 0) return false;
                    if (b.getDueDate() == 0) return true;
                    return a.getDueDate() < b.getDueDate();
                });
        return result;
    }
   
    bool saveToFile(const string& filename) const {
        ofstream file(filename);
        if (!file.is_open()) return false;
        for (const auto& item : items) file << item.serialize() << endl;
        return true;
    }
   
    bool loadFromFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) return false;
       
        items.clear();
        nextId = 1;
       
        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                TodoItem item = TodoItem::deserialize(line);
                items.push_back(item);
                if (item.getId() >= nextId) nextId = item.getId() + 1;
            }
        }
        return true;
    }
};

// ==================== USER CLASS ====================
class User {
private:
    int id;
    string username;
    string password;
    string fullName;
    vector<StudySession> sessions;
    TodoList todoList;
    int nextSessionId;

public:
    User(int id, const string& username, const string& password, const string& fullName = "")
        : id(id), username(username), password(password), fullName(fullName), nextSessionId(1) {}
   
    int getId() const { return id; }
    string getUsername() const { return username; }
    string getFullName() const { return fullName; }
    bool verifyPassword(const string& pwd) const { return password == pwd; }
   
    int startSession(const string& subject, time_t startTime, const string& notes = "") {
        sessions.push_back(StudySession(nextSessionId, subject, startTime, time(nullptr), notes));
        return nextSessionId++;
    }
   
    bool endSession(int sessionId, time_t endTime, int breakTime = 0) {
        for (auto& session : sessions) {
            if (session.getId() == sessionId) {
                session = StudySession(session.getId(), session.getSubject(),
                                    session.getStartTime(), endTime, session.getNotes(), breakTime);
                return true;
            }
        }
        return false;
    }
   
    StudySession* getSession(int sessionId) {
        for (auto& session : sessions)
            if (session.getId() == sessionId) return &session;
        return nullptr;
    }
   
    vector<StudySession> getAllSessions() const { return sessions; }
   
    map<string, int> getTimePerSubject() const {
        map<string, int> result;
        for (const auto& session : sessions) {
            string subject = session.getSubject();
            if (result.find(subject) == result.end()) result[subject] = session.getDuration();
            else result[subject] += session.getDuration();
        }
        return result;
    }
   
    int getTotalStudyTime() const {
        int total = 0;
        for (const auto& session : sessions) total += session.getDuration();
        return total;
    }
   
    TodoList& getTodoList() { return todoList; }
    const TodoList& getTodoList() const { return todoList; }
   
    bool saveUserData(const string& sessionFile, const string& todoFile) const {
        ofstream sessionOut(sessionFile);
        if (!sessionOut.is_open()) return false;
       
        for (const auto& session : sessions) sessionOut << session.serialize() << endl;
        sessionOut << "NEXT_ID=" << nextSessionId << endl;
        sessionOut.close();
       
        return todoList.saveToFile(todoFile);
    }
   
    bool loadUserData(const string& sessionFile, const string& todoFile) {
        ifstream sessionIn(sessionFile);
        if (sessionIn.is_open()) {
            sessions.clear();
            nextSessionId = 1;
           
            string line;
            while (getline(sessionIn, line)) {
                if (line.substr(0, 8) == "NEXT_ID=") nextSessionId = stoi(line.substr(8));
                else if (!line.empty()) sessions.push_back(StudySession::deserialize(line));
            }
            sessionIn.close();
        }
        return todoList.loadFromFile(todoFile);
    }
   
};
User* currentUser = nullptr;
class FileUploader {
private:
    string uploadDir;
   
public:
    FileUploader(const string& dir = "uploads") : uploadDir(dir) {
        // to create upload directory when the file does not exist
        fs::create_directories(uploadDir);
    }
   
    bool uploadFile(const string& sourcePath, const string& destFilename, string& outPath) {
        if (!fs::exists(sourcePath)) {
            cerr << "Source file does not exist: " << sourcePath << endl;
            return false;
        }
       
        // Create user specific directory
        string userDir = uploadDir + "/" + to_string(currentUser->getId());
        fs::create_directories(userDir);
       
        // Generate destination path
        string destPath = userDir + "/" + destFilename;
       
        try {
            // Copy of the file
            fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
            outPath = destPath;
            return true;
        } catch (const fs::filesystem_error& e) {
            cerr << "Error copying file: " << e.what() << endl;
            return false;
        }
    }
   
};
//----------------SDL FUNCTIONS--------------------
void renderText(SDL_Renderer* renderer, TTF_Font* font, const string& text, int x, int y, SDL_Color color, bool centered = false) {
    if (!font || text.empty()) return;
   
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;
   
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
   
    SDL_Rect rect;
    rect.w = surface->w;
    rect.h = surface->h;
   
    if (centered) {
        rect.x = x - rect.w / 2;
        rect.y = y - rect.h / 2;
    } else {
        rect.x = x;
        rect.y = y;
    }
   
    SDL_RenderCopy(renderer, texture, NULL, &rect);
   
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// ==================== SDL PIE CHART CLASS ====================
class SDLPieChart {
private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    int centerX, centerY;
    int radius;
    string title;
    vector<pair<string, int>> data;
    vector<SDL_Color> colors;
   
    // Convert degrees to radians
    float degToRad(float degrees) const {
        return degrees * M_PI / 180.0f;
    }
   
    //To  Draw the pie slice
    void drawFilledArc(int x, int y, int radius, float startAngle, float endAngle, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
       
        //To convert angles to radians
        float startRad = degToRad(startAngle - 90); // -90 to start at top
        float endRad = degToRad(endAngle - 90);
       
        // Draw filled pie slice using triangles
        for (float angle = startRad; angle <= endRad; angle += 0.01f) {
            int x1 = x + static_cast<int>(radius * cos(angle));
            int y1 = y + static_cast<int>(radius * sin(angle));
            SDL_RenderDrawLine(renderer, x, y, x1, y1);
        }
       
        // To draw outline
        for (float angle = startRad; angle <= endRad; angle += 0.01f) {
            int x1 = x + static_cast<int>(radius * cos(angle));
            int y1 = y + static_cast<int>(radius * sin(angle));
            SDL_RenderDrawPoint(renderer, x1, y1);
        }
    }

public:
    SDLPieChart(SDL_Renderer* renderer, TTF_Font* font, int centerX, int centerY, int radius)
        : renderer(renderer), font(font), centerX(centerX), centerY(centerY), radius(radius), title("Pie Chart") {
        // Initialize default colors
        colors = {
            {255, 0, 0, 255},     // Red
            {0, 255, 0, 255},     // Green
            {0, 0, 255, 255},     // Blue
            {255, 255, 0, 255},   // Yellow
            {255, 0, 255, 255},   // Magenta
            {0, 255, 255, 255},   // Cyan
            {255, 128, 0, 255},   // Orange
            {128, 0, 255, 255},   // Purple
            {0, 128, 255, 255},   // Light blue
            {128, 255, 0, 255}    // Lime
        };
    }
    //To  add a data point to the pie chart
    void addDataPoint(const string& label, int value) {
        data.push_back(make_pair(label, value));
    }
    // Clear all data
    void clearData() {
        data.clear();
    }
   
    // Set chart title
    void setTitle(const string& newTitle) {
        title = newTitle;
    }
   
    // Render the pie chart
    void render() {
        if (data.empty()) return;
       
        // Calculate total value
        int total = 0;
        for (const auto& pair : data) total += pair.second;
       
        // Draw title
        SDL_Color titleColor = {255, 255, 255, 255};
        renderText(renderer, font, title, centerX, centerY - radius - 30, titleColor, true);
       
        // Draw each slice
        float currentAngle = 0.0f;
        for (size_t i = 0; i < data.size(); ++i) {
            const auto& pair = data[i];
            SDL_Color color = colors[i % colors.size()];
           
            // Calculate angles
            float sliceAngle = (static_cast<float>(pair.second) / total) * 360.0f;
            float midAngle = currentAngle + sliceAngle / 2.0f;
           
            // To draw the slice
            drawFilledArc(centerX, centerY, radius, currentAngle, currentAngle + sliceAngle, color);
           
            // To calculate label
            float labelDistance = radius * 1.3f;
            int labelX = centerX + static_cast<int>(labelDistance * cos(degToRad(midAngle - 90)));
            int labelY = centerY + static_cast<int>(labelDistance * sin(degToRad(midAngle - 90)));
           
            // To Draw label
            renderText(renderer, font, pair.first, labelX, labelY, color, true);
           
            // To draw value and percentage
            float percentage = (static_cast<float>(pair.second) / total) * 100.0f;
            string valueText = Utils::formatDuration(pair.second) +
                                   " (" + to_string(static_cast<int>(percentage)) + "%)";
           
            // Draw value text slightly below the label
            renderText(renderer, font, valueText, labelX, labelY + 20, color, true);
           
            // Update current angle for next slice
            currentAngle += sliceAngle;
        }
       
        // Draw legend
        int legendX = centerX + radius + 50;
        int legendY = centerY - radius;
        int legendWidth = 200;
        int legendHeight = data.size() * 25 + 40;
       
        // Draw legend background
        SDL_Rect legendRect = {legendX, legendY, legendWidth, legendHeight};
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &legendRect);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &legendRect);
       
        // Draw legend title
        renderText(renderer, font, "Legend", legendX + legendWidth / 2, legendY + 20, titleColor, true);
       
        // Draw legend items
        for (size_t i = 0; i < data.size(); i++) {
            const auto& pair = data[i];
            SDL_Color color = colors[i % colors.size()];
            int itemY = legendY + 50 + i * 25;
           
            // Draw color box
            SDL_Rect colorRect = {legendX + 20, itemY, 15, 15};
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(renderer, &colorRect);
           
            // Draw label and value
            string legendText = pair.first + ": " + Utils::formatDuration(pair.second);
            renderText(renderer, font, legendText, legendX + 45, itemY + 7, titleColor, false);
        }
    }
};

// ----------- VISUALIZATION CLASS (to show visulation on terminal only)------
class Visualization {
protected:
    User* user;
    string content;
    int width;
   
    string getBarSymbol(int value, int maxValue, int maxBars = 50) const {
        if (maxValue <= 0) return "";
        int numBars = static_cast<int>((static_cast<double>(value) / maxValue) * maxBars);
        return string(numBars, '|');
    }

public:
    Visualization(User* user, int width = 80) : user(user), width(width) {}
    virtual ~Visualization() {}
   
    virtual void render() = 0;
    string getContent() const { return content; }
   
    void saveToFile(const string& filename) {
        ofstream file(filename);
        if (file.is_open()) {
            file << content;
            file.close();
            cout << "Visualization saved to " << filename << endl;
        }
    }
};

class BarChart : public Visualization {
private:
    string title;
    vector<pair<string, int>> data;

public:
    BarChart(User* user, const string& title) : Visualization(user, 80), title(title) {}
   
    void addDataPoint(const string& label, int value) {
        data.push_back(make_pair(label, value));
    }
   
    void render() override {
        stringstream ss;
        int maxValue = 0;
        for (const auto& pair : data)
            if (pair.second > maxValue) maxValue = pair.second;
       
        ss << "+" << string(width - 2, '-') << "+" << endl;
        ss << "|" << setw((width - 2 - title.length()) / 2) << " " << title
           << setw((width - 2 - title.length() + 1) / 2) << " " << "|" << endl;
        ss << "+" << string(width - 2, '-') << "+" << endl;
       
        for (const auto& pair : data) {
            ss << "| " << left << setw(15) << pair.first.substr(0, 15) << " | "
               << getBarSymbol(pair.second, maxValue, width - 30) << " "
               << right << setw(8) << Utils::formatDuration(pair.second) << " |" << endl;
        }
       
        ss << "+" << string(width - 2, '-') << "+" << endl;
        content = ss.str();
        cout << content << endl;
    }
};

class PieChart : public Visualization {
private:
    string title;
    vector<pair<string, int>> data;
   
    // SDL-related members
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    bool sdlInitialized;

public:
    PieChart(User* user, const string& title)
        : Visualization(user, 80), title(title), window(nullptr), renderer(nullptr), font(nullptr), sdlInitialized(false) {}
   
    ~PieChart() {
        // Clean up SDL resources if initialized
        if (sdlInitialized) {
            if (font) TTF_CloseFont(font);
            if (renderer) SDL_DestroyRenderer(renderer);
            if (window) SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
        }
    }
   
    void addDataPoint(const string& label, int value) {
        data.push_back(make_pair(label, value));
    }
   
    // Text-based rendering for console output
    void renderTextBased() {
        stringstream ss;
        int total = 0;
        for (const auto& pair : data) total += pair.second;
       
        ss << "+" << string(width - 2, '-') << "+" << endl;
        ss << "|" << setw((width - 2 - title.length()) / 2) << " " << title
           << setw((width - 2 - title.length() + 1) / 2) << " " << "|" << endl;
        ss << "+" << string(width - 2, '-') << "+" << endl;
       
        for (const auto& pair : data) {
            double percentage = (total > 0) ? (pair.second * 100.0 / total) : 0.0;
           
            ss << "| " << left << setw(15) << pair.first.substr(0, 15) << " | "
               << getBarSymbol(pair.second, total, width - 40) << " "
               << fixed << setprecision(1) << right << setw(5) << percentage << "% | "
               << right << setw(8) << Utils::formatDuration(pair.second) << " |" << endl;
        }
       
        ss << "+" << string(width - 2, '-') << "+" << endl;
        content = ss.str();
        cout << content << endl;
    }
   
    // Initialize SDL for graphical rendering
    bool initSDL() {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
            return false;
        }
       
       
        if (TTF_Init() < 0) {
            cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << endl;
            SDL_Quit();
            return false;
        }
       
        // to Create window
        window = SDL_CreateWindow(("Study Time - " + title).c_str(),
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 800, 600,
                                 SDL_WINDOW_SHOWN);
        if (!window) {
            cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
            TTF_Quit();
            SDL_Quit();
            return false;
        }
       

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return false;
        }
        //to  Load font
        font = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", 16);
        if (!font) {
            cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << endl;
                  }
       
        sdlInitialized = true;
        return true;
    }
    // Render the pie chart using SDL
    void renderSDL() {
        if (!sdlInitialized && !initSDL()) {
            cerr << "Failed to initialize SDL. Falling back to text-based rendering." << endl;
            renderTextBased();
            return;
        }
        // to Create pie chart
        SDLPieChart pieChart(renderer, font, 400, 300, 150);
        pieChart.setTitle(title);
        for (const auto& pair : data) {
            pieChart.addDataPoint(pair.first, pair.second);
        }
     
        // Main loop flag
        bool quit = false;
        // Event handler
        SDL_Event e;
        while (!quit) {
            // Handle events
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) {
                    quit = true;
                }
                else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_RETURN) {
                        quit = true;
                    }
                }
            }
           
            // Clear screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
           
            // Render pie chart
            pieChart.render();
           
            // Render instructions
            SDL_Color textColor = {255, 255, 255, 255};
            renderText(renderer, font, "Press ESC or ENTER to return", 400, 550, textColor, true);
           
            // Update screen
            SDL_RenderPresent(renderer);
           
            // Cap to 60 FPS
            SDL_Delay(16);
        }
    }
   
    void render() override {
        // First generate the text-based content for saving to file
        renderTextBased();
       
        // Then render the graphical version if requested
        cout << "Would you like to view the pie chart graphically? (y/n): ";
        string response;
        getline(cin, response);
       
        if (response == "y" || response == "Y") {
            renderSDL();
        }
    }
};

// ------------------ BREAK TIME FUNCTIONS ------------------------
// Function to play background music during breaks
void playBackgroundMusic(const char* musicFile) {
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << endl;
        return;
    }
   
    // Load music file
    backgroundMusic = Mix_LoadMUS(musicFile);
    if (!backgroundMusic) {
        cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << endl;
        Mix_CloseAudio();
        return;
    }
   
    // Play music on loop
    Mix_PlayMusic(backgroundMusic, -1);
   
    // Keep thread alive while break is active
    while (breakActive) {
        SDL_Delay(100);
    }
   
    // Stop and clean up when break ends
    Mix_HaltMusic();
    Mix_FreeMusic(backgroundMusic);
    backgroundMusic = nullptr;
    Mix_CloseAudio();
}
// Utility functions
void clearScreen() { system("clear"); }
void pauseExecution() {
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
// Function to handle break time
void takeBreak() {
    clearScreen();
    cout << "===== BREAK TIME =====" << endl;
    cout << "Your study session is paused. Enjoy your break!" << endl;
    cout << "Playing relaxing music..." << endl;
    cout << "\nPress Enter to end your break and resume studying..." << endl;
   
    // Set break as active
    breakActive = true;
   
    // Start music in a separate thread
    musicThread = thread(playBackgroundMusic, "/Users/chiragsingh/Downloads/blue\ bird\ \(Naruto\ but\ is\ it\ okay\ if\ it\'s\ lofi\ hiphop_\).mp3");
   
    // Wait for user to press Enter
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
   
    // End break
    breakActive = false;
   
    // Wait for music thread to finish
    if (musicThread.joinable()) {
        musicThread.join();
    }
   
    cout << "Break ended. Resuming study session..." << endl;
    pauseExecution();
}

// --------------------------FILE MANAGEMENT --------------------
class FileManager {
public:
    static bool saveUserData(int userId, User* user) {
        if (!user) return false;
        string userDir = "data/user_" + to_string(userId);
        system(("mkdir -p " + userDir).c_str());
        return user->saveUserData(userDir + "/sessions.dat", userDir + "/todo.dat");
    }
   
    static bool loadUserData(int userId, User* user) {
        if (!user) return false;
        string userDir = "data/user_" + to_string(userId);
        return user->loadUserData(userDir + "/sessions.dat", userDir + "/todo.dat");
    }
   
    static bool saveUserCredentials(const vector<User*>& users) {
        ofstream file("users.dat");
        if (!file.is_open()) return false;
        for (const auto& user : users) {
            if (user) file << user->getId() << "|" << user->getUsername() << "|"
                           << user->getFullName() << "|" << endl;
        }
        return true;
    }
};

// ----------------------MAIN APPLICATION-----------------------
vector<User*> users;
int nextUserId = 1;
FileUploader fileUploader;

void uploadImageToSession(int sessionId);
void uploadNotesToSession(int sessionId);
void viewSessionAttachments(int sessionId);

string getStringInput(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    return input;
}


string getPasswordInput() {
    cout << "Enter password: ";
    string password;
    getline(cin, password);
    return password;
}

int getIntInput(const string& prompt) {
    cout << prompt;
    string line;
    getline(cin, line);
    try { return line.empty() ? 0 : stoi(line); }
    catch (...) { return 0; }
}

time_t getDateInput(const string& prompt) {
    string dateStr = getStringInput(prompt);
    time_t date = Utils::parseDateTime(dateStr);
    if (date == 0) {
        cout << "Invalid date format. Using current time instead." << endl;
        date = time(nullptr);
    }
    return date;
}

// Function declarations
void displayMainMenu();
void displayLoginMenu();
void registerUser();
bool loginUser();
void displayStudyMenu();
void startStudySession();
void endStudySession();
void viewStudySessions();
void displayVisualizationMenu();
void createBarChart();
void createPieChart();
void displayTodoMenu();
void addTodoItem();
void viewTodoList();
void markTodoItemComplete();
void removeTodoItem();
void generateReport();
void viewRankings();

// Core functionality implementations
void registerUser() {
    clearScreen();
    cout << "--------------REGISTRATION-------------"<< endl;
    string username = getStringInput("Enter username: ");
    string password = getPasswordInput();
    string fullName = getStringInput("Enter full name: ");
   
    for (const auto& user : users) {
        if (user->getUsername() == username) {
            cout << "Username already exists." << endl;
            pauseExecution();
            return;
        }
    }
   
    User* newUser = new User(nextUserId++, username, password, fullName);
    users.push_back(newUser);
    FileManager::saveUserCredentials(users);
    FileManager::saveUserData(newUser->getId(), newUser);
   
    cout << "Registration successful." << endl;
    pauseExecution();
}

bool loginUser() {
    clearScreen();
    cout << "===== LOGIN =====" << endl;
    string username = getStringInput("Enter username: ");
    string password = getPasswordInput();
   
    for (auto& user : users) {
        if (user->getUsername() == username) {
            if (user->verifyPassword(password)) {
                currentUser = user;
                FileManager::loadUserData(user->getId(), user);
                cout << "Login successful. Welcome, " << user->getFullName() << "!" << endl;
                pauseExecution();
                return true;
            } else {
                cout << "Incorrect password." << endl;
                pauseExecution();
                return false;
            }
        }
    }
   
    cout << "Username not found." << endl;
    pauseExecution();
    return false;
}

// Modified startStudySession function with break time feature
void startStudySession() {
    clearScreen();
    cout << "===== START STUDY SESSION =====" << endl;
    string subject = getStringInput("Enter subject: ");
    string notes = getStringInput("Enter notes (optional): ");
   
    time_t now = time(nullptr);
    int sessionId = currentUser->startSession(subject, now, notes);
   
    cout << "Study session #" << sessionId << " started at "
         << Utils::formatDateTime(now) << endl;
   
    FileManager::saveUserData(currentUser->getId(), currentUser);
   
    bool sessionActive = true;
    time_t breakStartTime = 0;
    time_t totalBreakTime = 0;
   
    while (sessionActive) {
        clearScreen();
        cout << "===== ACTIVE STUDY SESSION =====" << endl;
        cout << "Session #" << sessionId << " - " << subject << endl;
        cout << "Started at: " << Utils::formatDateTime(now) << endl;
        cout << "Current time: " << Utils::formatDateTime(time(nullptr)) << endl;
        cout << "Elapsed time: " << Utils::formatDuration(difftime(time(nullptr), now) - totalBreakTime) << endl;
       
        if (totalBreakTime > 0) {
            cout << "Total break time: " << Utils::formatDuration(totalBreakTime) << endl;
        }
       
        cout << "\nOptions:" << endl;
        cout << "0 - Continue studying (return to this screen)" << endl;
        cout << "1 - Take a break (pauses study timer)" << endl;
        cout << "2 - End study session" << endl;
       
        int choice = getIntInput("\nEnter your choice: ");
       
        switch (choice) {
            case 0:
                break;
                //resets the screen
            case 1:
                breakStartTime = time(nullptr);
                takeBreak();
                totalBreakTime += difftime(time(nullptr), breakStartTime);
                break;
            case 2:
                sessionActive = false;
                time_t endTime = time(nullptr);
                // Adjust end time to exclude break time
                if (currentUser->endSession(sessionId, endTime, totalBreakTime)) {
                    cout << "Study session #" << sessionId << " ended at " << Utils::formatDateTime(endTime) << endl;
                    StudySession* session = currentUser->getSession(sessionId);
                    if (session) {
                        cout << "Duration: " << Utils::formatDuration(session->getDuration()) << endl;
                        if (totalBreakTime > 0) {
                            cout << "Break time: " << Utils::formatDuration(totalBreakTime) << endl;
                        }
                    }
                    FileManager::saveUserData(currentUser->getId(), currentUser);
                }
                break;
        }
    }
}

void endStudySession() {
    clearScreen();
    cout << "===== END STUDY SESSION =====" << endl;
   
    vector<StudySession> sessions = currentUser->getAllSessions();
    if (sessions.empty()) {
        cout << "No study sessions found." << endl;
        pauseExecution();
        return;
    }
   
    int sessionId = getIntInput("Enter session ID to end (0 for latest): ");
    if (sessionId == 0) {
        int latestId = 0;
        for (const auto& session : sessions)
            if (session.getId() > latestId) latestId = session.getId();
        sessionId = latestId;
    }
   
    time_t now = time(nullptr);
    if (currentUser->endSession(sessionId, now)) {
        cout << "Study session #" << sessionId << " ended at " << Utils::formatDateTime(now) << endl;
        StudySession* session = currentUser->getSession(sessionId);
        if (session) cout << "Duration: " << Utils::formatDuration(session->getDuration()) << endl;
        FileManager::saveUserData(currentUser->getId(), currentUser);
    } else {
        cout << "Session not found or already ended." << endl;
    }
   
    pauseExecution();
}

void viewStudySessions() {
    clearScreen();
    cout << "------------------ STUDY SESSIONS----------------"<< endl;
   
    vector<StudySession> sessions = currentUser->getAllSessions();
    if (sessions.empty()) {
        cout << "No study sessions found." << endl;
        pauseExecution();
        return;
    }
   
    sort(sessions.begin(), sessions.end(),
             [](const StudySession& a, const StudySession& b) {
                 return a.getStartTime() > b.getStartTime();
             });
   
    for (const auto& session : sessions) cout << session.toString() << endl;
    pauseExecution();
}

// New function to upload image to a session
void uploadImageToSession(int sessionId) {
    clearScreen();
    cout << "===== UPLOAD IMAGE TO SESSION =====" << endl;
   
    StudySession* session = currentUser->getSession(sessionId);
    if (!session) {
        cout << "Session not found." << endl;
        pauseExecution();
        return;
    }
   
    string sourcePath = getStringInput("Enter path to image file: ");
    if (sourcePath.empty()) {
        cout << "No file specified." << endl;
        pauseExecution();
        return;
    }
   
    string filename = "image_" + to_string(sessionId) + "_" + to_string(time(nullptr));
   
    // Extract extension from source path
    size_t dotPos = sourcePath.find_last_of(".");
    if (dotPos != string::npos) {
        filename += sourcePath.substr(dotPos);
    }
   
    string destPath;
    if (fileUploader.uploadFile(sourcePath, filename, destPath)) {
        session->attachImage(destPath);
        cout << "Image uploaded successfully." << endl;
        FileManager::saveUserData(currentUser->getId(), currentUser);
    } else {
        cout << "Failed to upload image." << endl;
    }
   
    pauseExecution();
}


void uploadNotesToSession(int sessionId) {
    clearScreen();
    cout << "===== UPLOAD NOTES TO SESSION =====" << endl;
   
    StudySession* session = currentUser->getSession(sessionId);
    if (!session) {
        cout << "Session not found." << endl;
        pauseExecution();
        return;
    }
   
    string sourcePath = getStringInput("Enter path to notes file: ");
    if (sourcePath.empty()) {
        cout << "No file specified." << endl;
        pauseExecution();
        return;
    }
   
    string filename = "notes_" + to_string(sessionId) + "_" + to_string(time(nullptr));
   
    // Extract extension from source path
    size_t dotPos = sourcePath.find_last_of(".");
    if (dotPos != string::npos) {
        filename += sourcePath.substr(dotPos);
    }
   
    string destPath;
    if (fileUploader.uploadFile(sourcePath, filename, destPath)) {
        session->attachFile(destPath);
        cout << "Notes file uploaded successfully." << endl;
        FileManager::saveUserData(currentUser->getId(), currentUser);
    } else {
        cout << "Failed to upload notes file." << endl;
    }
   
    pauseExecution();
}

// New function to view session attachments
void viewSessionAttachments(int sessionId) {
    clearScreen();
    cout << "===== SESSION ATTACHMENTS =====" << endl;
   
    StudySession* session = currentUser->getSession(sessionId);
    if (!session) {
        cout << "Session not found." << endl;
        pauseExecution();
        return;
    }
   
    cout << "Session #" << sessionId << " - " << session->getSubject() << endl;
    cout << "------------------------------------" << endl;
   
    string imagePath = session->getAttachedImage();
    if (!imagePath.empty()) {
        cout << "Attached Image: " << imagePath << endl;
    } else {
        cout << "No image attached." << endl;
    }
   
    vector<string> files = session->getAttachedFiles();
    if (!files.empty()) {
        cout << "Attached Files:" << endl;
        for (size_t i = 0; i < files.size(); i++) {
            cout << (i+1) << ". " << files[i] << endl;
        }
    } else {
        cout << "No files attached." << endl;
    }
   
    pauseExecution();
}

void createBarChart() {
    BarChart chart(currentUser, "Time Per Subject");
   
    if (currentUser->getAllSessions().empty()) {
        // Add sample data for demonstration when no sessions exist
        cout << "No study sessions found. Showing sample visualization data." << endl;
        chart.addDataPoint("Sample Math", 3600);  // 1 hour
        chart.addDataPoint("Sample Science", 1800);  // 30 minutes
    } else {
        // Use real data
        map<string, int> timePerSubject = currentUser->getTimePerSubject();
        for (const auto& pair : timePerSubject) chart.addDataPoint(pair.first, pair.second);
    }
   
    cout << "Rendering bar chart..." << endl;
    chart.render();
   
    if (getStringInput("Save this chart to a file? (y/n): ") == "y") {
        chart.saveToFile("bar_chart.txt");
    }
    pauseExecution();
}

void createPieChart() {
    PieChart chart(currentUser, "Subject Distribution");
   
    if (currentUser->getAllSessions().empty()) {
        // Add sample data for demonstration when no sessions exist
        cout << "No study sessions found. Showing sample visualization data." << endl;
        chart.addDataPoint("Sample Math", 3600);  // 1 hour
        chart.addDataPoint("Sample Science", 1800);  // 30 minutes
        chart.addDataPoint("Sample English", 2700);  // 45 minutes
    } else {
        // Use real data
        map<string, int> timePerSubject = currentUser->getTimePerSubject();
        for (const auto& pair : timePerSubject) chart.addDataPoint(pair.first, pair.second);
    }
   
    cout << "Rendering pie chart..." << endl;
    chart.render();
   
    if (getStringInput("Save this chart to a file? (y/n): ") == "y") {
        chart.saveToFile("pie_chart.txt");
    }
    pauseExecution();
}

void addTodoItem() {
    clearScreen();
    cout << "===== ADD TODO ITEM =====" << endl;
   
    string description = getStringInput("Enter task description: ");
    string subject = getStringInput("Enter subject (optional): ");
   
    cout << "Priority: 1-High, 2-Medium, 3-Low" << endl;
    int priority = getIntInput("Enter priority (1-3): ");
    if (priority < 1 || priority > 3) priority = 2;
   
    time_t dueDate = getDateInput("Enter due date (YYYY-MM-DD) or leave blank: ");
   
    currentUser->getTodoList().addItem(description, priority, dueDate, subject);
    cout << "Todo item added successfully." << endl;
    FileManager::saveUserData(currentUser->getId(), currentUser);
    pauseExecution();
}

void viewTodoList() {
    clearScreen();
    cout << "===== TODO LIST =====" << endl;
   
    cout << "Sort: 1-Default, 2-Priority, 3-Due Date" << endl;
    int sortOption = getIntInput("Select sorting option: ");
   
    vector<TodoItem> items;
    switch (sortOption) {
        case 2: items = currentUser->getTodoList().getItemsSortedByPriority(); break;
        case 3: items = currentUser->getTodoList().getItemsSortedByDueDate(); break;
        default: items = currentUser->getTodoList().getAllItems();
    }
   
    if (items.empty()) {
        cout << "No todo items found." << endl;
        pauseExecution();
        return;
    }
    cout << "ID | Status | Description" << endl;
    cout << "-------------------------------------------" << endl;
   
    for (const auto& item : items) {
        cout << setw(3) << item.getId() << " | "
                  << (item.isCompleted() ? "[X]" : "[ ]") << " | "
                  << item.toString() << endl;
    }
   
    pauseExecution();
}

void markTodoItemComplete() {
    clearScreen();
    cout << "===== MARK ITEM AS COMPLETE =====" << endl;
   
    vector<TodoItem> items = currentUser->getTodoList().getIncompleteItems();
    if (items.empty()) {
        cout << "No incomplete todo items found." << endl;
        pauseExecution();
        return;
    }
   
    cout << "Incomplete items:" << endl;
    for (const auto& item : items) {
        cout << setw(3) << item.getId() << " | " << item.getDescription() << endl;
    }
   
    int itemId = getIntInput("Enter item ID to mark as complete (0 to cancel): ");
    if (itemId == 0) return;
   
    if (currentUser->getTodoList().markAsCompleted(itemId)) {
        cout << "Item marked as complete." << endl;
        FileManager::saveUserData(currentUser->getId(), currentUser);
    } else {
        cout << "Item not found or already complete." << endl;
    }
    pauseExecution();
}

void removeTodoItem() {
    clearScreen();
    cout << "===== REMOVE TODO ITEM =====" << endl;
   
    vector<TodoItem> items = currentUser->getTodoList().getAllItems();
    if (items.empty()) {
        cout << "No todo items found." << endl;
        pauseExecution();
        return;
    }
   
    cout << "All items:" << endl;
    for (const auto& item : items) {
        cout << setw(3) << item.getId() << " | "
                  << (item.isCompleted() ? "[X]" : "[ ]") << " | "
                  << item.getDescription() << endl;
    }
   
    int itemId = getIntInput("Enter item ID to remove (0 to cancel): ");
    if (itemId == 0) return;
   
    if (currentUser->getTodoList().removeItem(itemId)) {
        cout << "Item removed successfully." << endl;
        FileManager::saveUserData(currentUser->getId(), currentUser);
    } else {
        cout << "Item not found." << endl;
    }
    pauseExecution();
}

void generateReport() {
    clearScreen();
    cout << "===== STUDY REPORT =====" << endl;
   
    vector<StudySession> sessions = currentUser->getAllSessions();
    if (sessions.empty()) {
        cout << "No study data available for report." << endl;
        pauseExecution();
        return;
    }
   
    int totalTime = currentUser->getTotalStudyTime();
    map<string, int> timePerSubject = currentUser->getTimePerSubject();
   
    string mostStudiedSubject = "";
    int maxTime = 0;
    for (const auto& pair : timePerSubject) {
        if (pair.second > maxTime) {
            maxTime = pair.second;
            mostStudiedSubject = pair.first;
        }
    }
   
    cout << "Study Summary for " << currentUser->getFullName() << endl;
    cout << "------------------------------------" << endl;
    cout << "Total study sessions: " << sessions.size() << endl;
    cout << "Total study time: " << Utils::formatDuration(totalTime) << endl;
    cout << "Number of subjects: " << timePerSubject.size() << endl;
   
    if (!mostStudiedSubject.empty()) {
        cout << "Most studied: " << mostStudiedSubject << " ("
                  << Utils::formatDuration(maxTime) << ")" << endl;
    }
   
    cout << "------------------------------------" << endl;
    cout << "Time per subject:" << endl;
   
    for (const auto& pair : timePerSubject) {
        cout << setw(15) << left << pair.first << ": "
                  << Utils::formatDuration(pair.second) << endl;
    }
   
    if (getStringInput("Save this report to a file? (y/n): ") == "y") {
        ofstream file("study_report.txt");
        if (file.is_open()) {
            file << "Study Summary for " << currentUser->getFullName() << endl;
            file << "------------------------------------" << endl;
            file << "Total study sessions: " << sessions.size() << endl;
            file << "Total study time: " << Utils::formatDuration(totalTime) << endl;
            file << "Number of subjects: " << timePerSubject.size() << endl;
           
            if (!mostStudiedSubject.empty()) {
                file << "Most studied: " << mostStudiedSubject << " ("
                     << Utils::formatDuration(maxTime) << ")" << endl;
            }
           
            file << "------------------------------------" << endl;
            file << "Time per subject:" << endl;
           
            for (const auto& pair : timePerSubject) {
                file << setw(15) << left << pair.first << ": "
                     << Utils::formatDuration(pair.second) << endl;
            }
           
            file.close();
            cout << "Report saved to study_report.txt" << endl;
        }
    }
   
    // Ask if user wants to see a graphical visualization
    if (getStringInput("Would you like to see a graphical pie chart of your study time? (y/n): ") == "y") {
        PieChart chart(currentUser, "Study Time Distribution");
        for (const auto& pair : timePerSubject) {
            chart.addDataPoint(pair.first, pair.second);
        }
        chart.renderSDL();
    }
   
    pauseExecution();
}

void viewRankings() {
    clearScreen();
    cout << "===== RANKINGS =====" << endl;
   
    int totalTime = currentUser->getTotalStudyTime();
    string rank = "Beginner";
    string medal = "None";
   
    if (totalTime > 36000) { rank = "Gold"; medal = "Gold"; }
    else if (totalTime > 18000) { rank = "Silver"; medal = "Silver"; }
    else if (totalTime > 3600) { rank = "Bronze"; medal = "Bronze"; }
   
    cout << "Your Study Rank: " << rank << " (" << medal << ")" << endl;
    cout << "Total Study Time: " << Utils::formatDuration(totalTime) << endl;
    cout << "Ranking Requirements:" << endl;
    cout << "Gold: 10+ hours | Silver: 5+ hours | Bronze: 1+ hour" << endl;
    pauseExecution();
}

// Updated menu implementations
void displayMainMenu() {
    while (currentUser) {
        clearScreen();
        cout << "===== MAIN MENU =====" << endl;
        cout << "Welcome, " << currentUser->getFullName() << "!" << endl;
        cout << "1. Study Sessions" << endl;
        cout << "2. To-Do List" << endl;
        cout << "3. Visualizations" << endl;
        cout << "4. Reports" << endl;
        cout << "5. Rankings" << endl;
        cout << "6. Logout" << endl;
       
        switch (getIntInput("Enter your choice: ")) {
            case 1: displayStudyMenu(); break;
            case 2: displayTodoMenu(); break;
            case 3: displayVisualizationMenu(); break;
            case 4: generateReport(); break;
            case 5: viewRankings(); break;
            case 6:
                FileManager::saveUserData(currentUser->getId(), currentUser);
                currentUser = nullptr;
                return;
            default:
                cout << "Invalid choice." << endl;
                pauseExecution();
        }
    }
}

void displayLoginMenu() {
    while (true) {
        clearScreen();
        cout << "===== STUDY TRACKER =====" << endl;
        cout << "1. Login" << endl;
        cout << "2. Register" << endl;
        cout << "3. Exit" << endl;
       
        switch (getIntInput("Enter your choice: ")) {
            case 1: if (loginUser()) displayMainMenu(); break;
            case 2: registerUser(); break;
            case 3: return;
            default:
                cout << "Invalid choice." << endl;
                pauseExecution();
        }
    }
}

// Updated study menu with file upload options
void displayStudyMenu() {
    while (currentUser) {
        clearScreen();
        cout << "===== STUDY SESSIONS =====" << endl;
        cout << "1. Start Study Session" << endl;
        cout << "2. End Current Session" << endl;
        cout << "3. View Study History" << endl;
        cout << "4. Upload Image to Session" << endl;
        cout << "5. Upload Notes to Session" << endl;
        cout << "6. View Session Attachments" << endl;
        cout << "7. Back to Main Menu" << endl;
       
        switch (getIntInput("Enter your choice: ")) {
            case 1: startStudySession(); break;
            case 2: endStudySession(); break;
            case 3: viewStudySessions(); break;
            case 4: {
                int sessionId = getIntInput("Enter session ID: ");
                uploadImageToSession(sessionId);
                break;
            }
            case 5: {
                int sessionId = getIntInput("Enter session ID: ");
                uploadNotesToSession(sessionId);
                break;
            }
            case 6: {
                int sessionId = getIntInput("Enter session ID: ");
                viewSessionAttachments(sessionId);
                break;
            }
            case 7: return;
            default:
                cout << "Invalid choice." << endl;
                pauseExecution();
        }
    }
}

void displayVisualizationMenu() {
    while (currentUser) {
        clearScreen();
        cout << "===== VISUALIZATIONS =====" << endl;
        cout << "1. Bar Chart - Time per Subject" << endl;
        cout << "2. Pie Chart - Subject Distribution" << endl;
        cout << "3. Back to Main Menu" << endl;
       
        switch (getIntInput("Enter your choice: ")) {
            case 1: createBarChart(); break;
            case 2: createPieChart(); break;
            case 3: return;
            default:
                cout << "Invalid choice." << endl;
                pauseExecution();
        }
    }
}

void displayTodoMenu() {
    while (currentUser) {
        clearScreen();
        cout << "===== TO-DO LIST =====" << endl;
        cout << "1. Add New Item" << endl;
        cout << "2. View Items" << endl;
        cout << "3. Mark Item as Complete" << endl;
        cout << "4. Remove Item" << endl;
        cout << "5. Back to Main Menu" << endl;
       
        switch (getIntInput("Enter your choice: ")) {
            case 1: addTodoItem(); break;
            case 2: viewTodoList(); break;
            case 3: markTodoItemComplete(); break;
            case 4: removeTodoItem(); break;
            case 5: return;
            default:
                cout << "Invalid choice." << endl;
                pauseExecution();
        }
    }
}

int main() {
    system("mkdir -p data");
    displayLoginMenu();
    for (auto user : users) {
        delete user;
    }
    return 0;
}
