#include <bits/stdc++.h>
#include "lib.h"  
using namespace std;

// /******************************************************
//   預先計算 Cell 在 DieA/DieB 時的面積
// ******************************************************/
void computeCellAreas(map<string, Cell*> &cellMap, map<string, vector<LibraryCell>> &techLibCells, Die &dieA, Die &dieB){
    // dieA.techName 例如 "TA"
    // 在 techLibCells["TA"] 裏面找對應的 library cell
    // 假設每顆 Cell 都會 match 一個 library cellName
    // 找到後 areaA = width * height (for TA)
    // dieB 同理
    for(auto & kv : cellMap){
        Cell* c = kv.second;
        // 找出在 dieA.techName 之下, libCellName = c->libCellName 對應的寬高
        // 這裡簡單線性搜尋, 亦可用 map 做加速
        double wA = 0, hA = 0;
        for(const auto &libC : techLibCells[dieA.techName]){
            if(libC.name == c->libCellName){
                wA = libC.width;
                hA = libC.height;
                break;
            }
        }
        c->areaA = wA * hA;

        double wB = 0, hB = 0;
        for(const auto &libC : techLibCells[dieB.techName]){
            if(libC.name == c->libCellName){
                wB = libC.width;
                hB = libC.height;
                break;
            }
        }
        c->areaB = wB * hB;
    }
}



Cell* cellSelect(vector<Bucket> &buckets, double maxAreaA, double maxAreaB) {
    int indexA = buckets[0].maxIndex;
    int indexB = buckets[1].maxIndex;
    bool tie = false;
    list<Cell*>::iterator it;
    
    // 當至少有一邊還有候選的 bucket 時
    while (indexA >= 0 || indexB >= 0) {    
        tie = false;
        // 若兩邊最高 gain 的 bucket 索引相同，則以目前總面積較大的 partition 為優先
        if (indexA == indexB && buckets[0].size >= buckets[1].size)
            tie = true;
        
        if (indexA > indexB || tie) {
            // 從 A partition (buckets[0]) 的 bucketList 中選取候選 Cell
            for (it = buckets[0].bucketList[indexA].begin(); it != buckets[0].bucketList[indexA].end(); ++it) {
                // 模擬將此 Cell 從 A 移到 B 後的面積更新
                double newAreaA = buckets[0].size - (*it)->areaA;
                double newAreaB = buckets[1].size + (*it)->areaB;
                // 檢查更新後兩邊是否都未超過各自的最大允許面積
                if (newAreaA <= maxAreaA && newAreaB <= maxAreaB) {
                    Cell* target = *it;
                    it = buckets[0].bucketList[indexA].erase(it);
                    return target;
                }    
            }
            --indexA; // 當前 bucket 中沒有合適的候選，降低 gain 索引再試
        }
        else {
            // 從 B partition (buckets[1]) 的 bucketList 中選取候選 Cell
            for (it = buckets[1].bucketList[indexB].begin(); it != buckets[1].bucketList[indexB].end(); ++it) {
                double newAreaB = buckets[1].size - (*it)->areaB;
                double newAreaA = buckets[0].size + (*it)->areaA;
                if (newAreaA <= maxAreaA && newAreaB <= maxAreaB) {
                    Cell* target = *it;
                    it = buckets[1].bucketList[indexB].erase(it);
                    return target;
                }    
            }
            --indexB;
        }
    }
    return NULL; // 若無任何 Cell 符合條件則回傳 NULL
}





void initGainAndBuckets(vector<Net> &nets, map<string, Cell*> &cellMap, vector<Bucket> &buckets,int &cutSize, int &Pmax)
{
    //init gain
    for (auto &n : nets) {
        string cellA, cellB; //the critical cell
        for (auto &c : n.cells) {
            if (!cellMap[c]->partition) {
                ++n.cntBucket[0];
                cellA = c;
            }
            else {
                ++n.cntBucket[1];
                cellB = c;
            }
        }
        
        // 先把 bool 狀態弄出來，使條件判斷更直覺
        bool isCut = (n.cntBucket[0] > 0 && n.cntBucket[1] > 0);
        bool allInA = (n.cntBucket[0] == n.cells.size());
        bool allInB = (n.cntBucket[1] == n.cells.size());

        // 1) 被切割 (isCut)
        if (isCut) {
            cutSize += n.weight; // 只要 A,B 都有 Cell，就算被切割

            // 如果 A 邊只有 1 顆 Cell，且 B 邊還有 Cell，代表那顆 A Cell 是「critical」
            if (n.cntBucket[0] == 1 && n.cntBucket[1] > 0) {
                // cellA 為那唯一 A 邊 cell
                cellMap[cellA]->gain += n.weight;
            }
            // 如果 B 邊只有 1 顆 Cell，且 A 邊還有 Cell，代表那顆 B Cell 是「critical」
            if (n.cntBucket[1] == 1 && n.cntBucket[0] > 0) {
                // cellB 為那唯一 B 邊 cell
                cellMap[cellB]->gain += n.weight;
            }
        }
        // 2) 未被切割 (uncut) -> 全部都在 A 或全部都在 B
        else {
            // 如果全部都在 A，且 A 邊 Cell 數 > 1
            if (allInA && n.cntBucket[0] > 1) {
                // 任意搬走一顆都會導致被切割 => 對整個 net 的所有 Cell 做 gain--
                for (auto &c : n.cells) {
                    cellMap[c]->gain -= n.weight;
                }
            }
            // 如果全部都在 B，且 B 邊 Cell 數 > 1
            else if (allInB && n.cntBucket[1] > 1) {
                for (auto &c : n.cells) {
                    cellMap[c]->gain -= n.weight;
                }
            }
        }
    }

    //init buckets
    for (auto &c : cellMap) {
        int i = c.second->gain + Pmax;
        if (!c.second->partition) {
            if (i > buckets[0].maxIndex)
                buckets[0].maxIndex = i;
            buckets[0].bucketList[i].push_front(c.second);
            buckets[0].bucketSet.insert(c.second->name);
            c.second->it = buckets[0].bucketList[i].begin();
        }
        else {
            if (i > buckets[1].maxIndex)
                buckets[1].maxIndex = i;
            buckets[1].bucketList[i].push_front(c.second);
            buckets[1].bucketSet.insert(c.second->name);
            c.second->it = buckets[1].bucketList[i].begin();
        }
    }
    return;
}



/******************************************************
  初始化解（與先前類似，但要檢查面積限制）
  這裡示範：先都放在 DieA，然後依序搬到 DieB，
  直到不超過 DieB 利用率限制即可。
******************************************************/
void initSolution(vector<Bucket> &buckets, map<string, Cell*> &cellMap, Die &dieA, Die &dieB){
    // 全部先放在 DieA
    double usedAreaA = 0.0;
    double usedAreaB = 0.0;
    buckets[0].cnt = 0;
    buckets[0].size = 0;
    buckets[1].cnt = 0;
    buckets[1].size = 0;

    for(auto &kv : cellMap){
        Cell* c = kv.second;
        c->partition = false; // false => DieA
        usedAreaA += c->areaA;
        buckets[0].cnt++;
        buckets[0].size += c->areaA;
    }

    // 如果 DieA 的面積超標，搬移部分 cell 到 DieB
    if(usedAreaA > dieA.maxUsableArea()){
        // 將 cell 按 (areaA - areaB) 從大到小排序
        vector<Cell*> allCells;
        for(auto &kv : cellMap){
            allCells.push_back(kv.second);
        }
        sort(allCells.begin(), allCells.end(), 
             [](Cell* a, Cell* b){ return (a->areaA - a->areaB) > (b->areaA - b->areaB); });

        // 遍歷這些 cell，嘗試搬移直到 DieA 的面積滿足限制
        for(auto &c : allCells){
            if(usedAreaA <= dieA.maxUsableArea())
                break;
            // 檢查如果把此 cell 搬到 DieB，
            // DieB 的面積不會超過限制，同時減少 DieA 的面積
            if((usedAreaB + c->areaB) <= dieB.maxUsableArea()){
                // 搬移 cell：從 DieA 到 DieB
                c->partition = true;
                usedAreaA -= c->areaA;
                usedAreaB += c->areaB;
                buckets[0].cnt--;
                buckets[0].size -= c->areaA;
                buckets[1].cnt++;
                buckets[1].size += c->areaB;
            }
        }
    }
}



void updatePartition(Cell* target, vector<Bucket> &buckets, bool from, int &cutSize)
{
    target->partition = !from;
    target->lock = 1;
    cutSize -= target->gain;
    --buckets[from].cnt;
    ++buckets[!from].cnt;

    if(!from){ // from 是 A
        buckets[from].size -= target->areaA;
        buckets[!from].size += target->areaB;
    }else{ // from 是 B
        buckets[from].size -= target->areaB;
        buckets[!from].size += target->areaA;
    }
    
    return;
}




void updateBucketList(Cell* ptr, vector<Bucket> &buckets, int change, int &Pmax)
{
    int i = ptr->gain + Pmax;
    buckets[ptr->partition].bucketList[i].erase(ptr->it);
    buckets[ptr->partition].bucketList[i+change].push_front(ptr);
    ptr->it = buckets[ptr->partition].bucketList[i+change].begin();
    if (i + change > buckets[ptr->partition].maxIndex)
        buckets[ptr->partition].maxIndex = i + change;
    return;
}

Cell* findCritical(Cell* target, Net &net, map<string, Cell*> &cellMap, bool partition)
{
    for (auto &c : net.cells) {
        if (cellMap[c]->partition == partition)
            if (cellMap[c] != target)
                return cellMap[c];
    }
    return NULL;
}



void updateGain(Cell* target, Net &net, map<string, Cell*> &cellMap, vector<Bucket> &buckets, bool from, int &Pmax)
{
    // check critical nets before the move
    if (net.cntBucket[!from] == 0) {
        for (auto &c : net.cells) {
            if (!cellMap[c]->lock) {
                updateBucketList(cellMap[c], buckets, net.weight, Pmax);
                cellMap[c]->gain += net.weight;
            }
        }
    }
    else if (net.cntBucket[!from] == 1) {
        Cell* critical = findCritical(target, net, cellMap, !from);
        if (!critical->lock) {
            updateBucketList(critical, buckets, -net.weight, Pmax);
            critical->gain -= net.weight;
        }
    }
    --net.cntBucket[from];
    ++net.cntBucket[!from]; 
    // check critical nets after the move
    if (net.cntBucket[from] == 0) {
        for (auto &c : net.cells) {
            if (!cellMap[c]->lock) {
                updateBucketList(cellMap[c], buckets, -net.weight, Pmax);
                cellMap[c]->gain -= net.weight;
            }                    
        }
    }
    else if (net.cntBucket[from] == 1) {
        Cell* critical = findCritical(target, net, cellMap, from);
        if (!critical->lock) {
            updateBucketList(critical, buckets, net.weight, Pmax);
            critical->gain += net.weight;
        }
    }
    return;
}



void updateBucketSet(vector<pair<string, bool> > &move, vector<Bucket> &buckets, int bestPass)
{
    for (int i = 0; i < bestPass; ++i) {
        buckets[move[i].second].bucketSet.erase(move[i].first);
        buckets[!move[i].second].bucketSet.insert(move[i].first);
    }
    return;
}


/******************************************************
  讀取輸入檔並解析
******************************************************/
void parseInput(const string &inFile, map<string, vector<LibraryCell>> &techLibCells, map<string, Cell*> &cellMap, Die &dieA, Die &dieB, vector<Net> &nets) {
    ifstream fin(inFile);
    if(!fin) {
        cerr << "Cannot open input file: " << inFile << endl;
        exit(1);
    }

    // 先定義一些變數，方便暫存
    int numTechs = 0;   // 之後讀 "NumTechs"
    int techLeft = -1;  // 還要解析幾組 "Tech"
    
    // 這兩個參數讀完之後，要再讀 nLib 行的 LibCell
    int nLibLeft = 0;   // 目前這個技術還要讀多少行 LibCell
    
    int numCells = 0;   // "NumCells"
    int cellsLeft = 0;  // 還要讀幾行 "Cell"
    
    int numNets = 0;    // "NumNets"
    int netsLeft = 0;   // 還要讀幾組 Net
    int netCellCount = 0; // 該 net 還要讀多少行 "Cell"
    string currentTechName; 

    // 逐行讀
    string line;
    while(true) {
        if(!getline(fin, line)) {
            // EOF
            break;
        }
        // 跳過空白行
        if(line.empty()) {
            continue;
        }
        // 以 stringstream 切分
        stringstream ss(line);
        // 先讀第一個 token
        string label;
        ss >> label;
        // 若 label 仍是空，跳過
        if(label.empty()) {
            continue;
        }

        // 核心: 判斷 label 是什麼
        // 1) 若還在解析 "LibCell" 行
        if(nLibLeft > 0) {
            // 預期 line 是: "LibCell <libCellName> <w> <h>"
            if(label != "LibCell") {
                cerr << "[Error] Expect 'LibCell' but read: " << label << endl;
                // 依需求要不要直接 exit
                exit(1);
            }
            string libCellName;
            double w, h;
            ss >> libCellName >> w >> h;
            // 放到上一個 tech
            // 目前不知道哪個 tech? => 通常我們剛剛解析到 "Tech" 時就存起 techName
            // 這裡可用某個暫存techName 來知道該放哪裡
            // 這裡僅示意:
            // 假設 parse "Tech ..." 時填了 currentTechName
            techLibCells[currentTechName].emplace_back(libCellName, w, h);
            

            nLibLeft--;
            // 若 nLibLeft == 0，代表該 tech 的 LibCell 都讀完了

            continue; // 處理下一行
        }

        // 2) 若還在解析 "Cell" 行 (從 NumCells 開始)
        if(cellsLeft > 0) {
            // 預期: "Cell <CellName> <LibCellName>"
            if(label != "Cell") {
                cerr << "[Error] Expect 'Cell' but read: " << label << endl;
                exit(1);
            }
            string cName, libName;
            ss >> cName >> libName;
            Cell* newC = new Cell(cName, libName);
            cellMap[cName] = newC;

            cellsLeft--;
            continue;
        }

        // 3) 若正在解析某個 net 的 cell
        if(netCellCount > 0) {
            // 預期: "Cell <cellName>"
            if(label != "Cell") {
                cerr << "[Error] Expect 'Cell' but read: " << label << endl;
                exit(1);
            }
            string tmpCell;
            ss >> tmpCell;

            // 讓 nets.back() 取得這個 cell
            nets.back().cells.push_back(tmpCell);
            cellMap[tmpCell]->nets.push_back(nets.back().name);
            netCellCount--;

            continue;
        }

        // 如果都不是前述狀態，那就看 label 為何:
        if(label == "NumTechs") {
            ss >> numTechs;
            techLeft = numTechs; // 接下來會讀取 numTechs 組 Tech
        }
        else if(label == "Tech") {
            // 格式: "Tech <techName> <nLib>"
            string techName;
            int nLib;
            ss >> techName >> nLib;
            // 把 techLeft-1
            techLeft--;
            // 記錄當前 techName
            
            currentTechName = techName;
            // 之後要讀 nLib 行 "LibCell ..."
            nLibLeft = nLib;  
        }
        else if(label == "DieSize") {
            // 格式: "DieSize <width> <height>"
            double w, h;
            ss >> w >> h;
            dieA.width = w;
            dieA.height = h;
            dieB.width = w;
            dieB.height = h;
        }
        else if(label == "DieA") {
            // 格式: "DieA <techA> <utilA>"
            string techA;
            double utilA;
            ss >> techA >> utilA;
            dieA = Die(techA, dieA.width, dieA.height, utilA);
        }
        else if(label == "DieB") {
            // 同上
            string techB;
            double utilB;
            ss >> techB >> utilB;
            dieB = Die(techB, dieB.width, dieB.height, utilB);
            // 假設 DieB 與 DieA 的寬高相同 (根據 spec)
        }
        else if(label == "NumCells") {
            // 格式: "NumCells <n>"
            ss >> numCells;
            cellsLeft = numCells;
        }
        else if(label == "NumNets") {
            // 格式: "NumNets <m>"
            ss >> numNets;
            netsLeft = numNets;
        }
        else if(label == "Net") {
            // 格式: "Net <netName> <#Cells> <weight>"
            string netName;
            int cCount, weight;
            ss >> netName >> cCount >> weight;
            Net netObj(netName, weight);
            nets.push_back(netObj);

            // 之後要讀 cCount 行 "Cell <cellName>"
            netCellCount = cCount;
            netsLeft--;
        }
        else {
            // 可能遇到不在規格中的行
            // 可以自行決定要不要忽略或直接報錯
            cerr << "[Warning] Unknown label: " << label << endl;
        }
    }

    fin.close();
}


void writeOutput(const string &filename, int minCutSize, const vector<Bucket> &buckets) {
    ofstream fout(filename);
    if (!fout) {
        cerr << "Cannot open output file: " << filename << endl;
        return;
    }
    // 輸出 cut size
    fout << "CutSize " << minCutSize << "\n";
    
    // 輸出 DieA 的結果 (假設 buckets[0] 為 DieA)
    fout << "DieA " << buckets[0].bucketSet.size() << "\n";
    for (const auto &cellName : buckets[0].bucketSet)
        fout << cellName << "\n";
    
    
    // 輸出 DieB 的結果 (假設 buckets[1] 為 DieB)
    fout << "DieB " << buckets[1].bucketSet.size() << "\n";
    for (const auto &cellName : buckets[1].bucketSet)
        fout << cellName << "\n";
    
    fout.close();
}


/** 
 * 在本回合所有移動 (passes) 結束後，
 * 我們已經呼叫  updateBucketSet(move, buckets, bestPass);
 * 使得 buckets[0]/buckets[1] 回到 bestPass 分割。
 * 
 * 這個函式會依照 buckets 裏的分割，重新掃一遍所有的 net，計算 cntBucket[0]/[1]，
 * 並且累計出真正對應這個「best partition」的 cutSize。
 * 
 * 最後把這個 cutSize 回傳給呼叫者 (或用參數傳回)。
 **/
int recalcAfterBestPass(vector<Net> &nets,
                        const map<string, Cell*> &cellMap,
                        const vector<Bucket> &buckets)
{
    // 1) 先把所有 net 的 cntBucket 歸零
    for (auto &net : nets) {
        net.cntBucket[0] = 0;
        net.cntBucket[1] = 0;
    }

    // 2) 掃描 buckets[0]/[1]，看有哪些 cell 屬於 partition A(=0), B(=1)
    //    幫對應 net 的 cntBucket[x]++。
    //    因為 buckets[0].bucketSet 全都是 partition=0 的 cell，
    //    buckets[1].bucketSet 全都是 partition=1 的 cell。
    for (auto &cellName : buckets[0].bucketSet) {
        Cell* cptr = cellMap.at(cellName);
        for (auto &netName : cptr->nets) {
            // netName e.g. "N1", "N2"...
            // 假設你是用 stoi( netName.substr(1) )-1 來當 index
            // 也可用 map<string,int> 來對應 netName => net index
            int idx = stoi(netName.substr(1)) - 1;
            nets[idx].cntBucket[0]++;
        }
    }
    for (auto &cellName : buckets[1].bucketSet) {
        Cell* cptr = cellMap.at(cellName);
        for (auto &netName : cptr->nets) {
            int idx = stoi(netName.substr(1)) - 1;
            nets[idx].cntBucket[1]++;
        }
    }

    // 3) 計算 cutSize
    int newCutSize = 0;
    for (auto &net : nets) {
        // 如果 net 的 cntBucket[0] > 0 且 cntBucket[1] > 0 => net 被切割 => 加 net.weight
        if (net.cntBucket[0] > 0 && net.cntBucket[1] > 0) {
            newCutSize += net.weight;
        }
    }

    return newCutSize;
}



/******************************************************
  主程式示例
******************************************************/
int main(int argc, char *argv[]){
    if(argc < 3){
        cerr << "Usage: " << argv[0] << " <input.txt> <output.out>\n";
        return 1;
    }
    string inFile = argv[1];
    string outFile = argv[2];

    map<string, vector<LibraryCell>> techLibCells; // techName -> list of LibCells
    Die dieA, dieB;
    map<string, Cell*> cellMap;     // cellName -> Cell*
    vector<Net> nets;

    clock_t initTime = clock();
    /*-------Read File----------------------------------------------------------------------------------*/
    parseInput(inFile, techLibCells, cellMap, dieA, dieB, nets);

    cout << "Read input file done.\n";

    cout << "DieA: " << dieA.techName << ", width: " << dieA.width << ", height: " << dieA.height << ", utilization: " << dieA.util << endl;
    cout << "DieB: " << dieB.techName << ", width: " << dieB.width << ", height: " << dieB.height << ", utilization: " << dieB.util << endl;
    for(auto & kv : techLibCells){
        cout << "Tech: " << kv.first  << ", libcell size:" << kv.second.size() << endl;
    }
    
    cout << "NumCells: " << cellMap.size() << endl;
    cout << "NumNets: " << nets.size() << endl;

    /*-------FM Algorithm----------------------------------------------------------------------------------*/
    computeCellAreas(cellMap, techLibCells, dieA, dieB);

    int Pmax = 0; 

    // 走訪所有 Cell
    for (auto &kv : cellMap) {
        Cell* cellPtr = kv.second;  
        int sumWeight = 0;

        // 把該 Cell 連到的每條 net 的權重都加起來
        for (auto &netName : cellPtr->nets) {
            int i = stoi(netName.substr(1)) - 1;
            sumWeight += nets[i].weight;
        }

        // 追蹤最大連接權重和
        if (sumWeight > Pmax) {
            Pmax = sumWeight;
        }
    }

    cout<< "Pmax: " << Pmax << endl << endl;

    vector<Bucket> buckets = {  // 一邊一個 bucket
        Bucket("A", Pmax),
        Bucket("B", Pmax)
    };

    int iteration = 0 , minCutSize = 0, cutSize = 0;
    int partialSum = 0, maxPartialSum = 0;
    clock_t itBegin, itEnd;
    double itTime, totalTime;
    while (iteration == 0 || (maxPartialSum > 0 && totalTime + itTime < 170)) { // 170s 避免超過三分鐘, 因為還要讀寫檔
        itBegin = clock();
        ++iteration;
        partialSum = 0;
        maxPartialSum = 0;
        if (iteration == 1)
            initSolution(buckets, cellMap, dieA, dieB);
        else {
            cutSize = 0;
            double sizeA = 0, sizeB = 0;
            for (auto &c : buckets[0].bucketSet) {
                sizeA += cellMap[c]->areaA;
                cellMap[c]->partition = 0;
            }
            for (auto &c : buckets[1].bucketSet) {
                sizeB += cellMap[c]->areaB;
                cellMap[c]->partition = 1;
            }
            buckets[0].cnt = buckets[0].bucketSet.size();
            buckets[0].size = sizeA;
            buckets[1].cnt = buckets[1].bucketSet.size();
            buckets[1].size = sizeB;
            buckets[0].bucketSet.clear();
            buckets[1].bucketSet.clear();
            for (auto &n : nets) {
                n.cntBucket[0] = 0;
                n.cntBucket[1] = 0;
                n.lock[0] = 0;
                n.lock[1] = 0;
            }
            for (auto &c : cellMap) {
                c.second->gain = 0;
                c.second->lock = 0;
            }
        }

        initGainAndBuckets(nets, cellMap, buckets, cutSize, Pmax);
        cout << "iteration " << iteration << endl;
        cout << "--------init---------" << endl;
        cout << "--cutSize:  " << cutSize << endl;
        cout << "cntA, cntB: " << buckets[0].cnt << ", " << buckets[1].cnt << endl;
        cout << "sizeA, sizeB: " << buckets[0].size << ", " << buckets[1].size << endl << endl;

        int pass = 0, bestPass = 0;
        minCutSize = cutSize;
        
        vector<pair<string, bool> > move;
        Cell* target = cellSelect(buckets, dieA.maxUsableArea(), dieB.maxUsableArea());
        while (target) {
            ++pass;
            partialSum += target->gain;
            bool from = target->partition;
            move.push_back(make_pair(target->name, from));
            updatePartition(target, buckets, from, cutSize);
            if (minCutSize > cutSize) {
                minCutSize = cutSize;
                maxPartialSum = partialSum;
                bestPass = pass;
            }
            for (auto &n : target->nets) {
                int i = stoi(n.substr(1)) - 1;
                if (!(nets[i].lock[0] && nets[i].lock[1]))
                    updateGain(target, nets[i], cellMap, buckets, from, Pmax);
                else {
                    --nets[i].cntBucket[from];
                    ++nets[i].cntBucket[!from];
                }
                nets[i].lock[!from] = 1;
            }
            target = cellSelect(buckets, dieA.maxUsableArea(), dieB.maxUsableArea());
        }

        updateBucketSet(move, buckets, bestPass);
        // 接下來立刻做「真正對應 bestPass」的 net 統計 & cutSize 重算
        int finalCutSize = recalcAfterBestPass(nets, cellMap, buckets);
        // 把 minCutSize 改成這個回溯後的結果
        minCutSize = finalCutSize;

        // 重新計算 buckets[0]/buckets[1] 的總 area (A, B) 只是用來印出來
        double A = 0, B = 0;
        for (auto c : buckets[0].bucketSet) 
            A += cellMap[c]->areaA;
        for (auto c : buckets[1].bucketSet)
            B += cellMap[c]->areaB;

        cout << "--------best---------" << endl;
        cout << "bestPass:  " << bestPass <<  endl;
        cout << "maxPartialSum: " << maxPartialSum << endl;
        cout << "--minCutSize:  " << minCutSize << endl;  // 這裡印的 就是「真正對應 bestPass」的cutSize
        cout << "cntA, cntB: " << buckets[0].bucketSet.size() << " " << buckets[1].bucketSet.size() << endl;
        cout << "sizeA, sizeB: " << A << " " << B << endl;

        itEnd = clock();
        itTime = ((double)(itEnd - itBegin)) / CLOCKS_PER_SEC;
        totalTime = ((double)(itEnd - initTime)) / CLOCKS_PER_SEC;
        cout << "itTime: " << itTime << endl;
        cout << "totalTime: " << totalTime << endl;
        cout << "---------------------" << endl << endl;
    }

    /*---------writefile------------------------------------------------------------------------------------*/
    writeOutput(outFile, minCutSize, buckets);
    cout << "Write output file done.\n";

    /*--------------------------------------------------------------------------------------------*/
    for (auto &c : cellMap)
        delete c.second;
    
}
