#include "Legalizer.hpp"

Legalizer::Legalizer(string inputFile, string outputFile) {
    // Read input
    readInput(inputFile);

    // Slice rows
    sliceRows();

    // Legalize by Abacus
    abacus();
    determinePosition();

    writeOutput(outputFile);

    cout << "[INFO]: Legalization completed successfully.\n";
}

void Legalizer::readInput(string inputFile) {
    ifstream fin(inputFile);
    if (!fin.is_open()) {
        std::cerr << "[ERROR]: Cannot open input file: " << inputFile << '\n';
        exit(-1);
    }

    string line, token;
    istringstream iss;

    auto isBlank = [](const std::string& s){
        return s.find_first_not_of(" \t\r\n") == std::string::npos;
    };

    //------------------ MaxDisplacementConstraint ------------------//
    while (getline(fin, line)) {
        if (isBlank(line)) continue;
        iss.clear(); iss.str(line);
        iss >> token;
        if (token == "MaxDisplacementConstraint") {
            iss >> maxDisplacement;
            break;
        } else {
            throw runtime_error("Expect MaxDisplacementConstraint.");
        }
    }

    //------------------ Cells ------------------//
    int numCells = 0;
    while (getline(fin, line)) {
        if (isBlank(line)) continue;
        iss.clear(); iss.str(line);
        iss >> token >> numCells;
        if (token == "NumCells") break;
        else throw runtime_error("Expect NumCells.");
    }

    cellList.reserve(numCells);
    for (int i = 0; i < numCells; ++i) {
        getline(fin, line);
        iss.clear(); iss.str(line);

        token.clear();
        Cell cell;
        iss >> token;          // "Cell"
        if (token != "Cell") throw runtime_error("Expect Cell.");
        iss >> cell.name >> cell.width >> cell.height >> cell.x >> cell.y;

        cellNameToIdx[cell.name] = static_cast<int>(cellList.size());
        cellList.push_back(cell);
    }

    //------------------ Blockages ------------------//
    int numBlk = 0;
    while (getline(fin, line)) {
        if (isBlank(line)) continue;
        iss.clear(); iss.str(line);
        iss >> token >> numBlk;
        if (token == "NumBlockages") break;
        else throw runtime_error("Expect NumBlockages.");
    }

    blockageList.reserve(numBlk);
    for (int i = 0; i < numBlk; ++i) {
        getline(fin, line);
        iss.clear(); iss.str(line);

        token.clear();
        Blockage blockage;
        iss >> token;          // "Blockage"
        if (token != "Blockage") throw runtime_error("Expect Blockage.");
        iss >> blockage.name >> blockage.width >> blockage.height >> blockage.x >> blockage.y;

        blockageList.push_back(blockage);
    }


    //------------------ Rows ------------------//
    int numRows = 0;
    while (getline(fin, line)) {
        if (isBlank(line)) continue;
        iss.clear(); iss.str(line);
        iss >> token >> numRows;
        if (token == "NumRows") break;
        else throw runtime_error("Expect NumRows.");
    }

    rowList.reserve(numRows);
    for (int i = 0; i < numRows; ++i) {
        getline(fin, line);
        iss.clear(); iss.str(line);

        token.clear();
        Row row;
        iss >> token;          // "Row"
        if (token != "Row") throw runtime_error("Expect Row.");
        iss >> row.name >> row.siteWidth >> row.height >> row.x >> row.y >> row.siteNum;

        rowList.push_back(row);
        rowList[i].subRows.push_back(subRow(row.x, row.x + row.siteNum * row.siteWidth));
    }


    cout<< "[INFO]: Read input file successfully.\n";
    cout<< "[INFO]: MaxDisplacementConstraint: " << maxDisplacement << '\n';
    cout<< "[INFO]: Number of Cells: " << cellList.size() << '\n';
    cout<< "[INFO]: Number of Blockages: " << blockageList.size() << '\n';
    cout<< "[INFO]: Number of Rows: " << rowList.size() << '\n';
    fin.close();


}


void Legalizer::writeOutput(string outputFile) {
    
    double totalDisplacement = 0.0;
    double maxDisplacement = 0.0;
    for (auto& cell : cellList) {
        double cost = cell.getCost();
        totalDisplacement += cost;
        if (maxDisplacement < cost)
            maxDisplacement = cost;
    }

    cout << "[INFO]: TotalDisplacement "
     << std::fixed
     << std::setprecision(0)
     << std::ceil(totalDisplacement) << '\n';

     cout << "[INFO]: Max Displacement: "
     << std::fixed
     << std::setprecision(0)
     << std::ceil(maxDisplacement) << '\n';
    

    // Variables
    ofstream fout(outputFile);
    fout << "TotalDisplacement "
     << std::fixed
     << std::setprecision(0)
     << std::ceil(totalDisplacement) << '\n';
    
    fout << "MaxDisplacement "
     << std::fixed
     << std::setprecision(0)
     << std::ceil(maxDisplacement) << '\n';

    fout << "NumCells " << cellList.size() << endl;
    // Write cells
    for(const auto& cell : cellList) {
        fout << cell.name << " ";
        fout << static_cast<int>(cell.optX) << " ";
        fout << static_cast<int>(cell.optY) << endl;
    }
}


void Legalizer::sliceRows() {
    // Sort blockages by x (ascending)
    sort(blockageList.begin(), blockageList.end(), [](Blockage& a, Blockage& b) {
        return a.x < b.x;
    });

    // For each blockage, if it occupies a row, slice the row
    for(auto& blockage: blockageList) {
        double blockageMinY = blockage.y;
        double blockageMaxY = blockage.y + blockage.height;

        for(auto& row : rowList) {
            // ── 垂直完全無交集：跳過 ──
            if (row.y + row.height <= blockageMinY || blockageMaxY <= row.y) // row 完全在 blockage 上下方, 跳過
                continue;

            // ── 有交集：切 sub-rows ──
            row.slice(blockage);
        }
    }
}


int Legalizer::getRowIdx(Cell& cell)
{
    int rowIdx = -1;
    double minDisplacement = numeric_limits<double>::max();
    for (size_t i = 0; i < rowList.size(); ++i)
    {
        double displacement = abs(cell.y - rowList[i].y);
        if (minDisplacement > displacement)
        {
            minDisplacement = displacement;
            rowIdx = i;
        }
        else
            break;
    }

    return rowIdx;
}

// 依  row->subRows  的先後順序 (由左到右) 找到
//  1) freeWidth ≥ cell->width
//  2) 位移距離 (displacement) 最小
// 若所有 sub-row 都放不下 → 回傳 -1
int Legalizer::getSubRowIdx(Row& row, Cell& cell)
{
    const vector<subRow>& subRows = row.subRows;

    /* ---------- 1) 收集「放得下的 sub-row」索引 ---------- */
    std::vector<size_t> candidates;
    for (size_t i = 0; i < subRows.size(); ++i)
        if (cell.width <= subRows[i].freeWidth)
            candidates.emplace_back(i);

    /* ---------- 2) Lambda：計算 cell 到該 sub-row 的最小位移 ---------- */
    auto calcDisp = [&](size_t idx) -> double
    {
        const subRow& sr = subRows[idx];
        if (cell.x < sr.m_minX)
            return sr.m_minX - cell.x;
        else if (cell.x + cell.width > sr.m_maxX)
            return (cell.x + cell.width) - sr.m_maxX;
        return 0.0;   // 原本就落在 sub-row 內
    };

    /* ---------- 3) 掃描候選，取 displacement 最小者 ---------- */
    int    bestIdx  = -1;
    double bestDisp = std::numeric_limits<double>::max();

    for (size_t idx : candidates)
    {
        double d = calcDisp(idx);
        if (d < bestDisp)          // 更小就更新
        {
            bestDisp = d;
            bestIdx  = static_cast<int>(idx);
        }
        else                       // displacement 已開始變大 ⇒ 之後也只會更遠
        {
            break;                 // 提前結束，加速
        }
    }
    return bestIdx;                // –1 代表找不到
}

pair<int,double> Legalizer::placeRowTrial(Row& row, Cell& cell, bool addPenalty)
{
    int subRowIdx = getSubRowIdx(row, cell);
    if (subRowIdx == -1) return {-1, DBL_MAX};

    const subRow& sr = row.subRows[subRowIdx];

    /* ---------- 0) 先把 cellX 夾回 sub-row ---------- */
    int cellX = std::max(sr.m_minX,
                 std::min(static_cast<int>(cell.x), sr.m_maxX - cell.width));

    Cluster* cluster = sr.lastCluster;

    /* ---------- 1) 直接放得下 ---------- */
    if (!cluster || cluster->x + cluster->width <= cellX) {
        cell.optX = cellX;
        cell.optY = row.y;
        return {subRowIdx, cell.getCost()};
    }

    /* ---------- 2) 需要 collapse ---------- */
    double clusterWeight = cluster->weight + cell.weight;
    double clusterQ      = cluster->q + cell.weight * cellX;
    int    clusterWidth  = cluster->width + cell.width;

    Cluster* cur = cluster;
    double   clusterX;                         // collapse 後最左邊座標
    std::vector<Cluster*> visited;             // 收集經過的 cluster

    while (true) {
        visited.push_back(cur);                // 紀錄目前 cluster

        /* 2-1) 更新 clusterX（含偏置） */
        double bias = 0.5;
        clusterX = (clusterQ + bias * cell.weight * cellX)
                   / (clusterWeight + bias * cell.weight);

        clusterX = std::max(static_cast<double>(sr.m_minX),
                   std::min(clusterX, static_cast<double>(sr.m_maxX - clusterWidth)));

        /* 2-2) 還會撞到左邊嗎？ */
        Cluster* prev = cur->pre;
        if (prev && prev->x + prev->width > clusterX) {
            clusterQ     += prev->q - prev->weight * clusterWidth;
            clusterWeight += prev->weight;
            clusterWidth  += prev->width;
            cur = prev;                         // 繼續往左 collapse
        } else {
            break;
        }
    }

    /* 2-3) 先給新 cell 一個暫定座標 */
    cell.optX = clusterX + clusterWidth - cell.width;
    cell.optY = row.y;

    /* ---------- 3) penalty 檢查：對整個 collapse 鏈逐顆驗證 ---------- */
    if (addPenalty) {
        int x = row.alignToSite(static_cast<int>(clusterX), false);  // site 對齊
        for (Cluster* cptr : visited) {
            /* 注意：visited 是 collapse 途中由右到左 push 的，
                     跑位移時要保持同樣順序 */
            for (Cell* m : cptr->cells) {
                int   savedX = m->optX;          // 暫存
                m->optX = x;
                if (m->getCost() > maxDisplacement) {
                    m->optX = savedX;            // 還原
                    return {-1, DBL_MAX};        // 這條路失敗
                }
                m->optX = savedX;                // 還原
                x += m->width;
            }
        }
    }

    return {subRowIdx, cell.getCost()};
}


void Legalizer::placeRowFinal(subRow& sr, Cell& cell)
{
    /* ---------- 1) 更新 sub-row 空間 ---------- */
    sr.freeWidth -= cell.width;

    /* ---------- 2) 把 cell 的 X 落在合法區 ---------- */
    int cellX = static_cast<int>(cell.x);
    if (cellX < sr.m_minX)                     cellX = sr.m_minX;
    else if (cellX > sr.m_maxX - cell.width)   cellX = sr.m_maxX - cell.width;

    /* ---------- 3) 取得當前最後一個 cluster ---------- */
    Cluster* cluster = sr.lastCluster;

    /* ---------- 3-A) 右側有空 → 新建 cluster ---------- */
    if (!cluster || cluster->x + cluster->width <= cellX) {

        cluster = new Cluster(cellX, cluster);   // pre = 原本最後一顆
        sr.lastCluster = cluster;                // 更新鏈尾

        /* 把 cell 放進 cluster */
        cluster->cells.push_back(&cell);          // 這裡是「值拷貝」；若要共享請改成指標
        cluster->width  += cell.width;
        cluster->weight += cell.weight;
        cluster->q      += cell.weight * (cellX - cluster->width);

        return;                                  // 不需 collapse，直接結束
    }

    /* ---------- 3-B) 與現有 cluster 重疊 → 黏進去 ---------- */
    cluster->cells.push_back(&cell);
    cluster->width  += cell.width;
    cluster->weight += cell.weight;
    cluster->q      += cell.weight * (cellX - cluster->width);

    /* ---------- 4) Collapse 向左合併 ---------- */
    while (true) {
        /* 4-1) 重新計算 cluster.x 並夾回 sub-row */
        cluster->x = cluster->q / cluster->weight;
        if (cluster->x < sr.m_minX)                       cluster->x = sr.m_minX;
        if (cluster->x > sr.m_maxX - cluster->width)      cluster->x = sr.m_maxX - cluster->width;

        /* 4-2) 檢查是否壓到前一個 cluster */
        Cluster* prev = cluster->pre;
        if (prev && prev->x + prev->width > cluster->x) {
            /* 把 prev 也併進來 */
            prev->cells.insert(prev->cells.end(),
                               cluster->cells.begin(), cluster->cells.end());
            prev->q      += cluster->q - cluster->weight * prev->width;
            prev->width  += cluster->width;
            prev->weight += cluster->weight;

            cluster = prev;        // 繼續往更左邊檢查
        } else {
            break;                 // 不再重疊，停止 collapse
        }
    }

    /* ---------- 5) 更新 sub-row 最後一顆 cluster ---------- */
    sr.lastCluster = cluster;
}

void Legalizer::abacus() {
    // 按x座標排序
    sort(cellList.begin(), cellList.end(), [](const Cell& a, const Cell& b) {
        return a.x < b.x;
    });

    for (auto& cell : cellList) {
        int baseRowIdx = getRowIdx(cell);
        int bestRowIdx = -1, bestSubRowIdx = -1;
        double bestCost = DBL_MAX;

        // 擴大搜索範圍，先不用penalty
        for (int addPenalty = 1; addPenalty >= 0; --addPenalty) {
            // 向上搜索
            for (int rowIdx = baseRowIdx; rowIdx >= 0; --rowIdx) {
                double yCost = abs(cell.y - rowList[rowIdx].y);
                if (yCost >= bestCost) break;

                auto [subRowIdx, cost] = placeRowTrial(rowList[rowIdx], cell, addPenalty);
                if (subRowIdx != -1 && cost < bestCost) {
                    bestCost = cost;
                    bestRowIdx = rowIdx;
                    bestSubRowIdx = subRowIdx;
                    // 如果找到很好的解就早停
                    if (cost < 100) break;  
                }
            }

            // 向下搜索
            for (size_t rowIdx = baseRowIdx + 1; rowIdx < rowList.size(); ++rowIdx) {
                double yCost = abs(cell.y - rowList[rowIdx].y);
                if (yCost >= bestCost) break;

                auto [subRowIdx, cost] = placeRowTrial(rowList[rowIdx], cell, addPenalty);
                if (subRowIdx != -1 && cost < bestCost) {
                    bestCost = cost;
                    bestRowIdx = rowIdx;
                    bestSubRowIdx = subRowIdx;
                    if (cost < 100) break;
                }
            }

            if (bestSubRowIdx != -1) break;  // 找到解就跳出
        }

        if (bestRowIdx == -1 || bestSubRowIdx == -1) {
            cerr << "[Error] Cell " << cell.name << " cannot be placed!" << endl;
            // 強制放到最近的row
            cell.optX = cell.x;
            cell.optY = rowList[baseRowIdx].y;
        } else {
            placeRowFinal(rowList[bestRowIdx].subRows[bestSubRowIdx], cell);
        }
    }
}


void Legalizer::determinePosition() {
    for (Row& row : rowList) {
        for (subRow& sr : row.subRows) {
            Cluster* cluster = sr.lastCluster;
            while (cluster) {
                int x = row.alignToSite(cluster->x, false);
                
                for (Cell* cell : cluster->cells) {
                    cell->optX = x;
                    cell->optY = row.y;
                    x += cell->width;
                }
                cluster = cluster->pre;
            }
        }
    }
}

