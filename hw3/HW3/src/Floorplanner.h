#pragma once
#include <bits/stdc++.h>
#include "Timer.h"
using namespace std;

struct Coord {
    int x, y;
    Coord(int _x=0, int _y=0) : x(_x), y(_y) {}
};

struct HardBlock {
    string name;
    int w, h;

    bool rotated;
    Coord coord;

    HardBlock();
    HardBlock(const string& n, int _w, int _h);
    
};

struct Pad {
    string name;
    int x, y;
    Pad();
    Pad(const string& n, int _x, int _y);
};

struct Net {
    string name;
    vector<string> pins;
    Net();
    Net(const string& n);
};


struct Node {
    // Variables
    std::string type;
    int index;
    int width, height;
    int left_from, left_at;
    int right_from, right_at;
    Coord coord;

    // Constructors
    Node();
    Node(string type, int index, int width, int height, int left_from, int left_at, int right_from, int right_at, Coord coord) {
        this->type = type;
        this->index = index;
        this->width = width, this->height = height;
        this->left_from = left_from, this->left_at = left_at;
        this->right_from = right_from, this->right_at = right_at;
        this->coord = coord;
    }
    
};

// Floorplanner 類別
class Floorplanner {
public:
    // 輸入資料
    int numHardBlocks;
    int numPads;
    int numNets;
    int seed;
    unordered_map<string, HardBlock> blocks;
    unordered_map<string, HardBlock> best_blocks;
    unordered_map<string, Pad> pads;
    vector<Net> nets;

    // 版圖大小 (Fixed outline)
    int outlineW, outlineH;

    // 最佳解資訊
    int best_wirelength;

    // 其他參數
    double dead_space_ratio;
    long long total_block_area;
    

public:
    Floorplanner();

    // 讀取
    void readInput(const string& inFile);

    // 計算外框
    void calcOutline();

    // 初始化解
    vector<string> initSolution();

    vector<Node> stockmeyer(const vector<Node>& l, const vector<Node>& r, const string& opType, int parentIndex);

    void update_coord(vector<vector<Node>>& record, int index, int min_at);

    // 計算 cost
    pair<bool, long long>  getCost(const vector<string>& sol, bool withWirelength);

    // pack => stockmeyer
    int getArea(const vector<string>& sol, int &w, int &h, bool withWirelength);

    // 計算 HPWL
    long long getWirelength();

    void invert_chain(vector<string>& sol);

    void swap_random_operand(vector<string>& sol);

    void swap_adjacent_operand(vector<string>& sol);

    // 產生鄰居解
    vector<string> genNeighbor(vector<string> sol, int r);

    // 模擬退火
    pair<vector<string>, int> simulatedAnnealing(vector<string> expression, bool withWirelength, double initTemperature, double minTemperature, double coolingCoefficient, int tryingTimes, double maxRejectRatio, Timer &timer, double timeLimit);
    

    // 輸出
    void writeOutput(const string& outFile);

    bool isCut(string s);

    bool isSkewed(const vector<string> &expression, int idx);

    bool satisfyBallot(const vector<string> &expression, int idx);

    bool isValidPolish(const vector<string>& expr);
    void set_seed();
    
};
