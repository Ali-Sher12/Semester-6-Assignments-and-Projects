#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define d_factor  0.85
#define thresh    1e-7
#define MAXITER   500
#define GRAPHFILE "web-Google.txt"

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int myrank, totalprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &totalprocs);


    int total_nodes = 0;
    int total_edges = 0;

    int* src_arr  = NULL;
    int* dst_arr  = NULL;
    int* outdeg   = NULL;
    int* in_start = NULL;
    int* in_edges = NULL;

    if(myrank == 0)
    {
        FILE* fp = fopen(GRAPHFILE, "r");
        if(!fp){ printf("cant open %s\n", GRAPHFILE); MPI_Abort(MPI_COMM_WORLD, 1); }

        char buf[300];
        int maxnode = -1;
        int ecnt = 0;


        while(fgets(buf, 300, fp)){
            if(buf[0] == '#') continue;
            int a, b;
            if(sscanf(buf, "%d %d", &a, &b) == 2){
                if(a > maxnode) maxnode = a;
                if(b > maxnode) maxnode = b;
                ecnt++;
            }
        }
        rewind(fp);

        total_nodes = maxnode + 1;
        total_edges = ecnt;

        src_arr = (int*)malloc((size_t)total_edges * sizeof(int));
        dst_arr = (int*)malloc((size_t)total_edges * sizeof(int));
        outdeg  = (int*)calloc((size_t)total_nodes, sizeof(int));

        int i = 0;
        while(fgets(buf, 300, fp)){
            if(buf[0] == '#') continue;
            int a, b;
            if(sscanf(buf, "%d %d", &a, &b) == 2){
                src_arr[i] = a;
                dst_arr[i] = b;
                outdeg[a]++;
                i++;
            }
        }
        fclose(fp);


        in_start = (int*)calloc((size_t)(total_nodes + 1), sizeof(int));
        for(int j = 0; j < total_edges; j++)
            in_start[ dst_arr[j] + 1 ]++;
        for(int j = 0; j < total_nodes; j++)
            in_start[j+1] += in_start[j];

        in_edges = (int*)malloc((size_t)total_edges * sizeof(int));
        int* tmp = (int*)malloc((size_t)total_nodes * sizeof(int));
        memset(tmp, 0, (size_t)total_nodes * sizeof(int));
        for(int j = 0; j < total_edges; j++){
            int dd = dst_arr[j];
            in_edges[ in_start[dd] + tmp[dd] ] = src_arr[j];
            tmp[dd]++;
        }
        free(tmp);
        free(src_arr);
        free(dst_arr);

        printf("rank0: loaded  nodes=%d  edges=%d\n", total_nodes, total_edges);
        fflush(stdout);
    }


    MPI_Bcast(&total_nodes, 1, MPI_INT, 0, MPI_COMM_WORLD);


    int chunk    = total_nodes / totalprocs;
    int leftover = total_nodes % totalprocs;

    int my_start = myrank * chunk + (myrank < leftover ? myrank : leftover);
    int my_end   = my_start + chunk + (myrank < leftover ? 1 : 0);
    int my_count = my_end - my_start;


    int* my_outdeg  = (int*)malloc((size_t)my_count * sizeof(int));
    int* my_instart = (int*)malloc((size_t)(my_count + 1) * sizeof(int));
    int  my_ne      = 0;
    int* my_inedges = NULL;

    if(myrank == 0)
    {

        for(int i = 0; i < my_count; i++)
            my_outdeg[i] = outdeg[my_start + i];

        my_ne = in_start[my_end] - in_start[my_start];
        my_inedges = (int*)malloc((size_t)(my_ne > 0 ? my_ne : 1) * sizeof(int));
        for(int i = 0; i <= my_count; i++)
            my_instart[i] = in_start[my_start + i] - in_start[my_start];
        for(int i = 0; i < my_ne; i++)
            my_inedges[i] = in_edges[ in_start[my_start] + i ];


        for(int r = 1; r < totalprocs; r++)
        {
            int rs = r * chunk + (r < leftover ? r : leftover);
            int re = rs + chunk + (r < leftover ? 1 : 0);
            int rn  = re - rs;
            int rne = in_start[re] - in_start[rs];

            MPI_Send(&rn,  1,  MPI_INT, r, 10, MPI_COMM_WORLD);
            MPI_Send(outdeg + rs, rn, MPI_INT, r, 11, MPI_COMM_WORLD);


            int* tmp2 = (int*)malloc((size_t)(rn + 1) * sizeof(int));
            for(int i = 0; i <= rn; i++)
                tmp2[i] = in_start[rs + i] - in_start[rs];

            MPI_Send(&rne, 1,    MPI_INT, r, 12, MPI_COMM_WORLD);
            MPI_Send(tmp2, rn+1, MPI_INT, r, 13, MPI_COMM_WORLD);
            free(tmp2);

            if(rne > 0)
                MPI_Send(in_edges + in_start[rs], rne, MPI_INT, r, 14, MPI_COMM_WORLD);
        }

        free(outdeg);
        free(in_start);
        free(in_edges);
    }
    else
    {
        int got_n, got_ne;
        MPI_Recv(&got_n,  1, MPI_INT, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(my_outdeg, got_n, MPI_INT, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&got_ne, 1, MPI_INT, 0, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(my_instart, my_count+1, MPI_INT, 0, 13, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        my_ne = got_ne;
        my_inedges = (int*)malloc((size_t)(my_ne > 0 ? my_ne : 1) * sizeof(int));
        if(my_ne > 0)
            MPI_Recv(my_inedges, my_ne, MPI_INT, 0, 14, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }


    int* is_boundary = (int*)calloc((size_t)my_count, sizeof(int));
    for(int lv = 0; lv < my_count; lv++){
        for(int ei = my_instart[lv]; ei < my_instart[lv+1]; ei++){
            int nb = my_inedges[ei];
            if(nb < my_start || nb >= my_end){
                is_boundary[lv] = 1;
                break;
            }
        }
    }


    double* pr     = (double*)malloc((size_t)total_nodes * sizeof(double));
    double* pr_new = (double*)malloc((size_t)total_nodes * sizeof(double));

    for(int i = 0; i < total_nodes; i++)
        pr[i] = 1.0 / total_nodes;


    double t1   = MPI_Wtime();
    int    iter = 0;
    double diff = 1.0;

    while(iter < MAXITER && diff >= thresh)
    {

        for(int r = 0; r < totalprocs; r++)
        {
            if(r == myrank){
                for(int dst_r = 0; dst_r < totalprocs; dst_r++){
                    if(dst_r == myrank) continue;
                    MPI_Send(pr + my_start, my_count, MPI_DOUBLE, dst_r, 20, MPI_COMM_WORLD);
                }
            } else {
                int rs2 = r * chunk + (r < leftover ? r : leftover);
                int re2 = rs2 + chunk + (r < leftover ? 1 : 0);
                MPI_Recv(pr + rs2, re2 - rs2, MPI_DOUBLE, r, 20, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }


        double base_val = (1.0 - d_factor) / total_nodes;

        for(int lv = 0; lv < my_count; lv++){
            int gv   = my_start + lv;
            double s = 0.0;
            for(int ei = my_instart[lv]; ei < my_instart[lv+1]; ei++){
                int nb = my_inedges[ei];
                int od;
                if(nb >= my_start && nb < my_end)
                    od = my_outdeg[nb - my_start];
                else
                    od = 1;
                if(od == 0) od = 1;
                s += pr[nb] / od;
            }
            pr_new[gv] = base_val + d_factor * s;
        }


        double local_d = 0.0;
        for(int lv = 0; lv < my_count; lv++){
            int gv  = my_start + lv;
            local_d += fabs(pr_new[gv] - pr[gv]);
            pr[gv]   = pr_new[gv];
        }

        MPI_Allreduce(&local_d, &diff, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        iter++;
    }

    double t2 = MPI_Wtime();


    double* all_pr = NULL;
    if(myrank == 0){
        all_pr = (double*)malloc((size_t)total_nodes * sizeof(double));
        for(int i = 0; i < my_count; i++)
            all_pr[my_start + i] = pr[my_start + i];
        for(int r = 1; r < totalprocs; r++){
            int rs3 = r * chunk + (r < leftover ? r : leftover);
            int re3 = rs3 + chunk + (r < leftover ? 1 : 0);
            MPI_Recv(all_pr + rs3, re3 - rs3, MPI_DOUBLE, r, 30, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Send(pr + my_start, my_count, MPI_DOUBLE, 0, 30, MPI_COMM_WORLD);
    }

    if(myrank == 0){
        printf("\n=== Scenario 1  P2P Blocking ===\n");
        printf("processes : %d\n", totalprocs);
        printf("iterations: %d\n", iter);
        printf("L1 diff   : %.4e\n", diff);
        printf("total time: %.4f s\n", t2 - t1);
        printf("per iter  : %.6f s\n", (t2 - t1) / iter);


        double* scopy = (double*)malloc((size_t)total_nodes * sizeof(double));
        int*    sidx  = (int*)   malloc((size_t)total_nodes * sizeof(int));
        for(int i = 0; i < total_nodes; i++){ scopy[i] = all_pr[i]; sidx[i] = i; }
        for(int k = 0; k < 10; k++){
            int best = k;
            for(int j = k+1; j < total_nodes; j++)
                if(scopy[j] > scopy[best]) best = j;
            double td = scopy[k]; scopy[k] = scopy[best]; scopy[best] = td;
            int    ti = sidx[k];  sidx[k]  = sidx[best];  sidx[best]  = ti;
        }
        printf("\nTop 10 nodes:\n");
        for(int k = 0; k < 10; k++)
            printf("  rank%2d  node=%-7d  pr=%.8f\n", k+1, sidx[k], scopy[k]);

        free(scopy); free(sidx); free(all_pr);
    }

    free(pr); free(pr_new);
    free(my_outdeg); free(my_instart); free(my_inedges);
    free(is_boundary);

    MPI_Finalize();
    return 0;
}

