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
#include <emmintrin.h>
#include <sys/time.h>

pthread_mutex_t mutex_sum;
double sum = 0;
int NB_THREADS = 4;

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
};

double rnorm(float *U, int n) {
    double res = 0;
    int i;
    for (i = 0; i < n; i++){
        res += sqrt(U[i]);
    }
    return res;
}

/*
double vect_rnorm (float *U, int n) {
    int i;
    int j = n % 4;
    // We fill U with zeros to have n % 4 == 0
    for (i = 0; i < j; i ++){
        U[n+i] = 0;
    }
    n += j;
    double res = 0;
    double* res_vec;
    int nb_iters = n / 4;
    __m128 *ptr = (__m128*)U;
    for(i = 0; i < nb_iters; i++, ptr++, U += 4) {
        _mm_store_ps(U, _mm_sqrt_ps(*ptr));
    }
    for (i = 0; i < nb_iters; i ++){
        ptr[i] = _mm_add_ps(ptr[i],ptr[i]);
        ptr[i] = _mm_add_ps(ptr[i],ptr[i]);
        res_vec = (double*) &ptr[i];
        printf("Sum result is %e \n", res_vec[0]);
        res += res_vec[0];
    }
    return res;
} */

void *PartialSum(void *ti_array) {
    int i, l;
    long tid;
    float* tab;
    double result=0.0;
    struct ThreadInfo *ti;
    ti = (struct ThreadInfo *) ti_array;
    tid = (long)ti -> tid;
    tab = (float*)ti ->tab;
    l = (int)ti -> length;
    for(i = 0; i < l; i++)
    {
        result += tab[i];
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

    for(t = 0; t < nb_threads; t++) {
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

int main(){
    // Initialize U
    int n = 1024;
    int i;
    float U[n];
    for (i = 0; i < n; i++){
        U[i] = rand_0_1();
    }
    
    double res_ref, res_vec, res_par, res_par_vec;
    double time_ref, time_vec, time_par, time_par_vec;
    double acc_vec, acc_par, acc_vec_par;
        
    // Reference
    time_ref = now();
    res_ref = rnorm(U,n);
    time_ref = now() - time_ref;
    
    //Parallel scalar
    time_par = now();
    rnormPar(U,n, NB_THREADS, 0);
    res_par = sum;
    time_par = now() - time_par;
        
    //Accelerations
    //acc_vec = time_ref / time_vec;
    acc_par = time_ref / time_par;
    //acc_vec_par = time_ref / time_vec_par;
    printf("VALEURS\nSéquentiel (scalaire: %e) Parallèle (nb_threads: %d, scalaire: %e) \n", res_ref, NB_THREADS, res_par);
    printf("TEMPS D'EXECUTION\nSéquentiel (scalaire: %e) Parallèle (nb_threads: %d, scalaire: %e) \n", time_ref, NB_THREADS, time_par);
    printf("Accélération (multithread: %e)", acc_par);
}
