/*
 * Copyright 2013 NVIDIA Corporation. All rights reserved.
 *
 * Sample CUPTI app to demonstrate the usage of unified memory counter profiling
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <cupti.h>

#define CUPTI_CALL(call)                                                    \
do {                                                                        \
    CUptiResult _status = call;                                             \
    if (_status != CUPTI_SUCCESS) {                                         \
      const char *errstr;                                                   \
      cuptiGetResultString(_status, &errstr);                               \
      fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n",  \
              __FILE__, __LINE__, #call, errstr);                           \
      exit(-1);                                                             \
    }                                                                       \
} while (0)

#define DRIVER_API_CALL(apiFuncCall)                                           \
do {                                                                           \
    CUresult _status = apiFuncCall;                                            \
    if (_status != CUDA_SUCCESS) {                                             \
        fprintf(stderr, "%s:%d: error: function %s failed with error %d.\n",   \
                __FILE__, __LINE__, #apiFuncCall, _status);                    \
        exit(-1);                                                              \
    }                                                                          \
} while (0)

#define RUNTIME_API_CALL(apiFuncCall)                                          \
do {                                                                           \
    cudaError_t _status = apiFuncCall;                                         \
    if (_status != cudaSuccess) {                                              \
        fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n",   \
                __FILE__, __LINE__, #apiFuncCall, cudaGetErrorString(_status));\
        exit(-1);                                                              \
    }                                                                          \
} while (0)

#define BUF_SIZE (8 * 1024)
#define ALIGN_SIZE (8)
#define ALIGN_BUFFER(buffer, align)                                            \
    (((uintptr_t) (buffer) & ((align)-1)) ? ((buffer) + (align) - ((uintptr_t) (buffer) & ((align)-1))) : (buffer))

static const char *
getUvmCounterKindString(CUpti_ActivityUnifiedMemoryCounterKind kind)
{
    switch (kind) 
    {
    case CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_KIND_BYTES_TRANSFER_HTOD:
        return "BYTES_TRANSFER_HTOD";
    case CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_KIND_BYTES_TRANSFER_DTOH:
        return "BYTES_TRANSFER_DTOH";
    case CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_KIND_CPU_PAGE_FAULT_COUNT:
        return "CPU_PAGE_FAULT_COUNT";
    default:
        break;
    }
    return "<unknown>";
}

static const char *
getUvmCounterScopeString(CUpti_ActivityUnifiedMemoryCounterScope scope)
{
    switch (scope) 
    {
    case CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_SCOPE_PROCESS_SINGLE_DEVICE:
        return "PROCESS_SINGLE_DEVICE";
    case CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_SCOPE_PROCESS_ALL_DEVICES:
        return "PROCESS_ALL_DEVICES";
    default:
        break;
    }
    return "<unknown>";
}

static void
printActivity(CUpti_Activity *record)
{
    switch (record->kind) 
    {
    case CUPTI_ACTIVITY_KIND_UNIFIED_MEMORY_COUNTER:
        {
            CUpti_ActivityUnifiedMemoryCounter *uvm = (CUpti_ActivityUnifiedMemoryCounter *)record;
            printf("UNIFIED_MEMORY_COUNTER [ %llu ] scope=%s kind=%s value=%llu device %u\n",
                (unsigned long long)(uvm->timestamp),
                getUvmCounterScopeString(uvm->scope),
                getUvmCounterKindString(uvm->counterKind),
                (unsigned long long)uvm->value,
                uvm->deviceId);
            break;
        }
    default:
        printf("  <unknown>\n");
        break;
    }
}

static void CUPTIAPI
bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    uint8_t *rawBuffer;

    *size = BUF_SIZE;
    rawBuffer = (uint8_t *)malloc(*size + ALIGN_SIZE);

    *buffer = ALIGN_BUFFER(rawBuffer, ALIGN_SIZE);
    *maxNumRecords = 0;

    if (*buffer == NULL) {
        printf("Error: out of memory\n");
        exit(-1);
    }
}

static void CUPTIAPI
bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize)
{
    CUptiResult status;

    if (ctx == NULL) {
        printf("==== Starting dump for global ====\n");
    } else if (streamId == 0) {
        printf("==== Starting dump for context %p ====\n", ctx);
    } else {
        printf("==== Starting dump for context %p, stream %u ====\n", ctx, streamId);
    }

    CUpti_Activity *record = NULL;
    do {
        status = cuptiActivityGetNextRecord(buffer, validSize, &record);
        if (status == CUPTI_SUCCESS) {
            printActivity(record);
        }
        else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED) {
            break;
        }
        else {
            CUPTI_CALL(status);
        }
    } while (1);

    // report any records dropped from the queue
    size_t dropped;
    CUPTI_CALL(cuptiActivityGetNumDroppedRecords(ctx, streamId, &dropped));
    if (dropped != 0) {
        printf("Dropped %u activity records\n", (unsigned int)dropped);
    }

    if (ctx == NULL) {
        printf("==== Finished dump for global ====\n");
    } else if (streamId == 0) {
        printf("==== Finished dump for context %p ====\n", ctx);
    } else {
        printf("==== Finished dump for context %p, stream %u ====\n", ctx, streamId);
    }

    free(buffer);
}

template<class T>
__host__ __device__ void checkData(const char *loc, T *data, int size, int expectedVal) {
    int i;

    for (i = 0; i < size / (int)sizeof(T); i++) {
        if (data[i] != expectedVal) {
            printf("Mismatch found on %s\n", loc);
            printf("Address 0x%p, Observed = 0x%x Expected = 0x%x\n", data+i, data[i], expectedVal);
            break;
        }
    }
}

template<class T>
__host__ __device__ void writeData(T *data, int size, int writeVal) {
    int i;

    for (i = 0; i < size / (int)sizeof(T); i++) {
        data[i] = writeVal;
    }
}

__global__ void testKernel(int *data, int size, int expectedVal)
{
    checkData("GPU", data, size, expectedVal);
    writeData(data, size, -expectedVal);
}

int main(int argc, char **argv)
{
    CUptiResult res;
    int deviceCount;
    int *data = NULL;
    int size = 64*1024;     // 64 KB
    int i = 123;
    CUpti_ActivityUnifiedMemoryCounterConfig config[3];

    DRIVER_API_CALL(cuInit(0));

    DRIVER_API_CALL(cuDeviceGetCount(&deviceCount));

    if (deviceCount == 0) {
        printf("There is no device supporting CUDA.\n");
        exit(-1);
    }

    // register cupti activity buffer callbacks
    CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));

    // configure unified memory counters
    config[0].scope = CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_SCOPE_PROCESS_SINGLE_DEVICE;
    config[0].kind = CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_KIND_BYTES_TRANSFER_HTOD;
    config[0].deviceId = 0;
    config[0].enable = 1;

    config[1].scope = CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_SCOPE_PROCESS_SINGLE_DEVICE;
    config[1].kind = CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_KIND_BYTES_TRANSFER_DTOH;
    config[1].deviceId = 0;
    config[1].enable = 1;

    config[2].scope = CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_SCOPE_PROCESS_SINGLE_DEVICE;
    config[2].kind = CUPTI_ACTIVITY_UNIFIED_MEMORY_COUNTER_KIND_CPU_PAGE_FAULT_COUNT;
    config[2].deviceId = 0;
    config[2].enable = 1;

    res = cuptiActivityConfigureUnifiedMemoryCounter(config, 3);
    if (res == CUPTI_ERROR_NOT_SUPPORTED) {
        printf("Test is waived, unified memory is not supported on the underlying platform.\n");
        return 0;
    }
    else {
        CUPTI_CALL(res);
    }

    // enable unified memory counter activity
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_UNIFIED_MEMORY_COUNTER));

    // allocate unified memory
    printf("Allocation size in bytes %d\n", size);
    RUNTIME_API_CALL(cudaMallocManaged(&data, size));

    // CPU access
    writeData(data, size, i);
    // kernel launch
    testKernel<<<1,1>>>(data, size, i);
    RUNTIME_API_CALL(cudaDeviceSynchronize());
    // CPU access
    checkData("CPU", data, size, -i);

    // free unified memory
    RUNTIME_API_CALL(cudaFree(data));

    CUPTI_CALL(cuptiActivityFlushAll(0));

    // disable unified memory counter activity
    CUPTI_CALL(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_UNIFIED_MEMORY_COUNTER));

    cudaDeviceReset();

    return 0;
}
