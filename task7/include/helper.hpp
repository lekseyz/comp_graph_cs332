#include <iostream>
#include <sstream>
#include <fstream>
#include <istream>
#include <map>
#include <string>
#include <vector>
#include <stack>
#include <cmath>
#include <limits>
#include <algorithm>

#include "raylib.h"

using namespace std;

class Lexer {
    int start_angle;
    int angle;
    char right_rotate;
    char left_rotate;
    stringstream ss;
    string start;
    map<char, int> vars;
    map<char, string> rules;

    string cur_str;
    
    int iteration;

    void init() {
        ss >> angle >> start_angle;
        ss >> right_rotate >> left_rotate;

        char ch;
        do {
            ss >> ch;
            if (isalpha(ch))
                ss >> vars[ch];
            if (ch == '-') {
                ss >> ch;
                if (ch == '>') {
                    ss >> start;
                    break;
                }
                else
                    throw "parser error";
            }
                
        } while (true);
        
        while (ss >> ch) {
            string arrow;
            if (ss >> arrow && arrow == "->") {
                string rule;
                if (ss >> rule) {
                    rules[ch] = rule;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        cur_str = start;
        iteration = 0;
    }

    string countStr(string prev, int curIter, int targetIter) {
        string next = "";

        if (curIter >= targetIter)
            return prev;

        for (auto ch : prev) {
            if (rules.find(ch) != rules.end()) {
                next.append(rules[ch]);
            }
            else {
                next.push_back(ch);
            }
        }
        return countStr(next, curIter + 1, targetIter);
    }

    public: 

    struct Line {
        Vector2 start;
        Vector2 end;
        int width;
        Color color;
    };

    Lexer(const string& filepath) {
        ifstream file(filepath);
        
        if (!file.is_open()) {
            throw "file open error";
        }
        ss << file.rdbuf();
        file.close();
        init();
    }

    vector<Line> draw(Rectangle& borders) const {
        borders = {0, 0, 0, 0};
        vector<Line> res;
        Vector2 curPos = {0, 0};
        float curAngle = start_angle;
        stack<pair<Vector2, float>> st;
        for (auto ch : cur_str) {
            if (ch == '[') {
                st.push({curPos, curAngle});
                continue;
            }
            if (ch == ']') {
                auto top = st.top();
                st.pop();
                curPos = top.first;
                curAngle = top.second;
                continue;
            }
            if (ch == left_rotate) {
                curAngle += angle;
                continue;
            }
            if (ch == right_rotate) {
                curAngle -= angle;
                continue;
            }

            int length = vars.at(ch);
            Vector2 nextPosition = {.x = cos(DEG2RAD * curAngle) * (float)length + curPos.x, .y = sin(DEG2RAD * curAngle) * (float)length + curPos.y};
            res.push_back((Line) { .start = curPos, .end = nextPosition, .width = 3, .color = BLACK});
            curPos = nextPosition;

            borders.height = max(curPos.y, borders.height);
            borders.width = max(curPos.x, borders.width);
            borders.x = min(curPos.x, borders.x);
            borders.y = min(curPos.y, borders.y);
        }

        return res;
    }

    vector<Line> drawTree(Rectangle& borders) const {
        borders = {0, 0, 0, 0};
        vector<Line> res;
        Vector2 curPos = {0, 0};
        float curAngle = start_angle;
        int max_thickness = 0;
        int thickness = max_thickness;
        for (auto ch : cur_str) {
            if (ch == '[') {
                thickness++;
                max_thickness = max(max_thickness, thickness);
            }
            else if (ch == ']')
                thickness--;
                
        }
        thickness = max_thickness;
        stack<pair<Vector2, pair<float, int>>> st;
        for (auto ch : cur_str) {
            if (ch == '[') {
                st.push({curPos, {curAngle, thickness}});
                thickness--;
                continue;
            }
            if (ch == ']') {
                auto top = st.top();
                st.pop();
                curPos = top.first;
                curAngle = top.second.first;
                thickness++;
                continue;
            }
            if (ch == left_rotate) {
                curAngle += angle + (float)rand() / (float)RAND_MAX * 2;
                continue;
            }
            if (ch == right_rotate) {
                curAngle -= angle + (float)rand() / (float)RAND_MAX * 2;
                continue;
            }

            int length = vars.at(ch);
            Vector2 nextPosition = {.x = cos(DEG2RAD * curAngle) * (float)length + curPos.x, .y = sin(DEG2RAD * curAngle) * (float)length + curPos.y};
            res.push_back((Line) { .start = curPos, .end = nextPosition, .width = thickness, .color = ColorLerp(GREEN, BROWN, (float)thickness / (float)max_thickness)});
            curPos = nextPosition;
            
            borders.height = max(curPos.y, borders.height);
            borders.width = max(curPos.x, borders.width);
            borders.x = min(curPos.x, borders.x);
            borders.y = min(curPos.y, borders.y);
        }

        return res;
    }

    string getPattern() {
        return cur_str;
    }

    int getIteration() {
        return iteration;
    }

    void prevIteration() {
        if (iteration == 0)
            return;
        cur_str = countStr(start, 0, --iteration);
    }

    void nextIteration() {
        iteration = iteration + 1;
        cur_str = countStr(start, 0, iteration);
    }
};