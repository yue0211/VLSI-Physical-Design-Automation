#pragma once
#include <bits/stdc++.h>
using namespace std;

struct Cell {
    string name;
    int width  = 0;
    int height = 0;
    int weight = 1;  // 預設權重為1
    double x   = 0.0;
    double y   = 0.0;
    double optX = DBL_MAX, optY = DBL_MAX;

    double getCost() {
        if(optX == DBL_MAX || optY == DBL_MAX)
            return DBL_MAX;
        return std::sqrt(pow(optX - x, 2) + pow(optY - y, 2));
    }
};

struct Cluster {
    double x = 0;
    double q = 0;
    int width = 0;
    int weight = 0;
    vector<Cell*> cells;
    Cluster* pre = nullptr;

    Cluster() = default;
    
    Cluster(double x, Cluster* pre) : x(x), pre(pre) {}
    
    void addCell(Cell& cell, int cellX) {
        cells.push_back(&cell);
        q += cell.weight * (cellX - width);  // 修正公式
        width += cell.width;
        weight += cell.weight;
    }
};

struct Blockage {
    string name;
    int width  = 0;
    int height = 0;
    int x      = 0;
    int y      = 0;
};

struct subRow {
    int m_minX = 0;
    int m_maxX = 0;
    int freeWidth = 0;
    Cluster* lastCluster = nullptr;

    subRow(int minX, int maxX) {
        set(minX, maxX);
    }

    void set(int minX, int maxX) {
        m_minX = minX;
        m_maxX = maxX;
        freeWidth = maxX - minX;
    }
};

struct Row {
    string name;
    int siteWidth = 0;
    int height    = 0;
    int x         = 0;
    int y         = 0;
    int siteNum   = 0;
    vector<subRow> subRows;

    inline int alignToSite(double coord, bool ceilFlag) const {
        if (siteWidth <= 0) return static_cast<int>(coord);
        double shift = coord - x;
        double sites = shift / siteWidth;
        double s = ceilFlag ? std::ceil(sites) : std::floor(sites);
        return static_cast<int>(x + s * siteWidth);
    }

    void slice(const Blockage& blkg) {
        if (subRows.empty()) return;
        
        int blkgMinX = alignToSite(blkg.x, false);
        int blkgMaxX = alignToSite(blkg.x + blkg.width, true);

        for (auto it = subRows.begin(); it != subRows.end();) {
            int minX = it->m_minX;
            int maxX = it->m_maxX;

            if (maxX <= blkgMinX || blkgMaxX <= minX) {
                ++it;
                continue;
            }

            if (minX < blkgMinX) {
                if (maxX > blkgMaxX) {
                    size_t idxLeft = std::distance(subRows.begin(), it);
                    subRows.insert(it + 1, subRow(blkgMaxX, maxX));
                    it = subRows.begin() + idxLeft;
                }
                it->set(minX, blkgMinX);
                ++it;
            } else {
                if (maxX > blkgMaxX) {
                    it->set(blkgMaxX, maxX);
                    ++it;
                } else {
                    it = subRows.erase(it);
                }
            }
        }
    }
};
