#!/bin/bash

# 設定變數
APR_TCL="apr.tcl"
SHA256_SDC="../sdc/sha256.sdc"
LOG_FILE="apr.log"
SUMMARY_RPT="../summary.rpt"
DRC_RPT="../drc.rpt"
TIMING_RPT="../timing.rpt"
CSV_FILE="optimization_results.csv"
BEST_CSV_FILE="best_optimization_result.csv"

# 初始參數
UTILIZATION=0.6
CLOCK_PERIOD=580
MAX_UTILIZATION=0.8
MIN_UTILIZATION=0.6
MAX_CLOCK_PERIOD=690

# 初始化 CSV
echo "Slack,DRC,Chip Area,Wire Length,Utilization,Clock Period" > $CSV_FILE
echo "Slack,DRC,Chip Area,Wire Length,Utilization,Clock Period" > $BEST_CSV_FILE

# 修改 apr.tcl 中的 core utilization
modify_apr_tcl() {
    sed -i -E "s/(floorPlan .* -r [0-9]+ )([0-9]*\.?[0-9]+)/\1$UTILIZATION/" "$APR_TCL"
    echo "Updated core utilization to $UTILIZATION in $APR_TCL"
}

# 修改 sha256.sdc 中的 clock period
modify_clock_period() {
    sed -i -E "s/(create_clock -name \"clk\" -period )([0-9]+\.?[0-9]*)/\1$CURRENT_CLOCK_PERIOD/" "$SHA256_SDC"
    echo "Updated clock period to $CURRENT_CLOCK_PERIOD in $SHA256_SDC"
}

# 執行 APR
run_apr() {
    innovus -no_gui -file "$APR_TCL" > "$LOG_FILE" 2>&1
}

# 解析報告
analyze_results() {
    SLACK=$(grep -oP '= Slack Time\s+\K-?[0-9]+\.[0-9]+' "$TIMING_RPT" | head -n 1)
    DRC=$(grep -oP 'Verification Complete : \K[0-9]+' "$DRC_RPT" | head -n 1)
    CHIP_AREA=$(grep -oP 'Total area of Chip:\s*\K[0-9]+\.[0-9]+' "$SUMMARY_RPT" | head -n 1)
    WIRE_LENGTH=$(grep -oP 'Total wire length:\s*\K[0-9]+\.[0-9]+' "$SUMMARY_RPT" | head -n 1)

    SLACK=${SLACK:-"-1"}
    DRC=${DRC:-"-1"}
    CHIP_AREA=${CHIP_AREA:-"-1"}
    WIRE_LENGTH=${WIRE_LENGTH:-"-1"}

    echo "DEBUG: Slack=$SLACK, DRC=$DRC, Area=$CHIP_AREA, Wire Length=$WIRE_LENGTH"
}

# 自動優化
auto_optimize() {
    BEST_RESULT=""
    BEST_METRIC=99999999
    BEST_FOUND=0

    while (( $(echo "$UTILIZATION <= $MAX_UTILIZATION" | bc -l) )); do
        CURRENT_CLOCK_PERIOD=$CLOCK_PERIOD

        while (( $(echo "$CURRENT_CLOCK_PERIOD <= $MAX_CLOCK_PERIOD" | bc -l) )); do
            echo "Running with UTILIZATION=$UTILIZATION, CLOCK_PERIOD=$CURRENT_CLOCK_PERIOD"

            modify_apr_tcl
            modify_clock_period
            run_apr
            analyze_results
            echo "$SLACK,$DRC,$CHIP_AREA,$WIRE_LENGTH,$UTILIZATION,$CURRENT_CLOCK_PERIOD" >> $CSV_FILE

            # 若 Slack > 0 且 DRC = 0，則記錄最佳結果
            if (( $(echo "$SLACK > 0" | bc -l) )) && (( "$DRC" == 0 )); then
                (( BEST_FOUND++ ))
                echo "$SLACK,$DRC,$CHIP_AREA,$WIRE_LENGTH,$UTILIZATION,$CURRENT_CLOCK_PERIOD" >> $BEST_CSV_FILE

                METRIC=$(echo "$CHIP_AREA + $WIRE_LENGTH + $CURRENT_CLOCK_PERIOD" | bc)
                if (( $(echo "$METRIC < $BEST_METRIC" | bc -l) )); then
                    BEST_METRIC=$METRIC
                    BEST_RESULT="$SLACK,$DRC,$CHIP_AREA,$WIRE_LENGTH,$UTILIZATION,$CURRENT_CLOCK_PERIOD"
                fi

                break
            fi

            # 增加時脈周期
            CURRENT_CLOCK_PERIOD=$(echo "$CURRENT_CLOCK_PERIOD + 1" | bc)
        done

        # 增加核心利用率
        UTILIZATION=$(echo "$UTILIZATION + 0.01" | bc)
    done

    if (( "$BEST_FOUND" > 0 )); then
        echo "Best optimization found: $BEST_RESULT"
    else
        echo "Optimization failed to converge"
    fi
}

# 執行最佳化流程
auto_optimize

