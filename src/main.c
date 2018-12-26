/*********************************************************************
*
* ANSI C Example program:
*    VoltUpdate.c
*
* Example Category:
*    AO
*
* Description:
*    This example demonstrates how to output a single Voltage Update
*    (Sample) to an Analog Output Channel.
*
* Instructions for Running:
*    1. Select the Physical Channel to correspond to where your
*       signal is output on the DAQ device.
*    2. Enter the Minimum and Maximum Voltage Ranges.
*    Note: Use the Acq One Sample example to verify you are
*          generating the correct output on the DAQ device.
*
* Steps:
*    1. Create a task.
*    2. Create an Analog Output Voltage Channel.
*    3. Use the Write function to Output 1 Sample to 1 Channel on the
*       Data Acquisition Card.
*    4. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal output terminal matches the Physical
*    Channel I/O Control. For further connection information, refer
*    to your hardware reference manual.
*
*********************************************************************/

/*********************************************************************
* Microsoft Windows Vista User Account Control
* Running certain applications on Microsoft Windows Vista requires
* administrator privileges, because the application name contains keywords
* such as setup, update, or install. To avoid this problem, you must add an
* additional manifest to the application that specifies the privileges
* required to run the application. Some ANSI-C NI-DAQmx examples include
* these keywords. Therefore, these examples are shipped with an additional
* manifest file that you must embed in the example executable. The manifest
* file is named [ExampleName].exe.manifest, where [ExampleName] is the
* NI-provided example name. For information on how to embed the manifest
* file, refer to http://msdn2.microsoft.com/en-us/library/bb756929.aspx.
*********************************************************************/

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <windows.h>
#include <string.h>
#include <NIDAQmx.h>

#define CSV_COLS 2
#define PERIOD_MS 10
#define BUF_SIZE 10000
#define FS 100
#define CR 10

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) {finalize();return -100;} else
#define DAQmxErrChk1(functionCall) if( DAQmxFailed(error=(functionCall)) ) {finalize();} else

void finalize();

volatile int error = 0;
TaskHandle taskHandle = 0;

int csv_read(char *filename, int *rows, double data[][CSV_COLS]) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        TCHAR buf[256] = {'\0'};
        GetCurrentDirectory(256, buf);
        printf("couldn't open %s\\%s\n", buf, filename);
        return -1;
    }
    double buf[CSV_COLS] = {0.0};
    int i, j;
    for (i = 0; (fscanf(fp, "%lf,%lf", &buf[0], &buf[1])) != EOF && i < *rows; i++) {
        for (j = 0; j < CSV_COLS; ++j) {
            data[i][j] = buf[j];
        }
    }
    *rows = i;
    return 0;
}

int csv_write(char *filename, int rows, double data[][CSV_COLS]) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        TCHAR buf[256] = {'\0'};
        GetCurrentDirectory(256, buf);
        printf("couldn't open %s\\%s\n", buf, filename);
        return -1;
    }
    int i;
    for (i = 0; i < rows; i++) {
        fprintf(fp, "%lf,%lf\n", data[i][0], data[i][1]);
    }
    return 0;
}

int daq_init() {
    char errBuff[2048] = {'\0'};
    float64 data[1] = {1.0};

    /*********************************************/
    // DAQmx Configure Code
    /*********************************************/
    DAQmxErrChk (DAQmxCreateTask("", &taskHandle));
    DAQmxErrChk (DAQmxCreateAOVoltageChan(taskHandle, "Dev1/ao0", "", 0.0, 5.0, DAQmx_Val_Volts, ""));
    DAQmxErrChk (DAQmxSetSampTimingType(taskHandle, DAQmx_Val_OnDemand));

    /*********************************************/
    // DAQmx Start Code
    /*********************************************/
    DAQmxErrChk (DAQmxStartTask(taskHandle));

    DAQmxWriteAnalogF64(taskHandle, 1, 1, 0.1, DAQmx_Val_GroupByChannel, data, NULL, NULL);

    return 0;
}

volatile int counter = 0;
volatile const int period_ms = PERIOD_MS;

void finalize() {
    float64 data[1] = {0.0};
    DAQmxWriteAnalogF64(taskHandle, 1, 1, 0.1, DAQmx_Val_GroupByChannel, data, NULL, NULL);
    DAQmxStopTask(taskHandle);
    DAQmxClearTask(taskHandle);
}

double sin_(double amp, double freq, double bias) {
    return amp * sin(M_2_PI * freq * (double) counter * period_ms / 100.0) + bias;
}

double source[BUF_SIZE][CSV_COLS] = {0.0};

VOID CALLBACK callback(PVOID pvoid, DWORD dword, DWORD dword1) {
    double tmp = source[counter][1];
    if (tmp < 0) tmp = 0;
    else if (tmp > 5) tmp = 5;
    float64 data[1] = {tmp};
    /*********************************************/
    // DAQmx Write Code
    /*********************************************/
    DAQmxErrChk1 (DAQmxWriteAnalogF64(taskHandle, 1, 1, 0.01, DAQmx_Val_GroupByChannel, data, NULL, NULL));
    printf("Output %lf\n", data[0]);
}

int out(char *filename, int sec) {
    int n = sec * FS;
    HANDLE timer = NULL;
    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    daq_init();
    if (timer == NULL) {
        printf("ERROR timer NULL");
    }
    int csvN = (n == 0) ? BUF_SIZE : n;
    if (csv_read(filename, &csvN, source) != 0) {
        finalize();
        return -1;
    }

    LARGE_INTEGER li = {0};
    if (!SetWaitableTimer(timer, &li, CR, callback, NULL, FALSE)) {
        printf("ERROR cant timer set");
    }
    if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0) {
        printf("Something ERROR");
    }

    int totalCount = 0;
    while (n == 0 || totalCount < n) {
        if (counter == csvN) counter = 0;
        printf("DO %d times -> ", counter);
        SleepEx(INFINITE, TRUE);
        counter++;
        totalCount++;
    }

    if (!CloseHandle(timer)) {
        printf("ERROR cant close timer");
    }
    return 0;
}

int in(const char *filename, int sec) {
    int n = sec * FS;
    TaskHandle taskHandle;
    int32 read;
    float64 data[BUF_SIZE] = {0.0};

    /*********************************************/
    // DAQmx Configure Code
    /*********************************************/
    DAQmxErrChk (DAQmxCreateTask("", &taskHandle));
    DAQmxErrChk (
            DAQmxCreateAIVoltageChan(taskHandle, "Dev1/ai0", "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts,
                                     NULL));
    DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle, "", FS, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps,
                                       n));
    DAQmxErrChk (DAQmxSetAITermCfg(taskHandle, "Dev1/ai0", DAQmx_Val_RSE));

    /*********************************************/
    // DAQmx Start Code
    /*********************************************/
    printf("Ready to Sample...");
    getchar();
    printf("GO!!!\n");
    DAQmxErrChk (DAQmxStartTask(taskHandle));

    /*********************************************/
    // DAQmx Read Code
    /*********************************************/
    DAQmxErrChk (
            DAQmxReadAnalogF64(taskHandle, n, (double) n / FS, DAQmx_Val_GroupByChannel, data, BUF_SIZE, &read,
                               NULL));

    printf("Acquired %d points\n", (int) read);
    int i;
    double buf[BUF_SIZE][CSV_COLS];
    for (i = 0; i < read; i++) {
        buf[i][0] = (double) i / FS;
        buf[i][1] = (data[i] < 0) ? 0 : data[i];
    }
    if (csv_write(filename, read, buf) != 0) {
        finalize();
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    puts("debug");
    if (argc != 4) {
        printf("Bad Argument Number. %d\n", argc);
        return -10;
    }
    int sec = 0;
    if (sscanf(argv[3], "%d", &sec) < 1) {
        printf("Bad Time. %d\n", sec);
        return -13;
    }
    if (strcmp(argv[1], "in") == 0) {
        if (in(argv[2], sec) != 0) {
            printf("Bad File Name %s\n", argv[2]);
            return -11;
        }
    } else if (strcmp(argv[1], "out") == 0) {
        if (out(argv[2], sec) != 0) {
            printf("Bad File Name %s\n", argv[2]);
            return -11;
        }
    } else {
        printf("Bad Argument Type. \"in\" or \"out\" %s\n", argv[1]);
        return -12;
    }
    finalize();
    return 0;
}
