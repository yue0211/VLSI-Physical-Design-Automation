#include "Floorplanner.h"

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc < 4){
        cerr << "Usage: " << argv[0] << " <input.txt> <output.out> <dead_space_ratio>\n";
        return 1;
    }

    string inFile  = argv[1];
    string outFile = argv[2];
    double ds_ratio = atof(argv[3]);

    // 建立一個 Floorplanner 物件
    Floorplanner fp;
    fp.dead_space_ratio = ds_ratio;

    Timer timer;
    timer.start();
    // can set the random seed for different testcases

    // 1) 讀檔
    fp.readInput(inFile);

    fp.set_seed();

    // 2) 計算 Fixed Outline
    fp.calcOutline();

    // 第一階段 SA
    vector<string> expression = fp.initSolution();
    // for(auto & s : expression){
    //     cout << s << " ";
    // }
    cout << endl;
    const double AREA_TIME_LIMIT = 400.0;
    const double TOTAL_TIME_LIMIT = 580.0;
    long long cost = fp.getCost(expression, false).second;
    cout << "Initial cost: " << cost << endl;
    // 3) 執行模擬退火
    cout << "---------- SA FOR AREA ----------\n";
    int iter = 0;
    while (cost != 0 && !timer.is_timeout(AREA_TIME_LIMIT))
    {  
        // tie(expression, cost) = fp.simulatedAnnealing(expression, false, 15000, 0.1, 0.9, 10, 1, timer); // second
        // tie(expression, cost) = fp.simulatedAnnealing(expression, false, 15000, 0.1, 0.9, 10, 1, timer); // best
        tie(expression, cost) = fp.simulatedAnnealing(expression, false, 20000, 0.1, 0.9, 10, 1, timer, AREA_TIME_LIMIT); // for public 3
        printf("Iteration %2d - area cost: %d\n", ++iter, cost);
    }

    if (cost != 0)
    {
        cout << "No feasible solution is found!\n";
    }else
    {
        cout << "Area cost: " << cost << endl;
    }

    
    int wirelength = fp.getWirelength();
    cout << "A feasible solution is found!\n"
                << "Wirelength: " << wirelength << "\n"
                << "\n";

    double used = timer.get_elapsed_time();
    double remaining = TOTAL_TIME_LIMIT - used;
    timer.start();
    fp.best_blocks = fp.blocks;
    cout << "------- SA FOR WIRELENGTH -------\n";
    // tie(expression, cost) = fp.simulatedAnnealing(expression, true, 1000, 1, 0.95, 5, 1, timer); // second
    // tie(expression, cost) = fp.simulatedAnnealing(expression, true, 1000, 1, 0.95, 10, 1, timer); // best
    tie(expression, cost) = fp.simulatedAnnealing(expression, true, 1000, 1, 0.95, 10, 1, timer, remaining); //  for public 3
    fp.blocks = fp.best_blocks;
    wirelength = fp.getWirelength();
    cout << "A minimum wirelength solution is found!\n"
                << "Wirelength: " << wirelength << "\n"
                << "\n";



    // 4) 輸出
    fp.writeOutput(outFile);

}
