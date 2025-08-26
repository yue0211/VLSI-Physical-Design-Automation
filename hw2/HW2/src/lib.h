#ifndef LIB_H
#define LIB_H

#include <string>
#include <vector>
#include <list>
#include <set>

using namespace std;

/************************************
 * LibraryCell: 
 *   用來記錄某技術(Technology)底下的
 *   一個標準元件 (LibCell) 的寬、高
 ************************************/
struct LibraryCell {
    string name;  // 例如 "MC1"
    double width;
    double height;
    LibraryCell(const string &n, double w, double h)
        : name(n), width(w), height(h) {}
};

/************************************
 * Die:
 *   用於記錄一個 Die 的資訊：
 *   1. 它是用哪一種 Technology
 *   2. Die 的寬、高 (可推算面積)
 *   3. Die 的最大利用率(百分比)
 ************************************/
struct Die {
    string techName;  // 例如 "TA" or "TB"
    double width;
    double height;
    double util;           // 如 80 表示 80%
    
    // 輔助函式，回傳 die 的最大可用面積
    double maxUsableArea() const {
        return width * height * (util / 100.0);
    }
    // 回傳 die 本身的總面積
    double area() const {
        return width * height;
    }
    
    Die() : width(0), height(0), util(100.0) {}
    Die(const string &tech, double w, double h, double u)
        : techName(tech), width(w), height(h), util(u) {}
};


class Cell {
    public:
        string name;         // 例如 "C1"
        string libCellName;  // 例如 "MC1"
    
        // 放在 DieA / DieB 時對應的面積
        double areaA;             
        double areaB;             
    
        // FM 演算法需要的欄位
        bool partition;  // 0 => DieA, 1 => DieB
        int gain;
        bool lock;
    
        // 連接的 net 名稱
        vector<string> nets;
    
        // 在 bucket list 裏存放的位置 (iterator)
        // 當前這個 cell 位於哪個 gain bucket，就可以透過 it 直接移除或移動
        list<Cell*>::iterator it;
    
        // 建構子
        Cell(const string &n, const string &lib)
            : name(n), libCellName(lib),
              areaA(0), areaB(0),
              partition(false), gain(0), lock(false) {}
};

class Net {
    public:
        string name;    
        int weight;          // 新的 spec 裏 net 有 weight
        int cntBucket[2];    // partition0/1 裏有多少 cell
        bool lock[2];        // FM 更新中使用
        vector<string> cells; // net下所有 cell 名稱
    
        Net(const string &n, int w)
            : name(n), weight(w)
        {
            cntBucket[0] = cntBucket[1] = 0;
            lock[0] = lock[1] = false;
        }
};



class Bucket {
    public:
        vector<list<Cell*>> bucketList;
        set<string> bucketSet; // 存放屬於這個分割區域的所有 cell 名稱
        
        int maxIndex; // 記錄目前 bucket 中“最大 index”的位置，以方便快速找到「gain 最大」的 cell。
        string name;  // 用來記錄是哪一個 partition
        double size;  // 此 bucket 中的所有 cell 大小總和
        int cnt;      // 目前這個 partition 包含了多少個 cell
    
    public:
        // 建構子：給定 Pmax，就開好 2*Pmax+1 個桶
        Bucket(string name, int Pmax) {
            name = name;
            bucketList.resize(2 * Pmax + 1);
            maxIndex = 0;
            cnt = 0;
            size = 0;
        }
    
        
};

#endif // LIB_H