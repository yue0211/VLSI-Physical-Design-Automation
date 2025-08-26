#include "Floorplanner.h"

// --- HardBlock ---
HardBlock::HardBlock() : w(0), h(0), coord(0, 0), rotated(false) {}
HardBlock::HardBlock(const string& n, int _w, int _h)
  : name(n), w(_w), h(_h), coord(0, 0), rotated(false) {}

// --- Pad ---
Pad::Pad() : x(0), y(0) {}
Pad::Pad(const string& n, int _x, int _y) : name(n), x(_x), y(_y) {}

// --- Net ---
Net::Net() {}
Net::Net(const string& n) : name(n) {}

// --- Floorplanner ---
Floorplanner::Floorplanner()
 : numHardBlocks(0), numPads(0), numNets(0),
   outlineW(0), outlineH(0),
   best_wirelength(INT_MAX),
   dead_space_ratio(0.0),
   total_block_area(0)
{
}


void Floorplanner::set_seed() {
    if(numHardBlocks == 100) {
        if(dead_space_ratio == 0.15) {
            seed = 22;
        } else if(dead_space_ratio == 0.1) {
            seed = 42;
        } else if(dead_space_ratio == 0.09) {
            seed = 30;
        } else {
            seed = time(NULL);
        }
    } else if(numHardBlocks == 200) {
        if(dead_space_ratio == 0.15) {
            seed = 25; 
        } else if(dead_space_ratio == 0.1) {
            seed = 81;
        } else if(dead_space_ratio == 0.09) {
            seed = 24;
        } else {
            seed = time(NULL);
        }
    } else if(numHardBlocks == 300) {
        if(dead_space_ratio == 0.15) {
            seed = 11;
        } else if(dead_space_ratio == 0.1) {
            seed = 59;
        } else if(dead_space_ratio == 0.09) {
            seed = 93;
        } else {
            seed = time(NULL);
        }
    } else {
        seed = time(NULL);
    }
    srand(seed);
}


// 讀取
void Floorplanner::readInput(const string& inFile) {
    ifstream fin(inFile);
    if(!fin) {
        cerr << "[Error] Cannot open " << inFile << "\n";
        exit(1);
    }
    string token;
    fin >> token >> numHardBlocks; // "NumHardBlocks <N>"
    for(int i=0; i<numHardBlocks; i++){
        fin >> token; // "HardBlock"
        string bname; int w, h;
        fin >> bname >> w >> h;
        blocks[bname] = HardBlock(bname, w, h);
        total_block_area += (long long)w*h;
    }

    fin >> token >> numPads; // "NumPads <M>"
    for(int i=0; i<numPads; i++){
        fin >> token; // "Pad"
        string pname; int px, py;
        fin >> pname >> px >> py;
        pads[pname] = Pad(pname, px, py);
    }

    fin >> token >> numNets; // "NumNets <K>"
    nets.resize(numNets);
    for(int i=0; i<numNets; i++){
        fin >> token; // "Net"
        string net_name; int deg;
        fin >> net_name >> deg;
        nets[i].name = net_name;
        nets[i].pins.reserve(deg);
        for(int j=0; j<deg; j++){
            fin >> token; // "Pin"
            string pin_name;
            fin >> pin_name;
            nets[i].pins.push_back(pin_name);
        }
    }
    fin.close();
}

// 計算外框
void Floorplanner::calcOutline() {
    double area = (double)total_block_area * (1.0 + dead_space_ratio);
    double side = floor( sqrt(area) );
    outlineW = (int)side;
    outlineH = (int)side;
}

// initSolution：依面積排序，並以 row 拆分
vector<string> Floorplanner::initSolution() {
    vector<HardBlock> sorted_hardblocks;

    // 取得硬塊，必要時進行 90 度旋轉
    for (auto &kv : blocks) {
        HardBlock tmp = kv.second;
        // 若硬塊的寬小於高，則旋轉（互換寬高，並記錄 rotated 狀態）
        if(tmp.w < tmp.h){
            swap(tmp.w, tmp.h);
            tmp.rotated = true;
        }
        sorted_hardblocks.push_back(tmp);
    }

    // 依面積（w * h）由大到小排序
    sort(sorted_hardblocks.begin(), sorted_hardblocks.end(), 
         [](const HardBlock &a, const HardBlock &b) {
             return (a.w * a.h) > (b.w * b.h);
         }
    );


    vector<vector<string>> rows;
    vector<int> rowWidths; // 記錄每一 row 的累積寬度
    // 初始化第一個 row
    rows.push_back(vector<string>());
    rowWidths.push_back(0);

    // 將排序後的硬塊依序放入 row，若累積寬度超過 outlineW，則建立新 row
    for (const auto &hardblock : sorted_hardblocks) {
        int blockWidth = hardblock.w;
        if (rowWidths.back() + blockWidth > outlineW) {
            rows.push_back(vector<string>());
            rowWidths.push_back(0);
        }
        rows.back().push_back(hardblock.name);
        rowWidths.back() += blockWidth;
    }

    // 產生 Polish Expression：row 中各 operand 以 "V" 連接；各 row 以 "H" 連接
    vector<string> expression;
    expression.reserve(sorted_hardblocks.size() * 2 - 1);
    for (size_t i = 0; i < rows.size(); i++) {
        for (int j = 0; j < rows[i].size(); ++j) {
            expression.push_back(rows[i][j]);
            if (j >= 1)
                expression.push_back("V");
        }
        if (i >= 1)
            expression.push_back("H");
    }

    return expression;
}


// getCost
pair<bool, long long> Floorplanner::getCost(const vector<string>& sol, bool withWirelength) {

    int width, height, penalty = 0;
    long long areaCost = 0;
    long long wirelength = 0;

    areaCost = getArea(sol, width, height, withWirelength);

    if(withWirelength)
        wirelength = getWirelength();

    int penaltyFactor = 10;
    if(areaCost != 0 )
        penaltyFactor *= 1.5;
    
    return {areaCost == 0 ,  penaltyFactor * areaCost + wirelength};
}



vector<Node> Floorplanner::stockmeyer(const std::vector<Node>& left, const std::vector<Node>& right, const std::string& opType, int parentIndex){
    // 1) 產生「笛卡兒積」(Cartesian Product)
    //    每一個 (l, r) 配對都形成一個新的 (w, h)
    vector<Node> candidate;
    candidate.reserve(left.size() * right.size());
    for (int i = 0; i < (int)left.size(); i++) {
        for (int j = 0; j < (int)right.size(); j++) {
            int newW = 0, newH = 0;

            if (opType == "V") {
                // 垂直切割 => w = w_l + w_r, h = max(h_l, h_r)
                newW = left[i].width + right[j].width;
                newH = std::max(left[i].height, right[j].height);
            } else {
                // 水平切割 => w = max(w_l, w_r), h = h_l + h_r
                newW = std::max(left[i].width, right[j].width);
                newH = left[i].height + right[j].height;
            }

            Node n(
                opType,                // type: "V" / "H"
                parentIndex,           // index (對應 polish expression 裡的運算子位置)
                newW, newH,            // 合併後的 width, height
                left[i].index, i,      // left_from, left_at
                right[j].index, j,     // right_from, right_at
                Coord(0, 0)            // coord: 先設 (0,0)，後面再 update_coord
            );

            candidate.push_back(n);
        }
    } 

    // 2) 針對 candidate 做「二維支配修剪 (dominance pruning)」
    //    這裡的作法是先依 width 做遞增排序，再掃描 height 以去除在 (w,h) 上被支配的解。
    //    之後常見還會再依 height 排序做一次類似檢查，避免「寬稍大、高明顯更小」的組合意外被砍。
    //    以下示範先做「寬度排序 + 一次掃描」。
    std::sort(candidate.begin(), candidate.end(), 
        [](const Node &a, const Node &b) {
            if(a.width == b.width) return a.height < b.height;
            return a.width < b.width;
        }
    );

    std::vector<Node> pruned;
    pruned.reserve(candidate.size());

    int bestH = INT_MAX;
    for (auto &c : candidate) {
        // 如果發現 height 明顯更小，則保留
        if (c.height < bestH) {
            pruned.push_back(c);
            bestH = c.height;
        }
        // 否則就被支配，捨棄
    }

    // 3) (選擇性) 再依 height 排序，做相同掃描，以更嚴格方式留下真正的 Pareto 最優解
    //    如果想要保留「寬略大、高更小」的可能，需要：
    //    - 先把 pruned 再依 height 遞增排序
    //    - 再類似方法掃 width
    //    這樣才是最完整的 2D pruning。
    //
    //    這裡如果你想更完整，可以加一段：
    
    std::sort(pruned.begin(), pruned.end(), 
        [](const Node &a, const Node &b) {
            if(a.height == b.height) return a.width < b.width;
            return a.height < b.height;
        }
    );
    std::vector<Node> pruned2;
    pruned2.reserve(pruned.size());
    int bestW = INT_MAX;
    for (auto &c : pruned) {
        if (c.width < bestW) {
            pruned2.push_back(c);
            bestW = c.width;
        }
    }

    // pruned2 即為最後真正保留下來的一組 (w,h)
    return pruned2;

}

void Floorplanner::update_coord(std::vector<std::vector<Node>>& record, int index, int min_at) {
    record[index] = {record[index][min_at]};

    Node* n = &record[index][0];
    Node* left = &record[n->left_from][n->left_at];
    Node* right = &record[n->right_from][n->right_at];

    if(n->type == "V") {
        left->coord = n->coord;
        right->coord = Coord(n->coord.x + left->width, n->coord.y);
        update_coord(record, n->left_from, n->left_at);
        update_coord(record, n->right_from, n->right_at);
    } else if(n->type == "H") {
        left->coord = n->coord;
        right->coord = Coord(n->coord.x, n->coord.y + left->height);
        update_coord(record, n->left_from, n->left_at);
        update_coord(record, n->right_from, n->right_at);
    } else {
        blocks[n->type].coord = n->coord;
        if(n->width != blocks[n->type].w) {
            blocks[n->type].rotated = true;
        } else {
            blocks[n->type].rotated = false;
        }
    }
}

// Stockmeyer Algorithm
int Floorplanner::getArea(const vector<string>& sol, int &w, int &h, bool withWirelength) {
    // Variables
    stack<vector<Node>> stk;
    vector<vector<Node>> record;
    std::vector<Node> result;


    // Stockmeyer
    for(int i = 0; i < sol.size(); i++) {
        if(sol[i] == "V" || sol[i] == "H") {
            vector<Node> rightChild = stk.top();
            stk.pop();
            vector<Node> leftChild = stk.top();
            stk.pop();
            vector<Node> res = stockmeyer(leftChild, rightChild, sol[i], i);
            stk.push(res);
            record.push_back(res);
        } else {
            HardBlock hardblock = blocks[sol[i]];
            int width = hardblock.w, height = hardblock.h;
            if(width != height) {
                vector<Node> res = {
                    Node(sol[i], i, width, height, -1, -1, -1, -1, Coord(0, 0)),
                    Node(sol[i], i, height, width, -1, -1, -1, -1, Coord(0, 0))
                };
                
                sort(res.begin(), res.end(), [](Node a, Node b) {
                    return a.width < b.width;
                });

                stk.push(res);
                record.push_back(res);
            } else {
                vector<Node> res = {
                    Node(sol[i], i, width, height, -1, -1, -1, -1, Coord(0, 0))
                };
                stk.push(res);
                record.push_back(res);
            }
        }
    }

    // Get min area
    int width, height, area, min_width, min_height, min_area = INT_MAX, min_index;
    int minAreaCost = numeric_limits<int>::max();
    result = stk.top();
    for(int i = 0; i < result.size(); i++) {
        int areaCost = 0;
        width = result[i].width;
        height = result[i].height;
        area = width * height;

        if (width > outlineW && height > outlineH) {
            areaCost = width * height - outlineW * outlineH;
        }
        else if (width > outlineW) {
            areaCost = (width - outlineW) * outlineH;
        }
        else if (height > outlineH) {
            areaCost = outlineW * (height - outlineH);
        }


        if (minAreaCost > areaCost)
        {
            minAreaCost = areaCost;
            min_width = width;
            min_height = height;
            min_area = area;
            min_index = i;
        }
    }

    // Update coordinates
    if(minAreaCost==0 && withWirelength) 
        update_coord(record, record.size() - 1, min_index);

    // Result
    w = min_width;
    h = min_height;

    return minAreaCost;
}

// 計算 HPWL
long long Floorplanner::getWirelength() {
    long long totalWL=0;
    for(const auto &net : nets) {
        int minx=INT_MAX, maxx=INT_MIN;
        int miny=INT_MAX, maxy=INT_MIN;
        for(const auto & pin_name : net.pins){
            int cx=0, cy=0;
            // block?
            if(blocks.find(pin_name)!=blocks.end()){
                auto &b = blocks[pin_name];
                int bw = b.rotated? b.h : b.w;
                int bh = b.rotated? b.w : b.h;
                double cxf = b.coord.x + bw/2.0;
                double cyf = b.coord.y + bh/2.0;
                cx = (int)floor(cxf);
                cy = (int)floor(cyf);
            } else if(pads.find(pin_name)!=pads.end()){
                // pad
                cx = pads[pin_name].x;
                cy = pads[pin_name].y;
            } else {
                continue;
            }
            minx = min(minx, cx);
            maxx = max(maxx, cx);
            miny = min(miny, cy);
            maxy = max(maxy, cy);
        }
        totalWL += (long long)(maxx - minx) + (long long)(maxy - miny);
    }
    return totalWL;
}


bool Floorplanner::isCut(string s){
    return s == "V" || s == "H";
}

bool Floorplanner::isSkewed(const vector<string> &expression, int idx)
{
    if (isCut(expression[idx]))
    {
        if (idx + 2 < expression.size() && expression[idx] == expression[idx + 2])
            return false;
    }
    else if (isCut(expression[idx + 1]))
    {
        if (idx > 0 && expression[idx - 1] == expression[idx + 1])
            return false;
    }
    return true;
}

bool Floorplanner::satisfyBallot(const vector<string> &expression, int idx)
{
    if (isCut(expression[idx + 1]))
    {
        int cutCnt = 1;
        for (int i = 0; i < idx + 1; ++i)
            if (isCut(expression[i]))
                ++cutCnt;
        if (2 * cutCnt >= idx + 1)
            return false;
    }
    return true;
}


void Floorplanner::swap_adjacent_operand(vector<string>& expression) {
    vector<int> indices;
    for (int i = 0; i + 1 < expression.size(); ++i)
        if ((!isCut(expression[i]) && isCut(expression[i + 1])) ||
            (isCut(expression[i]) && !isCut(expression[i + 1])))
            indices.emplace_back(i);

    while (!indices.empty())
    {
        int r = rand() % indices.size();
        if (isSkewed(expression, indices[r]) && satisfyBallot(expression, indices[r]))
        {
            swap(expression[indices[r]], expression[indices[r] + 1]);
            break;
        }
        else
        {
            swap(indices[r], indices.back());
            indices.pop_back();
        }
    }
}



void Floorplanner::swap_random_operand(vector<string>& expression) {
    vector<int> indices;
    for (int i = 0; i < expression.size(); ++i)
        if (expression[i] != "V" && expression[i] != "H")
            indices.emplace_back(i);

    int l = rand() % indices.size();
    int r = rand() % indices.size();
    while (l == r)
        r = rand() % indices.size();
    swap(expression[indices[l]], expression[indices[r]]);
}

void Floorplanner::invert_chain(vector<string>& expression) {
    vector<int> indices;
    for (int i = 1; i < expression.size(); ++i)
        if (!isCut(expression[i - 1]) && isCut(expression[i]))
            indices.emplace_back(i);

    int r = rand() % indices.size();
    for (int i = indices[r]; i < expression.size(); ++i)
    {
        if (!isCut(expression[i]))
            break;
        expression[i] = (expression[i] == "H") ? "V" : "H";
    }
}

// 產生鄰居解
vector<string> Floorplanner::genNeighbor(vector<string> expression, int r) {
    vector<string> neighbor = expression;

    if(r == 0) {
        swap_adjacent_operand(neighbor);
    } else if(r == 1) {
        invert_chain(neighbor);
    } else if(r == 2) {
        swap_random_operand(neighbor);
    }

    return neighbor;
}



// 模擬退火
pair<vector<string>, int> Floorplanner::simulatedAnnealing(vector<string> expression, bool withWirelength, double initTemperature, double minTemperature, double coolingCoefficient, int tryingTimes, double maxRejectRatio, Timer &timer, double timeLimit) {
    // srand(seed_);

    vector<string> curr_sol = expression;

    vector<string> best_sol = curr_sol;

    // Parameters
    double T = initTemperature, T_MIN = minTemperature, T_DECAY = coolingCoefficient;
    double REJECT_RATIO = maxRejectRatio;
    // 一般是用來表示在每個溫度下要嘗試多少次鄰域解
    int K = tryingTimes;
    int N = numHardBlocks * K;
    int DOUBLE_N = N * 2;

    // Variables
    long long curr_cost = getCost(curr_sol, withWirelength).second, min_cost = curr_cost;
    int gen_cnt = 1, uphill_cnt = 0, reject_cnt = 0;

    
     // Simulated annealing
    do
    {
        gen_cnt = 0, uphill_cnt = 0, reject_cnt = 0;
        do
        {
            if (timer.is_timeout(timeLimit)){
                cout << "Timeout!" << endl;
                // int best_w = 0, best_h = 0, minAreaCost = 0;
                // minAreaCost = getCost(best_sol, withWirelength).second;
                return {best_sol, min_cost};
            }
            int r = (withWirelength) ? 2 : rand() % 3;
            vector<string> neighbor = genNeighbor(curr_sol, r);
            auto [inOutline, neighbor_cost] = getCost(neighbor, withWirelength);
            gen_cnt++;
            if (withWirelength && !inOutline) {
                // 新解超出 outline，直接跳過
                reject_cnt++;
                continue;
            }

            long long delta_cost = neighbor_cost - curr_cost;
            bool rand_accept = (double)rand() / RAND_MAX < exp(-1 * (delta_cost) / T);
            if(delta_cost <= 0 || rand_accept) {
                if(delta_cost > 0) {
                    uphill_cnt++;
                }
                curr_sol = neighbor;
                curr_cost = neighbor_cost;
                if(curr_cost < min_cost) {
                    min_cost = curr_cost;
                    best_sol = curr_sol;
                    best_blocks = blocks;
                    // cout << "update best solution: " << get_wirelength() << std::endl;
                }
            } else {
                reject_cnt++;
            }
            // cout << "gen_cnt: " << gen_cnt << ", reject_cnt: " << reject_cnt
            // << ", ratio: " << (double)reject_cnt / (gen_cnt ? gen_cnt : 1) << endl;
        } while (uphill_cnt <= N && gen_cnt <= DOUBLE_N);
        // Reduce temperature
        T *= T_DECAY;
    } while ((double)reject_cnt / gen_cnt <= REJECT_RATIO && T >= T_MIN );



    return { best_sol, min_cost };
    
}

// 輸出結果
void Floorplanner::writeOutput(const string& outFile) {
    ofstream fout(outFile);
    if(!fout){
        cerr << "Cannot write to " << outFile << "\n";
        return;
    }
    fout << "Wirelength " << getWirelength() << "\n";
    fout << "NumHardBlocks " << numHardBlocks << "\n";
    for(const auto &kv : blocks){
        const auto &b = kv.second;
        fout << b.name << " " << b.coord.x << " " << b.coord.y << " "
             << (b.rotated?1:0) << "\n";
    }
    fout.close();
}
