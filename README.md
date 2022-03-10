Project of Mines Paris' course 'Architecture materiel et logiciel'
==============================

**Authors**: Alexiane Laude, Arthur Pignet

The aim of this project is to implement a vectorized and multi-threaded function in C, to compute as fast as possible (in double precision) the following sum: 

$$
d(U,n) ) \sum_{i=0}^{n-1} \sqrt{u_i}
$$

for a vector $U$ of $n$ real numbers (single precision). 

Project compilation
------------

To compile the project using gcc run:

```bash
gcc  -lpthread -mavx2 -O3 -o project projet2.c 
```
 
and then execute `project.exe`

Result example
---------------

```bash
VALEURS
Sequentiel (scalaire: 6.991246e+005, vectoriel: 6.991246e+005) Parallele (nb_threads: 8, scalaire: 6.991246e+005, vectoriel: 6.991246e+005)
TEMPS D EXECUTION
Sequentiel (scalaire: 6.598651e-003, vectoriel: 1.093769e-003) Parallele (nb_threads: 8, scalaire: 2.031269e-003, , vectoriel: 7.812715e-004)
Acceleration (vectoriel: 6.032948e+000, multithread: 3.248536e+000,  vectoriel + multithread : 8.446042e+000)
```
