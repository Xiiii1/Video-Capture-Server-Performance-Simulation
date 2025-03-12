#include <stdio.h>
#include <math.h>
#include "lcgrand.h"

#define BUFFER_LIMIT 20  // 緩衝區的最大容量
#define BUSY 1
#define IDLE 0
#define ALPHA 0.1          // 幀大小與複雜性的比例因子
#define C_ENC 15800.0      // 編碼器的處理能力 (fobs/秒)
#define C_STORAGE 1600.0   // 存儲伺服器的處理能力 (bytes/秒)
#define size_stor_queue 99999999  // size of storage buffer

int enc_buffer[BUFFER_LIMIT + 1];
int sto_buffer[size_stor_queue + 2];
int enc_buffer_count;       // 當前encoder緩衝區中的影像幀數量
int sto_buffer_count;       // 當前storage queue的影像幀數量
int encoder_status;     // 編碼器的狀態，可能為 BUSY 或 IDLE
int storage_status;     // 存儲伺服器的狀態，可能為 BUSY 或 IDLE
int next_event_type;    // 下一個即將發生的事件類型
int total_frame_lost; // 總共丟失的影像幀數
int TorB;
float sim_time;         // 當前的模擬時間
float time_next_event[4];  // 各事件的下一次發生時間（索引 1 到 3）
//int total_frame_stored;  // 總共儲存好的影像幀數
float total_storage_time; // 存儲伺服器的總忙碌時間
float sim_duration;     // 總模擬時間
float mean_interarrival; // 平均到達間隔時間
float mean_complexity;   // 平均字段複雜性
float last_storage_server_start;
FILE* infile, * outfile;

// 函數宣告
void initialize(void);
void timing(void);
void arrive(void);
void encode_complete(void);
void store_complete(void);
void report(void);
float expon(float mean);

int main() {
    // 打開輸入與輸出檔案
    infile = fopen("simulation.in", "r");
    outfile = fopen("simulation.out", "w");
    if (infile == NULL || outfile == NULL) {
        printf("Error opening input or output file.\n");
        return 1;
    }
    // 從輸入檔案中讀取模擬參數（模擬時長 8 小時、平均到達時間 1/120、平均字段複雜性 h = 200）
    fscanf(infile, "%f %f %f", &sim_duration, &mean_interarrival, &mean_complexity);
    //printf("sim_duration = %f\n", sim_duration);

    // 初始化模擬環境
    initialize();

    // 模擬主循環
    while (sim_time < sim_duration) {
        timing();
        switch (next_event_type) {
        case 1:
            //printf("Event: Arrive\n");
            arrive();
            break;
        case 2:
            //printf("Event: Encode Complete\n");
            encode_complete();
            break;
        case 3:
            //printf("Event: Store Complete\n");
            store_complete();
            break;
        }
    }
    //printf("simulation complete\n");

    report();

    fclose(infile);
    fclose(outfile);

    return 0;
}

void initialize(void) {
    sim_time = 0.0;  // 初始化模擬時間為 0
    encoder_status = IDLE;
    storage_status = IDLE;
    enc_buffer_count = 0;   //enc_buffer[0] for encoder server
    sto_buffer_count = 1;   // sto_buffer[0] & sto_buffer[1] for storage server
    total_frame_lost = 0;
    //total_frame_stored = 0;
    total_storage_time = 0.0;
    last_storage_server_start = 0.0;
    TorB = 0;   // 1 for "T", 0 for "b", -1 for nothing there
    for (int i = 0; i < BUFFER_LIMIT + 1; i++) enc_buffer[i] = -1;
    for (int i = 0; i < size_stor_queue + 1; i++) sto_buffer[i] = -1;
    // 初始化第一個字段到達的時間
    time_next_event[1] = 0; // 題目假設第一個T在 t = 0時到達
    time_next_event[2] = 1.0e+30;  // 尚無編碼完成事件
    time_next_event[3] = 1.0e+30;  // 尚無存儲完成事件
}

void timing(void) {
    int i;
    float min_time_next_event = 1.0e+29;
    next_event_type = 0;

    // 找出下一個即將發生的事件
    for (i = 1; i <= 3; ++i) {
        if (time_next_event[i] < min_time_next_event) {
            min_time_next_event = time_next_event[i];
            next_event_type = i;
        }
    }
    if (next_event_type == 0) {
        fprintf(outfile, "Event list empty at time %f\n", sim_time);
        //sim_time = sim_duration;
        return;
    }
    sim_time = min_time_next_event;
}

void arrive(void) {
    // 設置下一個字段到達的時間
    time_next_event[1] = sim_time + expon(mean_interarrival);
    TorB++;

    /* if the buffer is full */
    if (enc_buffer_count >= BUFFER_LIMIT) {
        // if 進來的是T，，那除了這個T不要以外，下一個arrive的B也不要
        if (TorB % 2 == 1) {
            TorB++; // 略過下一個arrive的B
            time_next_event[1] = sim_time + expon(mean_interarrival) * 2;
        }
        // if 進來的是B，那除了這個B不要以外，queue最後面的T也不要
        else {
            enc_buffer[BUFFER_LIMIT] = -1;  // 最後一格定義為空，可以不用這行
            enc_buffer_count--;
        }
        total_frame_lost++;  // 緩衝區已滿，字段丟失。1 frame = 1 T field + 1 B field
    }

    /* if the buffer isn't full */
    else {
        enc_buffer_count++;  // 添加字段到queue

        // the incoming field is "T"
        // if (TorB % 2 == 1) {
        //     enc_buffer[enc_buffer_count] = 1;   // [0]定義為server
        // }
        // // the incoming field is "B"
        // else {
        //     enc_buffer[enc_buffer_count] = 0;
        // }

        if (encoder_status == IDLE) {  // 如果編碼器空閒
            encoder_status = BUSY;
            // 移動queue，送入encoder server, which is enc_buffer[0]
            // enc_buffer index從0~BUFFER_LIMIT，所以i + enc_buffer_count
            // for (int i = 0;i < enc_buffer_count;i++) {
            //     enc_buffer[i] = enc_buffer[i + 1];
            // }
            // enc_buffer[enc_buffer_count] = -1;  // [enc_buffer_count]定義為空，可以不用這行
            enc_buffer_count--;                 // 因為queue中的一個field進入encoder server了

            float complexity = expon(mean_complexity);  // 隨機生成字段複雜性
            time_next_event[2] = sim_time + complexity / C_ENC;  // 計算編碼所需時間
        }
    }
}

void encode_complete(void) {
    // encode 完成後送入 storage queue。不用考慮storage queue是否滿了，所以直接++
    sto_buffer_count++;
    sto_buffer[sto_buffer_count] = enc_buffer[0];   //  enc_buffer[0] means the field is in the encoder server
    // 如果queue還有需要處理的field，則移動encode queue, 送queue最前頭的field進入encoder server 
    // enc_buffer index從0~BUFFER_LIMIT，所以i + enc_buffer_count
    if (enc_buffer_count > 0) {
        // for (int i = 0;i < enc_buffer_count;i++) {
        //     enc_buffer[i] = enc_buffer[i + 1];
        // }
        // enc_buffer[enc_buffer_count] = -1; // [enc_buffer_count]定義為空，可以不用這行
        enc_buffer_count--;

        float complexity = expon(mean_complexity);
        time_next_event[2] = sim_time + complexity / C_ENC;
    }
    else {
        encoder_status = IDLE;  // 緩衝區空，編碼器空閒
        time_next_event[2] = 1.0e+30;
    }

    if (storage_status == IDLE && sto_buffer_count > 2) {  // 如果存儲伺服器空閒且T跟B都已收到
        storage_status = BUSY;
        // 將queue開頭的T跟B field送入Storage server，整個queue平移
        // for (int i = 0;i < sto_buffer_count - 1;i += 2) {
        //     sto_buffer[i] = sto_buffer[i + 2];
        // }
        // sto_buffer[sto_buffer_count] = sto_buffer[sto_buffer_count - 1] = -1;
        sto_buffer_count -= 2;

        float frame_size = ALPHA * (expon(mean_complexity) + expon(mean_complexity));  // 計算幀大小
        time_next_event[3] = sim_time + frame_size / C_STORAGE;  // 計算存儲時間 (storage server完成的時間)
        last_storage_server_start = sim_time;
    }
}

void store_complete(void) {
    // frame儲存完成
    //total_frame_stored++;

    // 累加繁忙時間段
    total_storage_time += (sim_time - last_storage_server_start);

    // 如果storage queue裡面還有至少一組T跟B，則排程下一個store_complete
    if (sto_buffer_count > 2) {
        // for (int i = 0;i < sto_buffer_count - 1;i += 2) {
        //     sto_buffer[i] = sto_buffer[i + 2];
        // }
        // sto_buffer[sto_buffer_count] = sto_buffer[sto_buffer_count - 1] = -1;
        sto_buffer_count -= 2;

        float frame_size = ALPHA * (expon(mean_complexity) + expon(mean_complexity));
        time_next_event[3] = sim_time + frame_size / C_STORAGE;
        last_storage_server_start = sim_time;
    }
    else {
        storage_status = IDLE;
        time_next_event[3] = 1.0e+30;
    }
}

void report(void) {
    float frame_loss_ratio = total_frame_lost / (TorB / 2.0);   // 總共傳了(TorB/2)組T & B
    float storage_utilization = total_storage_time / sim_time;
    fprintf(outfile, "Buffer Limit: %d\n", BUFFER_LIMIT);
    fprintf(outfile, "Capacity of encoder: %.2f\n", C_ENC);
    fprintf(outfile, "Capacity of storage: %.2f\n", C_STORAGE);
    fprintf(outfile, "Frame Loss Ratio (f): %.8f\n", frame_loss_ratio);
    fprintf(outfile, "Storage Utilization (u): %.8f", storage_utilization);
    // 調試輸出
    printf("Buffer Limit: %d\n", BUFFER_LIMIT);
    printf("Capacity of encoder: %.2f\n", C_ENC);
    printf("Capacity of storage: %.2f\n", C_STORAGE);
    printf("-------------------------------------------------------\n");
    printf("Final Sim Time: %.2f, Total Storage Time: %.2f\n", sim_time, total_storage_time);
    printf("Total frames loss: %d\n", total_frame_lost);
    printf("Total fields sent: %d\n", TorB);
    printf("Frame Loss Ratio (f) : %.8f\n", frame_loss_ratio);
    printf("Storage Utilization (u): %.8f\n", storage_utilization);
}

float expon(float mean)  /* Exponential variate generation function. */
{
    /* Return an exponential random variate with mean "mean". */

    return (-mean * log(lcgrand(1)));
}