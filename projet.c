//
//  projet.c
//  
//
//  Created by Alexiane on 27/2/22.
//

#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <immintrin.h>
#include <sys/time.h>

#define NB_THREADS 8

pthread_mutex_t mutex_sum;
double sum = 0;

float rand_0_1(void)
{
    return rand() / ((float) RAND_MAX);
}

double now(){
   // Retourne l'heure actuelle en secondes
   struct timeval t; double f_t;
   gettimeofday(&t, NULL);
   f_t = t.tv_usec; f_t = f_t/1000000.0; f_t +=t.tv_sec;
   return f_t;
}

struct ThreadInfo {
    long tid;
    float* tab;
    int length;
    int mode;
};

double rnorm(float *U, int n) {
    double res = 0;
    int i;
    for (i = 0; i < n; i++){
        res += sqrt(U[i]);
    }
    return res;
}


double vect_rnorm (float *U, int n) {
    // TODO: compute sqrt in double precision + more efficient sum ?
    int i;
    int j = n % 4;
    // We fill U with zeros to have n % 4 == 0
    // TODO: bad idea
    for (i = 0; i < j; i ++){
        printf("filling U\n");
        U[n+i] = 0;
    }
    n += j;
    double res = 0;
    float* res_vec;
    int nb_iters = n / 4;
    __m128* ptr = (__m128*)U;
    for(i = 0; i < nb_iters; i++) {
        ptr[i] = _mm_sqrt_ps(ptr[i]);
        res_vec = (float*) &ptr[i];
        res = res + (double) res_vec[0] + (double) res_vec[1] + (double) res_vec[2] + (double) res_vec[3];
    }
    return res;
}

void *PartialSum(void *ti_array) {
    int i, l, mode;
    long tid;
    float* tab;
    double result=0.0;
    struct ThreadInfo *ti;
    ti = (struct ThreadInfo *) ti_array;
    tid = (long)ti -> tid;
    tab = (float*)ti ->tab;
    l = (int)ti -> length;
    mode = (int)ti -> mode;
    if (mode == 0){
        result = rnorm(tab, l);
    }
    else {
        result = vect_rnorm(tab, l);
    }
    pthread_mutex_lock(&mutex_sum);
    sum = sum + result;
    pthread_mutex_unlock(&mutex_sum);
    pthread_exit((void*) ti);
}

void rnormPar (float *U, int n, int nb_threads, int mode) {
    pthread_t thread[nb_threads]; pthread_attr_t attr;
    int rc;
    long t;
    void *status;
    int block_size = ceil(n / nb_threads);
       /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr); pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    struct ThreadInfo ti_array[n];
    if (mode != 0 && mode != 1) {
        printf("ERROR: mode should be 0 (scalar) or 1 (vectorial)");
        exit(-1);
    }
    for(t = 0; t < nb_threads; t++) {
        ti_array[t].mode = mode;
        ti_array[t].tid = t;
        ti_array[t].tab = (U + t*block_size);
        if (t == nb_threads - 1) {
            ti_array[t].length = n - t*block_size;
        }
        else {
            ti_array[t].length = block_size;
        }
        rc = pthread_create(&thread[t], &attr, PartialSum, (void *) &ti_array[t]);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
   /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);
    for(t = 0; t < nb_threads; t++) {
        rc = pthread_join(thread[t], &status);
        if (rc) {
            printf("ERROR: pthread_join() is %d\n", rc);
            exit(-1);
        }
    }
       /* All the threads have terminated, now we are done */
}

// function to copy an array
void copyArray(float arr[], float copy[], int size)
{
  // loop to iterate through array
  for (int i = 0; i < size; ++i)
  {
    copy[i] = arr[i];
  }
}

int main(){
    printf("hello world\n");
    // Initialize U
    int n = 1024*1024;
    int i;
    float *U = (float*) malloc(n * sizeof(float));
    if(U == NULL) {
    printf("Error! memory not allocated.");
    exit(0);
    }
    
    for (i = 0; i < n; i++){
        U[i] = rand_0_1();
    }
    
    // Copy U
    float U_vec[n];
    copyArray(U,U_vec,n);
    float U_par[n];
    copyArray(U,U_par,n);
    float U_par_vec[n];
    copyArray(U,U_par_vec,n);
    
    double res_ref, res_vec, res_par, res_par_vec;
    double time_ref, time_vec, time_par, time_par_vec;
    double acc_vec, acc_par, acc_par_vec;
        
    // Reference
    time_ref = now();
    res_ref = rnorm(U,n);
    time_ref = now() - time_ref;
    printf("piche1\n");
    //Parallel scalar
    time_par = now();
    rnormPar(U_par,n, NB_THREADS, 0);
    res_par = sum;
    time_par = now() - time_par;
    printf("piche2\n");

    //Reinitialize mutex
    pthread_mutex_lock(&mutex_sum);
    sum = 0;
    pthread_mutex_unlock(&mutex_sum);
    
    //Vectorial
    time_vec = now();
    res_vec = vect_rnorm(U_vec,n);
    time_vec = now() - time_vec;
    printf("piche3\n");

    
    //Parallel + Vectorial
    time_par_vec = now();
    rnormPar(U_par_vec,n, NB_THREADS, 1);
    res_par_vec = sum;
    time_par_vec = now() - time_par_vec;

    //Accelerations
    acc_vec = time_ref / time_vec;
    acc_par = time_ref / time_par;
    acc_par_vec = time_ref / time_par_vec;
    
    printf("VALEURS\nSequentiel (scalaire: %2.e, vectoriel: %2.e) Parallele (nb_threads: %d, scalaire: %2.e) \n", res_ref, res_vec, NB_THREADS, res_par);
    printf("TEMPS D'EXECUTION\nSequentiel (scalaire: %2e, vectoriel: %2e) Parallele (nb_threads: %d, scalaire: %e) \n", time_ref, time_vec, NB_THREADS, time_par);
    printf("Acceleration (vectoriel: %e, multithread: %e)\n", acc_vec, acc_par);
    free(U);
    fflush(stdout);

}
