#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

#include "clutil.h"

#define TYPE CL_DEVICE_TYPE_ALL

static inline uint64_t get_timer() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t) ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
}

static inline uint64_t start_timer() {
  return get_timer();
}

static inline uint64_t stop_timer(uint64_t start) {
  return get_timer() - start;
}

int main(int argc, char **argv) {
  const char *file = argc > 1 ? argv[1] : "out.dat";
  const char *kernel_file = "iron.cl";
  const char *kernel_name ="main";
  
  printf("PID: %u\n", getpid());
  
  cl_uint num;
  clGetDeviceIDs(NULL, TYPE, 0, NULL, &num);
  
  if (num == 0) {
    fprintf(stderr, "No devices found.\n");
    return -1;
  }
  
  cl_device_id devices[num];
  clGetDeviceIDs(NULL, TYPE, num, devices, NULL);
  
  printf("Available devices:\n");
  for (int i = 0; i < num; i++) {
    char *info = getDeviceInfo(devices[i]);
    puts(info);
    free(info);
  }

  cl_context context = clCreateContext(NULL, num, devices, NULL, NULL, NULL);
  cl_device_id device = devices[0];
  char *info = getDeviceInfo(device);
  printf("Using %s\n", info);
  free(info);
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
  
  const char *header_names[] = { "jrand.cl" };
  cl_int error;
  const char *jrand_src = readFile("jrand.cl");
  cl_program jrand_cl = clCreateProgramWithSource(context, 1, &jrand_src, NULL, &error);
  check(error, "Creating header jrand");
  const cl_program headers[] = { jrand_cl };
  free(jrand_src);
  //check(clCompileProgram(jrand_cl, 0, NULL, NULL, 0, headers, header_names, NULL, NULL), "Compiling jrand.cl");
  
  const char *main_src = readFile(kernel_file);
  cl_program main_cl = clCreateProgramWithSource(context, 1, &main_src, NULL, &error);
  check(error, "Creating program");
  error = clCompileProgram(main_cl, 0, NULL, NULL, 1, headers, header_names, NULL, NULL);
  if (error == CL_BUILD_PROGRAM_FAILURE) {
    char *log = getBuildLog(main_cl, device);
    fprintf(stderr, "Error compiling:\n%s\n", log);
    free(log);
    exit(error);
  } else if (error) {
    fprintf(stderr, "Error compiling: %s\n", getErrorString(error));
    exit(error);
  }
  const cl_program programs[] = { main_cl };
  cl_program program = clLinkProgram(context, 0, NULL, NULL, 1, programs, NULL, NULL, &error);
  check(error, "Linking");
  cl_kernel kernel = clCreateKernel(program, kernel_name, NULL);
  
  int x2total = 32;
  size_t ntotal = 1LL << x2total;
  size_t nstep = 1LL << min(x2total, 20);
  
  printf("Total: %lu, step: %lu\n", ntotal, nstep);

  cl_mem mem_input = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong) * nstep, NULL, NULL);
  cl_mem mem_output = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uchar) * nstep, NULL, NULL);
  
  check(clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_input), "Argument input");
  check(clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_output), "Argument output");
  
  size_t global_work_size[1] = { nstep };
  
  uint64_t *input = malloc(nstep * sizeof(uint64_t));
  uint8_t *output = malloc(nstep * sizeof(uint8_t));
  
  FILE *f = fopen(file, "w");
  
  char *kf = malloc(strlen(kernel_file));
  strcpy(kf, kernel_file);
  kf[strlen(kf) - 3] = 0;
  printf("Executing %s.%s\n", kf, kernel_name);

  uint64_t t = start_timer();
  for (size_t offset = 0; offset < ntotal; offset += nstep) {
    for (size_t i = 0; i < nstep; i++) input[i] = offset + i;
    float perc = offset * 100.f / ntotal;
    uint64_t t2 = start_timer();
    printf("\r-> %3.3f%%", perc);
    fflush(stdout);
    check(clEnqueueWriteBuffer(queue, mem_input, 1, 0, nstep * sizeof(uint64_t), input, 0, NULL, NULL), "\nWrite");
    printf("\rx  %3.3f%%", perc);
    fflush(stdout);
    check(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL), "\nExecute");
    printf("\r<- %3.3f%%", perc);
    fflush(stdout);
    check(clEnqueueReadBuffer(queue, mem_output, 1, 0, nstep * sizeof(uint8_t), output, 0, NULL, NULL), "\nRead");
    uint64_t d2 = stop_timer(t2);
    printf("\rw  %3.3f%% %.4f", perc, d2 / 1000000.f);
    fflush(stdout);
    int max_count = 0;
    uint64_t max_seed = 0;
    for (size_t i = 0; i < nstep; i++) {
      if (output[i] > max_count) {
        max_count = output[i];
        max_seed = offset + i;
      }
    }
    if (max_count > 2) printf("  %llx %d", max_seed, max_count);
    fwrite(output, sizeof(uint8_t), nstep, f);
    uint64_t d = get_timer() - t;
    double eta = ((double) d / ((offset + nstep) * 1000000000.)) * (ntotal - offset - nstep);
    printf("  %fs / %llu items = %lfns/item, ETA: %lfs", d / 1000000000.f, offset + nstep, (double) d / (offset + nstep), eta);
  }
  puts("\rDone.                                                       ");
  uint64_t d = stop_timer(t);
  
  printf("%fs / %llu items = %lfns/item\n", d / 1000000000.f, ntotal, (double) d / ntotal);
  fclose(f);
  return 0;
}
