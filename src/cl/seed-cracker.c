#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#include "clutil.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

#ifdef _MSC_VER
#include <malloc.h>
#define alloca _alloca
#define VLA(type, name, elems) type *name = alloca(sizeof(type) * elems)
#else
#define VLA(type, name, elems) type name[elems]
#endif

#define CLEAR_LINE "\x1b[K"
#define TYPE CL_DEVICE_TYPE_ALL
#define DUMP_PTX

static inline uint64_t get_timer() {
#ifdef _MSC_VER
  return ((uint64_t) clock())  * 1000000000ULL;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t) ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
#endif
}

static inline uint64_t start_timer() {
  return get_timer();
}

static inline uint64_t stop_timer(uint64_t start) {
  return get_timer() - start;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fputs("Expected at least 1 argument\n", stderr);
    return -1;
  }
  const char *kernel_file = argv[1];
  const char *kernel_name = argc > 2 ? argv[2] : "kern";
  
  printf("PID: %u\n", getpid());
  
  cl_uint num_platforms;
  check(clGetPlatformIDs(0, NULL, &num_platforms), "getPlatformIDs (num)");
  printf("%d platforms:\n", num_platforms);
  
  VLA(cl_platform_id, platforms, num_platforms);
  check(clGetPlatformIDs(num_platforms, platforms, NULL), "getPlatformIDs");
  
  cl_device_id device = NULL;
  cl_uint cus = 0;
  
  printf("Available platforms:\n");
  for (int i = 0; i < num_platforms; i++) {
    char *info = getPlatformInfo(platforms[i]);
    printf("%d: %s\n", i, info);
    free(info);
    cl_uint num_devices;
    check(clGetDeviceIDs(platforms[i], TYPE, 0, NULL, &num_devices), "getDeviceIDs (num)");
    
    VLA(cl_device_id, devices, num_devices);
    check(clGetDeviceIDs(platforms[i], TYPE, num_devices, devices, NULL), "getDeviceIDs");
    
    printf("  %d available devices:\n", num_devices);
    for (int i = 0; i < num_devices; i++) {
      device_info *info = getDeviceInfo(devices[i]);
      printf("    %s\n", info->info_str);
      if (info->compute_units >= cus) {
        cus = info->compute_units;
        device = devices[i];
      }
      free(info);
    }
    putchar('\n');
  }
  
  if (!device) {
    fprintf(stderr, "No devices found.\n");
    return -1;
  }

  cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
  device_info *info = getDeviceInfo(device);
  printf("Using %s\n", info->info_str);
  free(info);
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
  
  const char *header_names[] = { "jrand.cl", "seed-cracking.cl" };
  cl_int error;
  int num_headers = sizeof(header_names) / sizeof(char *);
  VLA(cl_program, headers, num_headers);
  for (int i = 0; i < num_headers; i++) {
    const char *header_src = readFile(header_names[i]);
    headers[i] = clCreateProgramWithSource(context, 1, &header_src, NULL, &error);
    if (error != CL_SUCCESS) {
      fprintf(stderr, "Error creating header %s: %s\n", header_names[i], getErrorString(error));
      exit(error);
    }
    free(header_src);
  }
  
  const char *main_src = readFile(kernel_file);
  cl_program main_cl = clCreateProgramWithSource(context, 1, &main_src, NULL, &error);
  check(error, "Creating program");
  printf("Compiling...\n");
  error = clCompileProgram(main_cl, 0, NULL, NULL, num_headers, headers, header_names, NULL, NULL);
  if (error == CL_BUILD_PROGRAM_FAILURE || error == CL_COMPILE_PROGRAM_FAILURE) {
    char *log = getBuildLog(main_cl, device);
    fprintf(stderr, "Error compiling:\n%s\n", log);
    free(log);
    exit(error);
  } else if (error) {
    fprintf(stderr, "Error compiling: %s\n", getErrorString(error));
    exit(error);
  }
  const cl_program programs[] = { main_cl };
  printf("Linking...\n");
  cl_program program = clLinkProgram(context, 0, NULL, NULL, 1, programs, NULL, NULL, &error);
  check(error, "Linking");
  
  char *kf = alloca(strlen(kernel_file));
  strcpy(kf, kernel_file);
  kf[strlen(kf) - 3] = 0;
  #ifdef DUMP_PTX
  dumpProgramCode(program, kf);
  #endif
  
  cl_kernel kernel = clCreateKernel(program, kernel_name, &error);
  check(error, "Creating kernel");
  
  int x2total = 37;
  int x2step = 20;
  size_t ntotal = 1LL << x2total;
  size_t nstep = 1LL << min(x2step, x2total);
  uint64_t stride = 1LL << (48 - x2total);
  
  uint64_t nsteps = 1LL << (x2total - min(x2step, x2total));
  printf("Total: %lu (%u), steps: %lu (%u), step size: %lu (%u), stride: %lu (%u)\n", ntotal, x2total, nsteps, x2total - min(x2step, x2total), nstep, min(x2step, x2total), stride, 48 - x2total);

  cl_mem mem_seeds = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong) * nstep, NULL, NULL);
  cl_mem mem_output = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ushort) * nstep, NULL, NULL);
  
  check(clSetKernelArg(kernel, 1, sizeof(cl_ulong), &stride), "Argument stride");
  check(clSetKernelArg(kernel, 2, sizeof(cl_mem), &mem_seeds), "Argument seeds");
  check(clSetKernelArg(kernel, 3, sizeof(cl_mem), &mem_output), "Argument output");
  
  size_t global_work_size[1] = { nstep };
  
  uint64_t *seeds = malloc(nstep * sizeof(uint64_t));
  uint16_t *output = malloc(nstep * sizeof(uint16_t));
  
  printf("Executing %s.%s\n", kf, kernel_name);
  
  int log_file_name_len = strlen(kf) + 9;
  char *log_file_name = alloca(log_file_name_len);
  snprintf(log_file_name, log_file_name_len, "out-%s.txt", kf);
  FILE *log_file = fopen(log_file_name, "w");
  if (!log_file) perror("Error opening logfile");
  printf("Writing log to %s\n", log_file_name);

  uint64_t t = start_timer();
  for (size_t offset = 0; offset < ntotal; offset += nstep) {
    for (size_t i = 0; i < nstep; i++) seeds[i] = offset + i;
    float perc = offset * 100.f / ntotal;
    uint64_t t2 = start_timer();
    check(clSetKernelArg(kernel, 0, sizeof(offset), &offset), "Argument offset");
    printf("\rx  %3.3f%%", perc);
    fflush(stdout);
    check(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL), "\nExecute");
    usleep(10000);
    check(clFinish(queue), "\nFinish execute");
    printf("\r<- %3.3f%%", perc);
    fflush(stdout);
    check(clEnqueueReadBuffer(queue, mem_output, 1, 0, nstep * sizeof(uint16_t), output, 0, NULL, NULL), "\nRead");
    check(clEnqueueReadBuffer(queue, mem_seeds, 1, 0, nstep * sizeof(uint64_t), seeds, 0, NULL, NULL), "\nRead");
    uint64_t d2 = stop_timer(t2);
    printf("\rw  %3.3f%% %.4f" CLEAR_LINE, perc, d2 / 1000000.f);
    fflush(stdout);
    for (size_t i = 0; i < nstep; i++) {
      uint8_t count = output[i];
      if (count > 0) {
        fprintf(log_file, "%016llx\n", seeds[i]);
        printf("\r%016llx%s\n", seeds[i], CLEAR_LINE);
        if (count > 1) {
          printf("%d more solutions in the range %llx-%llx\n", count - 1, (offset + i) * stride, (offset + i + 1) * stride - 1);
        }
      }
    }
    fflush(log_file);
    uint64_t d = get_timer() - t;
    double per_item = (double) d / (offset + nstep);
    double eta = ((double) d / ((offset + nstep) * 1000000000.)) * (ntotal - offset - nstep);
    uint64_t d2ms = d2 / 1000000;
    printf("  %fs / %llu items = %lfns/item, %lums/batch, ETA: %lfs (%dh%dm%ds)", d / 1000000000.f, offset + nstep,
      per_item, d2ms, eta,
      (int) (eta / 3600), ((int) (eta / 60)) % 60, ((int) eta) % 60);
  }
  puts("\rDone.                                                       ");
  uint64_t d = stop_timer(t);
  fclose(log_file);
  
  printf("%fs / %llu items = %lfns/item\n", d / 1000000000.f, ntotal, (double) d / ntotal);
  return 0;
}
