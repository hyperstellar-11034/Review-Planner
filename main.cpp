#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cmath>

using namespace std;

struct ReviewSession {
    time_t review_time;
    double duration_ratio;
    string method;
    string description;
    bool is_booster; // true if booster review
    string day_label; // For split sessions (e.g., "1a", "1b")
};

time_t dateToTimeT(int year, int month, int day) {
    tm time_in = {0, 0, 0, day, month - 1, year - 1900};
    return mktime(&time_in);
}

string timeTToDateStr(time_t t) {
    tm *time_out = localtime(&t);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", time_out);
    return string(buffer);
}

vector<int> generateDynamicIntervals(int total_days, double factor = 2.5) {
    vector<int> intervals = {1, 3, 7, 14, 30, 60, 90, 180, 360}; // Based on forgetting curve
    vector<int> filtered_intervals;
    for (int interval : intervals) {
        if (interval <= total_days) {
            filtered_intervals.push_back(interval);
        }
    }
    if (filtered_intervals.empty() || filtered_intervals.back() != total_days) {
        filtered_intervals.push_back(total_days);
    }
    return filtered_intervals;
}

vector<int> addBoosterReviews(const vector<int>& intervals, int total_days, int max_gap_days = 30) {
    vector<int> full_schedule = intervals;
    for (size_t i = 0; i + 1 < intervals.size(); i++) {
        int gap = intervals[i + 1] - intervals[i];
        if (gap > max_gap_days) {
            int booster_day = intervals[i] + gap / 2; // Add one booster in large gaps
            if (booster_day < intervals[i + 1] && booster_day <= total_days) {
                full_schedule.push_back(booster_day);
            }
        }
    }
    sort(full_schedule.begin(), full_schedule.end());
    full_schedule.erase(unique(full_schedule.begin(), full_schedule.end()), full_schedule.end());
    return full_schedule;
}

ReviewSession createReviewSession(time_t learn_date, int day_after, int initial_time, bool is_booster, string day_label) {
    ReviewSession session;
    session.review_time = learn_date + day_after * 24 * 3600;
    session.is_booster = is_booster;
    session.day_label = day_label;

    if (day_after == 0) {
        session.duration_ratio = 1.0;
        session.method = "Initial Full Learning";
        session.description = "Read carefully, understand, take notes, summarize.";
    } else if (is_booster) {
        session.duration_ratio = 0.10; // 10% for booster reviews
        session.method = "Booster Review";
        session.description = "Quick active recall and targeted problem solving.";
    } else {
        if (day_after == 1) {
            session.duration_ratio = 0.1667; // 16.67% to cap at ~120 min for 720 min
            session.method = "Active Recall";
            session.description = "Recall main concepts from memory, review notes briefly.";
        } else if (day_after <= 3) {
            session.duration_ratio = 0.14; // ~100 min for 720 min
            session.method = "Self-Test";
            session.description = "Solve 2-3 problems, review mistakes.";
        } else if (day_after <= 7) {
            session.duration_ratio = 0.10; // ~72 min
            session.method = "Focused Review";
            session.description = "Target weak areas, solve moderate problems.";
        } else if (day_after <= 14) {
            session.duration_ratio = 0.08; // ~58 min
            session.method = "Mixed Practice";
            session.description = "Solve varied problems, test key concepts.";
        } else if (day_after <= 30) {
            session.duration_ratio = 0.07; // ~50 min
            session.method = "Comprehensive Review";
            session.description = "Review all material, simulate exam conditions.";
        } else {
            session.duration_ratio = 0.06; // ~43 min
            session.method = "Long-Term Retention";
            session.description = "Quick review of key points, apply to new problems.";
        }
    }
    return session;
}

vector<ReviewSession> splitReviewSessions(const ReviewSession& session, int initial_time, int max_duration = 120) {
    vector<ReviewSession> sessions;
    double total_duration = session.duration_ratio * initial_time;
    if (total_duration <= max_duration || session.is_booster || session.day_label == "0") {
        sessions.push_back(session);
        return sessions;
    }

    int num_splits = ceil(total_duration / max_duration);
    double split_duration_ratio = session.duration_ratio / num_splits;
    time_t base_time = session.review_time;

    for (int i = 0; i < num_splits; i++) {
        ReviewSession split_session = session;
        split_session.duration_ratio = split_duration_ratio;
        split_session.review_time = base_time + i * 24 * 3600; // Spread over consecutive days
        split_session.day_label = session.day_label + (i == 0 ? "a" : string(1, char('b' + i - 1)));
        sessions.push_back(split_session);
    }
    return sessions;
}

int main() {
    string topic;
    int year, month, day;
    int initial_time;
    int total_days;

    cout << "Enter topic name: ";
    cin.ignore(1, '\n'); // Clear buffer
    getline(cin, topic);

    cout << "Enter chapter completion date (YYYY MM DD): ";
    cin >> year >> month >> day;

    cout << "Enter total learning time in minutes: ";
    cin >> initial_time;

    cout << "Enter total review period in days: ";
    cin >> total_days;
    if (total_days <= 0) total_days = 365;

    time_t learn_date = dateToTimeT(year, month, day);

    cout << "\n*** Topic: " << topic << " ***\n";
    cout << "*** Chapter Completion Date: " << timeTToDateStr(learn_date) << " ***\n";
    cout << "*** Total Learning Time: " << initial_time << " minutes ***\n";
    cout << "*** Total Review Window: " << total_days << " days ***\n\n";

    vector<int> base_intervals = generateDynamicIntervals(total_days);
    vector<int> full_schedule = addBoosterReviews(base_intervals, total_days);

    // Initial learning session
    ReviewSession initial_session = createReviewSession(learn_date, 0, initial_time, false, "0");
    cout << timeTToDateStr(initial_session.review_time) << " (Day 0) (Initial Learning):\n";
    cout << "  - Duration: " << initial_time << " minutes (100% of total study time)\n";
    cout << "  - Method: " << initial_session.method << "\n";
    cout << "  - Details: " << initial_session.description << "\n\n";

    vector<ReviewSession> all_sessions;
    for (int day_after : full_schedule) {
        bool is_booster = find(base_intervals.begin(), base_intervals.end(), day_after) == base_intervals.end();
        ReviewSession session = createReviewSession(learn_date, day_after, initial_time, is_booster, to_string(day_after));
        vector<ReviewSession> split_sessions = splitReviewSessions(session, initial_time);
        all_sessions.insert(all_sessions.end(), split_sessions.begin(), split_sessions.end());
    }

    // Sort sessions by review_time
    sort(all_sessions.begin(), all_sessions.end(), 
         [](const ReviewSession& a, const ReviewSession& b) {
             return a.review_time < b.review_time;
         });

    for (const auto& session : all_sessions) {
        int day_after = (session.review_time - learn_date) / (24 * 3600);
        cout << timeTToDateStr(session.review_time) << " (Day " << session.day_label << "):\n";
        cout << "  - Duration: " << fixed << setprecision(1)
             << session.duration_ratio * initial_time << " minutes ("
             << session.duration_ratio * 100 << "% of total study time)\n";
        cout << "  - Method: " << session.method << "\n";
        cout << "  - Details: " << session.description << "\n";

        if (session.is_booster && day_after != total_days)
            cout << "=== Booster Review: Quick session to reinforce memory! ===\n";
        if (day_after == total_days)
            cout << "=== Final Review: Prepare for application or exam! ===\n";

        cout << endl;
    }

    cout << "Good luck with your studies!\n";

    return 0;
}
