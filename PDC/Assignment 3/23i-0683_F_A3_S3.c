#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define DAMP    0.85
#define THOLD   1e-7
#define MAXITER 500
#define GRAPHFILE "web-Google.txt"

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int myrank, np;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    int totN = 0, totE = 0;

    int* g_od   = NULL;
    int* g_ih   = NULL;
    int* g_ia   = NULL;
    int* snd_od = NULL, *dsp_od = NULL;
    int* snd_ih = NULL, *dsp_ih = NULL;
    int* snd_ia = NULL, *dsp_ia = NULL;
    int* flt_ih = NULL;

    if(myrank == 0)
    {
        FILE* f = fopen(GRAPHFILE, "r");
        if(!f){ printf("cant open %s\n", GRAPHFILE); MPI_Abort(MPI_COMM_WORLD, 1); }

        char ln[256];
        int mx = -1, ec = 0;
        while(fgets(ln, 256, f)){
            if(ln[0]=='#') continue;
            int u, v;
            if(sscanf(ln, "%d %d", &u, &v) == 2){
                if(u > mx) mx = u;
                if(v > mx) mx = v;
                ec++;
            }
        }
        rewind(f);
        totN = mx + 1;
        totE = ec;

        int* sr = (int*)malloc((size_t)totE * sizeof(int));
        int* ds = (int*)malloc((size_t)totE * sizeof(int));
        g_od = (int*)calloc((size_t)totN, sizeof(int));

        int k = 0;
        while(fgets(ln, 256, f)){
            if(ln[0]=='#') continue;
            int u, v;
            if(sscanf(ln, "%d %d", &u, &v) == 2){
                sr[k] = u; ds[k] = v; g_od[u]++; k++;
            }
        }
        fclose(f);

        g_ih = (int*)calloc((size_t)(totN + 1), sizeof(int));
        for(int i = 0; i < totE; i++) g_ih[ds[i]+1]++;
        for(int i = 0; i < totN; i++) g_ih[i+1] += g_ih[i];
        g_ia = (int*)malloc((size_t)totE * sizeof(int));
        int* pp = (int*)malloc((size_t)totN * sizeof(int));
        memset(pp, 0, (size_t)totN * sizeof(int));
        for(int i = 0; i < totE; i++){
            int d = ds[i];
            g_ia[g_ih[d] + pp[d]] = sr[i];
            pp[d]++;
        }
        free(pp); free(sr); free(ds);
        printf("rank0: N=%d E=%d\n", totN, totE);

        snd_od = (int*)malloc((size_t)np * sizeof(int));
        dsp_od = (int*)malloc((size_t)np * sizeof(int));
        snd_ih = (int*)malloc((size_t)np * sizeof(int));
        dsp_ih = (int*)malloc((size_t)np * sizeof(int));
        snd_ia = (int*)malloc((size_t)np * sizeof(int));
        dsp_ia = (int*)malloc((size_t)np * sizeof(int));

        int tot_ih = 0;
        for(int r = 0; r < np; r++){
            int b = totN/np, ex = totN%np;
            int rs = r*b + (r < ex ? r : ex);
            int re = rs + b + (r < ex ? 1 : 0);
            int rn = re - rs;
            snd_od[r] = rn;       dsp_od[r] = rs;
            snd_ih[r] = rn + 1;   dsp_ih[r] = tot_ih;
            tot_ih += rn + 1;
            snd_ia[r] = g_ih[re] - g_ih[rs];
            dsp_ia[r] = g_ih[rs];
        }

        flt_ih = (int*)malloc((size_t)tot_ih * sizeof(int));
        int p2 = 0;
        for(int r = 0; r < np; r++){
            int b = totN/np, ex = totN%np;
            int rs = r*b + (r < ex ? r : ex);
            int re = rs + b + (r < ex ? 1 : 0);
            int rn = re - rs;
            for(int i = 0; i <= rn; i++)
                flt_ih[p2++] = g_ih[rs+i] - g_ih[rs];
        }
    }

    MPI_Bcast(&totN, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int b0  = totN / np;
    int ex0 = totN % np;
    int mys = myrank*b0 + (myrank < ex0 ? myrank : ex0);
    int mye = mys + b0 + (myrank < ex0 ? 1 : 0);
    int myn = mye - mys;

    int* l_od = (int*)malloc((size_t)myn * sizeof(int));
    MPI_Scatterv(g_od, snd_od, dsp_od, MPI_INT, l_od, myn, MPI_INT, 0, MPI_COMM_WORLD);

    int* l_ih = (int*)malloc((size_t)(myn + 1) * sizeof(int));
    MPI_Scatterv(flt_ih, snd_ih, dsp_ih, MPI_INT, l_ih, myn+1, MPI_INT, 0, MPI_COMM_WORLD);

    int  l_ne = l_ih[myn];
    int* l_ia = (int*)malloc((size_t)(l_ne > 0 ? l_ne : 1) * sizeof(int));
    MPI_Scatterv(g_ia, snd_ia, dsp_ia, MPI_INT, l_ia, l_ne, MPI_INT, 0, MPI_COMM_WORLD);

    if(myrank == 0){
        free(g_od); free(g_ih); free(g_ia); free(flt_ih);
        free(snd_od); free(dsp_od);
        free(snd_ih); free(dsp_ih);
        free(snd_ia); free(dsp_ia);
    }


    int* bflag     = (int*)calloc((size_t)myn, sizeof(int));
    int* need_rank = (int*)calloc((size_t)np,  sizeof(int));
    for(int lv = 0; lv < myn; lv++){
        for(int ei = l_ih[lv]; ei < l_ih[lv+1]; ei++){
            int nb = l_ia[ei];
            if(nb < mys || nb >= mye){
                bflag[lv] = 1;

                for(int r = 0; r < np; r++){
                    int b2 = totN/np, e2 = totN%np;
                    int rs = r*b2 + (r < e2 ? r : e2);
                    int re = rs + b2 + (r < e2 ? 1 : 0);
                    if(nb >= rs && nb < re){ need_rank[r] = 1; break; }
                }
            }
        }
    }


    int* all_needs = (int*)malloc((size_t)(np * np) * sizeof(int));
    MPI_Allgather(need_rank, np, MPI_INT, all_needs, np, MPI_INT, MPI_COMM_WORLD);


    int nsend = 0, nrecv = 0;
    for(int r = 0; r < np; r++){
        if(r != myrank && all_needs[r*np + myrank]) nsend++;
        if(r != myrank && need_rank[r])             nrecv++;
    }


    int* rc = (int*)malloc((size_t)np * sizeof(int));
    int* rd = (int*)malloc((size_t)np * sizeof(int));
    for(int r = 0; r < np; r++){
        int b2 = totN/np, e2 = totN%np;
        int rs = r*b2 + (r < e2 ? r : e2);
        int re = rs + b2 + (r < e2 ? 1 : 0);
        rc[r] = re - rs;
        rd[r] = rs;
    }

    double* pr   = (double*)malloc((size_t)totN * sizeof(double));
    double* mypr = (double*)malloc((size_t)myn  * sizeof(double));
    for(int i = 0; i < totN; i++) pr[i]   = 1.0 / totN;
    for(int i = 0; i < myn;  i++) mypr[i] = 1.0 / totN;

    MPI_Request* reqs = (MPI_Request*)malloc((size_t)(nsend + nrecv) * sizeof(MPI_Request));

    double time_comm = 0.0, time_comp = 0.0;
    double tstart = MPI_Wtime();
    int    iters  = 0;
    double gdiff  = 1.0;
    double tval2  = (1.0 - DAMP) / totN;

    while(iters < MAXITER && gdiff >= THOLD)
    {

        double tc0 = MPI_Wtime();
        int ri = 0;
        for(int r = 0; r < np; r++){
            if(r == myrank || !need_rank[r]) continue;
            int b2 = totN/np, e2 = totN%np;
            int rs = r*b2 + (r < e2 ? r : e2);
            int re = rs + b2 + (r < e2 ? 1 : 0);
            MPI_Irecv(pr + rs, re - rs, MPI_DOUBLE, r, 77, MPI_COMM_WORLD, &reqs[ri++]);
        }

        for(int r = 0; r < np; r++){
            if(r == myrank || !all_needs[r*np + myrank]) continue;
            MPI_Isend(pr + mys, myn, MPI_DOUBLE, r, 77, MPI_COMM_WORLD, &reqs[ri++]);
        }
        double tc1 = MPI_Wtime();
        time_comm += tc1 - tc0;


        double tk0 = MPI_Wtime();
        for(int lv = 0; lv < myn; lv++){
            if(bflag[lv]) continue;
            double s = 0.0;
            for(int ei = l_ih[lv]; ei < l_ih[lv+1]; ei++){
                int nb = l_ia[ei];
                int od = l_od[nb - mys];
                if(od == 0) od = 1;
                s += pr[nb] / od;
            }
            mypr[lv] = tval2 + DAMP * s;
        }


        double tw0 = MPI_Wtime();
        if(ri > 0) MPI_Waitall(ri, reqs, MPI_STATUSES_IGNORE);
        double tw1 = MPI_Wtime();
        time_comm += tw1 - tw0;


        for(int lv = 0; lv < myn; lv++){
            if(!bflag[lv]) continue;
            double s = 0.0;
            for(int ei = l_ih[lv]; ei < l_ih[lv+1]; ei++){
                int nb = l_ia[ei];
                int od;
                if(nb >= mys && nb < mye) od = l_od[nb - mys];
                else od = 1;
                if(od == 0) od = 1;
                s += pr[nb] / od;
            }
            mypr[lv] = tval2 + DAMP * s;
        }
        double tk1 = MPI_Wtime();
        time_comp += tk1 - tk0;


        double ldiff = 0.0;
        for(int lv = 0; lv < myn; lv++){
            ldiff += fabs(mypr[lv] - pr[mys + lv]);
            pr[mys + lv] = mypr[lv];
        }


        MPI_Allgatherv(mypr, myn, MPI_DOUBLE,
                       pr, rc, rd, MPI_DOUBLE, MPI_COMM_WORLD);

        MPI_Allreduce(&ldiff, &gdiff, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        iters++;
    }

    double tend = MPI_Wtime();

    double ov_pct = 0.0;
    if(time_comm + time_comp > 0.0)
        ov_pct = (time_comp / (time_comm + time_comp)) * 100.0;

    if(myrank == 0){
        printf("\n=== Scenario 3  Async Overlap ===\n");
        printf("processes       : %d\n", np);
        printf("iterations      : %d\n", iters);
        printf("L1 diff         : %.4e\n", gdiff);
        printf("total time      : %.4f s\n", tend - tstart);
        printf("per iter        : %.6f s\n", (tend - tstart) / iters);
        printf("comm time total : %.4f s\n", time_comm);
        printf("comp time total : %.4f s\n", time_comp);
        printf("overlap %%       : %.1f%%\n", ov_pct);

        double* sc3 = (double*)malloc((size_t)totN * sizeof(double));
        int*    si3 = (int*)   malloc((size_t)totN * sizeof(int));
        for(int i = 0; i < totN; i++){ sc3[i] = pr[i]; si3[i] = i; }
        for(int k = 0; k < 10; k++){
            int best = k;
            for(int j = k+1; j < totN; j++) if(sc3[j] > sc3[best]) best = j;
            double td = sc3[k]; sc3[k] = sc3[best]; sc3[best] = td;
            int    ti = si3[k]; si3[k] = si3[best]; si3[best] = ti;
        }
        printf("\nTop 10 nodes:\n");
        for(int k = 0; k < 10; k++)
            printf("  rank%2d  node=%-7d  pr=%.8f\n", k+1, si3[k], sc3[k]);
        free(sc3); free(si3);
    }

    free(pr); free(mypr); free(reqs);
    free(l_od); free(l_ih); free(l_ia);
    free(bflag); free(need_rank); free(all_needs);
    free(rc); free(rd);

    MPI_Finalize();
    return 0;
}

