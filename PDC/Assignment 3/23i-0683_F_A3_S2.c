#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define damp    0.85
#define EPS     1e-7
#define MAXITER 500
#define GRAPHFILE "web-Google.txt"

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int myid, nproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    int N = 0;
    int E = 0;


    int* g_outdeg  = NULL;
    int* g_instart = NULL;
    int* g_inadj   = NULL;


    int* sc_od = NULL, *dp_od = NULL;
    int* sc_ih = NULL, *dp_ih = NULL;
    int* sc_ia = NULL, *dp_ia = NULL;
    int* flat_instart = NULL;

    if(myid == 0)
    {
        FILE* f = fopen(GRAPHFILE, "r");
        if(!f){ printf("cant open %s\n", GRAPHFILE); MPI_Abort(MPI_COMM_WORLD, 1); }

        char line[256];
        int maxv = -1, ecnt = 0;
        while(fgets(line, 256, f)){
            if(line[0]=='#') continue;
            int u, v;
            if(sscanf(line, "%d %d", &u, &v) == 2){
                if(u > maxv) maxv = u;
                if(v > maxv) maxv = v;
                ecnt++;
            }
        }
        rewind(f);
        N = maxv + 1;
        E = ecnt;

        int* srcs = (int*)malloc((size_t)E * sizeof(int));
        int* dsts = (int*)malloc((size_t)E * sizeof(int));
        g_outdeg  = (int*)calloc((size_t)N, sizeof(int));

        int k = 0;
        while(fgets(line, 256, f)){
            if(line[0]=='#') continue;
            int u, v;
            if(sscanf(line, "%d %d", &u, &v) == 2){
                srcs[k] = u; dsts[k] = v;
                g_outdeg[u]++;
                k++;
            }
        }
        fclose(f);


        g_instart = (int*)calloc((size_t)(N + 1), sizeof(int));
        for(int i = 0; i < E; i++) g_instart[dsts[i]+1]++;
        for(int i = 0; i < N; i++) g_instart[i+1] += g_instart[i];

        g_inadj = (int*)malloc((size_t)E * sizeof(int));
        int* pos = (int*)malloc((size_t)N * sizeof(int));
        memset(pos, 0, (size_t)N * sizeof(int));
        for(int i = 0; i < E; i++){
            int d = dsts[i];
            g_inadj[g_instart[d] + pos[d]] = srcs[i];
            pos[d]++;
        }
        free(pos); free(srcs); free(dsts);
        printf("rank0 loaded N=%d E=%d\n", N, E);


        sc_od = (int*)malloc((size_t)nproc * sizeof(int));
        dp_od = (int*)malloc((size_t)nproc * sizeof(int));
        sc_ih = (int*)malloc((size_t)nproc * sizeof(int));
        dp_ih = (int*)malloc((size_t)nproc * sizeof(int));
        sc_ia = (int*)malloc((size_t)nproc * sizeof(int));
        dp_ia = (int*)malloc((size_t)nproc * sizeof(int));

        int total_ih = 0;
        for(int r = 0; r < nproc; r++){
            int base2  = N / nproc;
            int extra2 = N % nproc;
            int rs = r*base2 + (r < extra2 ? r : extra2);
            int re = rs + base2 + (r < extra2 ? 1 : 0);
            int rn = re - rs;

            sc_od[r] = rn;       dp_od[r] = rs;
            sc_ih[r] = rn + 1;   dp_ih[r] = total_ih;
            total_ih += rn + 1;
            sc_ia[r] = g_instart[re] - g_instart[rs];
            dp_ia[r] = g_instart[rs];
        }


        flat_instart = (int*)malloc((size_t)total_ih * sizeof(int));
        int pos2 = 0;
        for(int r = 0; r < nproc; r++){
            int base2  = N / nproc;
            int extra2 = N % nproc;
            int rs = r*base2 + (r < extra2 ? r : extra2);
            int re = rs + base2 + (r < extra2 ? 1 : 0);
            int rn = re - rs;
            for(int i = 0; i <= rn; i++)
                flat_instart[pos2++] = g_instart[rs+i] - g_instart[rs];
        }
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);


    int base    = N / nproc;
    int extra   = N % nproc;
    int mystart = myid * base + (myid < extra ? myid : extra);
    int myend   = mystart + base + (myid < extra ? 1 : 0);
    int mycount = myend - mystart;


    int* my_od = (int*)malloc((size_t)mycount * sizeof(int));
    MPI_Scatterv(g_outdeg, sc_od, dp_od, MPI_INT,
                 my_od, mycount, MPI_INT, 0, MPI_COMM_WORLD);


    int* my_is = (int*)malloc((size_t)(mycount + 1) * sizeof(int));
    MPI_Scatterv(flat_instart, sc_ih, dp_ih, MPI_INT,
                 my_is, mycount+1, MPI_INT, 0, MPI_COMM_WORLD);


    int my_ne  = my_is[mycount];
    int* my_ia = (int*)malloc((size_t)(my_ne > 0 ? my_ne : 1) * sizeof(int));
    MPI_Scatterv(g_inadj, sc_ia, dp_ia, MPI_INT,
                 my_ia, my_ne, MPI_INT, 0, MPI_COMM_WORLD);


    if(myid == 0){
        free(g_outdeg); free(g_instart); free(g_inadj);
        free(flat_instart);
        free(sc_od); free(dp_od);
        free(sc_ih); free(dp_ih);
        free(sc_ia); free(dp_ia);
    }


    int num_internal = 0, num_boundary = 0;
    int* bound_flag = (int*)calloc((size_t)mycount, sizeof(int));
    for(int lv = 0; lv < mycount; lv++){
        for(int ei = my_is[lv]; ei < my_is[lv+1]; ei++){
            int nb = my_ia[ei];
            if(nb < mystart || nb >= myend){
                bound_flag[lv] = 1;
                break;
            }
        }
        if(bound_flag[lv]) num_boundary++;
        else num_internal++;
    }
    if(myid == 0)
        printf("rank0  internal=%d  boundary=%d\n", num_internal, num_boundary);


    int* ag_counts = (int*)malloc((size_t)nproc * sizeof(int));
    int* ag_displs = (int*)malloc((size_t)nproc * sizeof(int));
    for(int r = 0; r < nproc; r++){
        int b2 = N/nproc, e2 = N%nproc;
        int rs = r*b2 + (r < e2 ? r : e2);
        int re = rs + b2 + (r < e2 ? 1 : 0);
        ag_counts[r] = re - rs;
        ag_displs[r] = rs;
    }


    double* pr   = (double*)malloc((size_t)N * sizeof(double));
    double* mypr = (double*)malloc((size_t)mycount * sizeof(double));

    for(int i = 0; i < N; i++)       pr[i]   = 1.0 / N;
    for(int i = 0; i < mycount; i++) mypr[i]  = 1.0 / N;

    double tstart = MPI_Wtime();
    int iter2  = 0;
    double gdiff = 1.0;
    double tval  = (1.0 - damp) / N;

    while(iter2 < MAXITER && gdiff >= EPS)
    {

        for(int lv = 0; lv < mycount; lv++){
            double s = 0.0;
            for(int ei = my_is[lv]; ei < my_is[lv+1]; ei++){
                int nb = my_ia[ei];
                int od;
                if(nb >= mystart && nb < myend)
                    od = my_od[nb - mystart];
                else
                    od = 1;
                if(od == 0) od = 1;
                s += pr[nb] / od;
            }
            mypr[lv] = tval + damp * s;
        }


        double ldiff = 0.0;
        for(int lv = 0; lv < mycount; lv++){
            ldiff += fabs(mypr[lv] - pr[mystart + lv]);
            pr[mystart + lv] = mypr[lv];
        }


        MPI_Allgatherv(mypr, mycount, MPI_DOUBLE,
                       pr,   ag_counts, ag_displs, MPI_DOUBLE,
                       MPI_COMM_WORLD);

        MPI_Allreduce(&ldiff, &gdiff, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        iter2++;
    }

    double tend = MPI_Wtime();

    if(myid == 0){
        printf("\n=== Scenario 2  Collective (Allgatherv) ===\n");
        printf("processes : %d\n", nproc);
        printf("iterations: %d\n", iter2);
        printf("L1 diff   : %.4e\n", gdiff);
        printf("total time: %.4f s\n", tend - tstart);
        printf("per iter  : %.6f s\n", (tend - tstart) / iter2);

        double* sc2 = (double*)malloc((size_t)N * sizeof(double));
        int*    si2 = (int*)   malloc((size_t)N * sizeof(int));
        for(int i = 0; i < N; i++){ sc2[i] = pr[i]; si2[i] = i; }
        for(int k = 0; k < 10; k++){
            int best = k;
            for(int j = k+1; j < N; j++) if(sc2[j] > sc2[best]) best = j;
            double td = sc2[k]; sc2[k] = sc2[best]; sc2[best] = td;
            int    ti = si2[k]; si2[k] = si2[best]; si2[best] = ti;
        }
        printf("\nTop 10 nodes:\n");
        for(int k = 0; k < 10; k++)
            printf("  rank%2d  node=%-7d  pr=%.8f\n", k+1, si2[k], sc2[k]);
        free(sc2); free(si2);
    }

    free(pr); free(mypr);
    free(my_od); free(my_is); free(my_ia);
    free(bound_flag);
    free(ag_counts); free(ag_displs);

    MPI_Finalize();
    return 0;
}

