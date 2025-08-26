#pragma once
#include <bits/stdc++.h>
#include <algorithm>
#include "types.hpp"

using namespace std;

class Legalizer {
public:
    Legalizer(string inputFile, string outputFile);
    void readInput(string inputFile);
    void writeOutput(string outputFile);
    void sliceRows();
    void abacus();

    int getRowIdx(Cell& cell);
    int getSubRowIdx(Row& row, Cell& cell);
    pair<int, double> placeRowTrial(Row& row, Cell& cell, bool addPenalty);
    void placeRowFinal(subRow& sr, Cell& cell);
    void determinePosition();
    void fineTuneDisplacement();


private:
    int maxDisplacement;
    vector<Cell> cellList;
    vector<Blockage>  blockageList;
    vector<Row> rowList;
    int violationCount = 0;      // 違反constraint的次數
    int totalPlacements = 0;     // 總放置次數
    double avgDisplacement = 0;  // 平均displacement

    unordered_map<string, int> cellNameToIdx;
};