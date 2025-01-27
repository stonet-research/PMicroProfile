#pragma once
#include "trace.hpp"

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

#define Mebibyte (1024 * 1024)
#define Gibibyte (1024 * 1024 * 1024)

#define ALIGN_TO_64(ptr) ((char*)(((uintptr_t)(ptr) + 63) & ~(uintptr_t)63))
//#define STRICT_CONSISTENCY

static constexpr size_t CACHE_LINE_SIZE = 64;

extern size_t sample_rate;
extern size_t sample_duty_cycle;

// #define SAMPLE_RATE  100  //50000000
// #define SAMPLE_DUTY_CYCLE 75
//#define SAMPLE_LENGTH 8000000  //500000
#define ENABLE_DCOLLECTION

// period_on = (period * (thread_data->duty_cycle)) / 100;
// 	period_off = (period * ((100 - thread_data->duty_cycle))) / 100;

extern unsigned long SAMPLE_PERIOD;
extern unsigned long SAMPLE_PERIOD_ON_US;
extern unsigned long SAMPLE_PERIOD_OFF_US;

// See: https://perfmon-events.intel.com/cascadelake_server.html
#define EVENT_UNC_M_CLOCKTICKS 0x00 // umask=0x0,event=0x0 
#define EVENT_UNC_M_PMM_WPQ_INSERTS 0xE7
#define EVENT_UNC_M_PMM_RPQ_INSERTS 0xE3
#define EVENT_UNC_M_PMM_RPQ_OCCUPANCY_ALL 0x1E0 // umask=0x1,event=0xE0
#define EVENT_UNC_M_PMM_WPQ_OCCUPANCY_ALL 0x1E4  // umask=0x1,event=0xE4
#define EVENT_UNC_M_RPQ_INSERTS 0x10
#define EVENT_UNC_M_RPQ_OCCUPANCY 0x80

#define EVENT_MEM_LOAD_L3_MISS_RETIRED_LOCAL_PMM 0x80D1 // umash=0x80,event=D1
#define EVENT_MEM_LOAD_L3_MISS_RETIRED_REMOTE_PMM 0x10D3 // umash=0x10,event=D3
#define EVENT_MEM_PMM_HIT_LOCAL_ANY_SNOOP 0x1b7 // ocr.all_reads.pmm_hit_local_pmm.any_snoop
#define EVENT_MEM_INST_RETIRED_ALL_STORES 0x82d0

#define MSR_PMM_HIT_LOCAL_ANY_SNOOP 0x3f804007f7
#define MSR_L3_MISS_LOCAL_DRAM_ANY_SNOOP 0x3f84000491

// from: https://github.com/hpides/perma-bench/blob/75b6e3ceea6895fdb779b4981aa43a2ff6185104/src/read_write_ops.hpp
#define READ_SIMD_512(mem_addr, offset) _mm512_load_si512((void*)((mem_addr) + ((offset)*CACHE_LINE_SIZE)))

#define WRITE_SIMD_NT_512(mem_addr, offset, data) \
  _mm512_stream_si512(reinterpret_cast<__m512i*>((mem_addr) + ((offset)*CACHE_LINE_SIZE)), data)

#define WRITE_SIMD_512(mem_addr, offset, data) \
  _mm512_store_si512(reinterpret_cast<__m512i*>((mem_addr) + ((offset)*CACHE_LINE_SIZE)), data)

#define READ_SIMD_256(mem_addr, offset) _mm256_load_si256((void*)((mem_addr) + ((offset)*CACHE_LINE_SIZE)))

#define WRITE_SIMD_NT_256(mem_addr, offset, data) \
  _mm256_stream_si256(reinterpret_cast<__m256i*>((mem_addr) + ((offset)*CACHE_LINE_SIZE)), data)

#define WRITE_SIMD_256(mem_addr, offset, data) \
  _mm256_store_si256(reinterpret_cast<__m256i*>((mem_addr) + ((offset)*CACHE_LINE_SIZE)), data)

inline void set_sampling_rate(const size_t freq, const size_t duty_cycle)
{
    sample_rate = freq;
    sample_duty_cycle = duty_cycle;

    SAMPLE_PERIOD = 1000000000L / sample_rate;
    SAMPLE_PERIOD_ON_US = (SAMPLE_PERIOD * sample_duty_cycle) / 100;
    SAMPLE_PERIOD_OFF_US = (SAMPLE_PERIOD * (100 - sample_duty_cycle)) / 100;

    std::cout << SAMPLE_PERIOD_ON_US << std::endl;
}


class BenchSuite {
public:
    BenchSuite(TraceFile &trace_file,
        const std::string& pmem_device_loc,
        const size_t mem_size,
        const unsigned int num_threads,
        const uint64_t num_samples,
        const bool force_ram, 
        const bool do_fallback_ram,
        const bool do_cache_warming,
        const bool prealloc = true) :
    trace_file(trace_file),
    pmem_device_loc(pmem_device_loc),
    mem_size(mem_size),
    num_threads(num_threads),
    num_samples(num_samples),
    force_ram(force_ram),
    do_fallback_ram(do_fallback_ram),
    do_cache_warming(do_cache_warming)
    {
        mem_area = nullptr;
        dax_fd = -1;

        if (prealloc)
            allocate_mem_area();
    };

    ~BenchSuite()
    {
        deallocate_mem_area();
    }

    bool run(const size_t replay_rounds);

private:
    void drop_caches();

    bool allocate_pmem_area();
    bool allocate_dram_area();

    void allocate_mem_area();
    void deallocate_mem_area();

    TraceFile &trace_file;
    const std::string pmem_device_loc;
    const size_t mem_size;
    const unsigned int num_threads;
    const uint64_t num_samples;
    const bool force_ram;
    const bool do_fallback_ram;
    const bool do_cache_warming;

    void* mem_area;
    int dax_fd;
};
