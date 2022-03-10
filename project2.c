// Example de code multithreadé pour le calcul de rnorme. 
/*

 gcc -o project proje.c -lpthread 

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h> // for timing
#include <math.h> // for sqrt
#include <stdbool.h>
#include <immintrin.h>

#define NB_THREADS 8
#define N 1048576 // 1024*1024

float U[N] __attribute__((aligned(32))); // Align the array in memory for vetorized computations
double sum = 0;

/* Time function */
double now(){
   struct timeval t; double f_t;
   gettimeofday(&t, NULL); 
   f_t = t.tv_usec; f_t = f_t/1000000.0; f_t +=t.tv_sec;
   return f_t; 
}

/* Init function for U*/
float rand_0_1(void)
{
    return rand() / ((float) RAND_MAX);
}


double rnorm(float *U, int n) {
    double res = 0;
    int i;
    for (i = 0; i < n; i++){
        res += sqrt(U[i]);
    }
    return res;
}

double vec_rnorm(float *u, int n) {
    double res = 0;
    int i;
    double res_list[4] __attribute__((aligned(32))) = {}; // registre size is supposed to be 256, ie 4 double. This array will be used for 4 double which will be temporary results.
    __m256d *reg_res_list = (__m256d*) res_list; // Pointer towards the register of size 1 which contains the list of 4 results. Here there is 2 pointers on the same array, one vectoriel (size 1) and one scalar (size 4)
    __m128 *reg_ptr = (__m128*)u; 

    for (i = 0; i < (n/4); i++){

        reg_res_list[0] = _mm256_add_pd(_mm256_sqrt_pd(_mm256_cvtps_pd(reg_ptr[i])), reg_res_list[0]); // The sum is one in place, and vectorized
    }

    for (i=0; i<4;i++) {res += res_list[i];} // We use the double pointer to access the the 4 double results which are stored in an unique vectoriel register, pointed at by reg_res_list. 

    return res;
}

struct thread_data{
  float *u;
  long a;
  long b;
  bool mode;
};

/* Varibles related to threads */
struct thread_data thread_data_array[NB_THREADS];
pthread_t thread_ptr[NB_THREADS];
pthread_mutex_t mutex_sum;

/*-------------------------------------------------------------*/
/* This is the thread version of the selected code portion     */
/*-------------------------------------------------------------*/
void *thread_function(void *threadarg){
  /* Local variables */
  double s;
  /* Shared variables correspondances */
  float *u;
  long a, b;
  bool mode;

  /* Association between shared variables and their correspondances */
  struct thread_data *thread_pointer_data;
  thread_pointer_data = (struct thread_data *)threadarg; 
  /* Shared variables */
  u = thread_pointer_data->u;
  a = thread_pointer_data->a;
  b = thread_pointer_data->b;
  mode = thread_pointer_data -> mode; 

  /* Body of the thread */
  if (mode){
    s = vec_rnorm(u+a, b-a);
  }else{
    s = rnorm(u+a, b-a);
  }

  pthread_mutex_lock(&mutex_sum);
  sum = sum + s;
  pthread_mutex_unlock(&mutex_sum);
  pthread_exit(NULL);
  return 0;
  }

/*-------------------------------------------------------------*/
/*                thread factory                               */
/*-------------------------------------------------------------*/

double rnormPar(float *U, int n, int nb_threads, bool mode){
long i;
double s;
int rc;


pthread_mutex_lock(&mutex_sum);
sum = 0;// reset the mutex
pthread_mutex_unlock(&mutex_sum);

/* Create and launch threads */
for(i=0;i<nb_threads;i++){
  /* Prepare data for this thread */
  thread_data_array[i].u = U;
  thread_data_array[i].a = i*(n/nb_threads);
  thread_data_array[i].b = (i+1)*(n/nb_threads);
  thread_data_array[i].mode = mode;
  /* Create and launch this thread */
  rc = pthread_create(&thread_ptr[i], NULL, thread_function, (void *) &thread_data_array[i]);
  if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
}
/* Wait for every thread to complete  */
for(i=0;i<nb_threads;i++){
  rc =pthread_join(thread_ptr[i], NULL);
  if (rc) {
            printf("ERROR: pthread_join() is %d\n", rc);
            exit(-1);
        }
}
pthread_mutex_lock(&mutex_sum);
s = sum;
pthread_mutex_unlock(&mutex_sum);
return s;
}
/* -------------------------------------------------------------------*/

int main(int argc, char *argv[]){

  int nb_repeat = 100;
  long i;
  double t0,t1 ;
  double res_ref, res_vec, res_par, res_par_vec;
  double time_ref, time_vec, time_par, time_vec_par;
  double acc_vec, acc_par, acc_par_vec;

  for(i=0;i<N;i++) U[i] = rand_0_1();

  time_ref = 0;
  for(i=0;i<nb_repeat;i++) {t0=now();res_ref = rnorm(U,N); t1=now(); time_ref += t1-t0;} 
  time_ref /= nb_repeat;
  //printf("Scalar S = %8.6f T = %6.4f\n",res_ref, time_ref); 

  time_vec = 0;
  for(i=0;i<nb_repeat;i++) {t0=now();res_vec = vec_rnorm(U,N); t1=now(); time_vec += t1-t0;}
  time_vec /= nb_repeat;
  //printf("Vectoriel S = %8.6f T = %6.4f\n",res_vec ,time_vec);

  time_par = 0;
  for(i=0;i<nb_repeat;i++) {t0=now();res_par = rnormPar(U,N,NB_THREADS, false); t1=now(); time_par += t1-t0;} 
  time_par /= nb_repeat;
  //printf("Multi-thread scalar (nb_threads: %d) S = %8.6f T = %6.4f\n", NB_THREADS, res_par, time_par);

  time_vec_par = 0; 
  for(i=0;i<nb_repeat;i++) {t0=now();res_par_vec = rnormPar(U,N,NB_THREADS, true); t1=now(); time_vec_par += t1-t0;} 
  time_vec_par /= nb_repeat;
  //printf("Multi-thread vectoriel (nb_threads: %d) S = %8.6f T = %6.4f\n", NB_THREADS, res_par_vec, time_vec_par);

  //Accelerations
  acc_vec = time_ref / time_vec;
  acc_par = time_ref / time_par;
  acc_par_vec = time_ref / time_vec_par;

  printf("VALEURS\nSequentiel (scalaire: %e, vectoriel: %e) Parallele (nb_threads: %d, scalaire: %e, vectoriel: %e) \n", res_ref, res_vec, NB_THREADS, res_par, res_par_vec);
  printf("TEMPS D'EXECUTION\nSequentiel (scalaire: %e, vectoriel: %e) Parallele (nb_threads: %d, scalaire: %e, , vectoriel: %e) \n", time_ref, time_vec, NB_THREADS, time_par, time_vec_par);
  printf("Acceleration (vectoriel: %e, multithread: %e,  vectoriel + multithread : %e)\n", acc_vec, acc_par, acc_par_vec);
  return 0;
}

////////////////////////////////////////////////////