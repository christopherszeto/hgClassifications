#include <ctype.h>
#include "common.h"
#include "NMF.h"
#include "../../inc/MLmatrix.h"

#define MIN_VAL 0.000000001

/**********************************************************************
** Copyright Christopher Szeto Sept. 2007
**
** This program reads two matrices and deconvolutes step wise from them.
**
************************************************************************/


//DEPRECATED CODE
///* Runs nmf "seeds" times, and averages the results. Returns matrix of residuals, and overwrites the H & W matrix with avg of these runs */
//matrix * multiWseed_nmf(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr,int seeds, int reps,int vary_w, double w_resistance, int vary_h, double h_resistance){
//    	int i,j,x;
//    	matrix * avgW_ptr = init_matrix(W_ptr->rows, W_ptr->cols);
//	matrix * avgH_ptr = init_matrix(H_ptr->rows, H_ptr->cols);
//	matrix * backupH_ptr = copy_matrix(H_ptr);
//    	double * residuals = malloc(reps * sizeof(double));
//
//	//init your various arrays:
//    	for(i = 0; i < reps; i++)
//        	residuals[i] = 0;
//    	matrix * ret = init_matrix(seeds,reps);
//    	for(i = 0; i < seeds;i++){
//        	for(j = 0; j < reps; j++){
//            		ret->graph[i][j] = NULL_FLAG;
//        	}
//    	}
//	
//	//iterate over seeds, calcing NMF results
//    	for(x = 0; x < seeds; x++){
//        	r_fill_matrix(W_ptr,0,1);
//		scale_matrix(W_ptr, 2, 1);
//		H_ptr = copy_matrix(backupH_ptr);
//        	for(i = 0; i < reps; i++)
//            		residuals[i] = 0;
//		fprintf(stderr, "Running seed %d/%d...\n", x+1, seeds);
//        	nmf(V_ptr,W_ptr,H_ptr,residuals,reps,vary_w,w_resistance,vary_h,h_resistance);
//		fprintf(stderr, "Completed seed %d\n", x+1);
//		add_matrices(W_ptr, avgW_ptr, avgW_ptr);
//		add_matrices(H_ptr, avgH_ptr, avgH_ptr);
//        	for(i = 0; i < reps; i++)
//            		ret->graph[x][i] = residuals[i];
//    	}
//    	/*scale the results matrix by number of seeds*/
//    	for(i = 0; i < W_ptr->rows;i++){
//        	for(j = 0; j < W_ptr->cols; j++){
//			if(avgW_ptr->graph[i][j] != NULL_FLAG)
//            			W_ptr->graph[i][j] = (avgW_ptr->graph[i][j]/seeds);
//			else
//				W_ptr->graph[i][j] = NULL_FLAG;
//        	}
//    	}
//        for(i = 0; i < H_ptr->rows; i++){
//                for(j = 0; j < H_ptr->cols; j++){
//                        if(avgH_ptr->graph[i][j] != NULL_FLAG)
//                                H_ptr->graph[i][j] = (avgH_ptr->graph[i][j]/seeds);
//                        else
//                                H_ptr->graph[i][j] = NULL_FLAG;
//                }
//        }
//    	free_matrix(avgW_ptr);
//	free_matrix(avgH_ptr);
//	free_matrix(backupH_ptr);
//	free(residuals);
//    	return ret;
//}
//
///* Runs nmf "seeds" times, and averages the results. Returns the matrix of residuals and overwrites H matrix with average H from runs */
//matrix * multiHseed_nmf(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, int seeds, int reps,int vary_w,double w_resistance,int vary_h, double h_resistance){
//    	int i,j,x;
//    	matrix * ret = init_matrix(seeds,reps);
//    	matrix * avgH_ptr = init_matrix(H_ptr->rows,H_ptr->cols);
//	matrix * backupW_ptr = copy_matrix(W_ptr);
//	
//    	double *residuals = malloc(reps * sizeof(double));
//
//    	//set up the table of values from V for filling H randomly
//    	double table[(V_ptr->cols*V_ptr->rows)];
//    	for(i = 0; i < V_ptr->rows; i++){
//        	for(j = 0; j < V_ptr->cols; j++){
//            		table[j+(i*V_ptr->cols)] = V_ptr->graph[i][j];
//        	}
//    	}
//    
//    	//iteratively do nmf over "seeds"
//    	for(x = 0; x < seeds; x++){
//        	r_fill_matrix_from_table(H_ptr, table, V_ptr->rows * V_ptr->cols);
//		W_ptr = copy_matrix(backupW_ptr);
//        	for(i = 0; i < reps; i++)
//            		residuals[i] = 0;
//		fprintf(stderr, "Running seed %d/%d...\n", x+1, seeds);
//        	nmf(V_ptr,W_ptr,H_ptr,residuals,reps,vary_w,w_resistance,vary_h,h_resistance); 
//		fprintf(stderr, "Completed seed %d\n", x+1);
//        	for(i = 0; i < H_ptr->rows; i++){
//            		for(j = 0; j < H_ptr->cols; j++){
//				if(H_ptr->graph[i][j] != NULL_FLAG)
//                			avgH_ptr->graph[i][j] += H_ptr->graph[i][j];
//            		}
//        	}
//        	for(i = 0; i < reps; i++)
//            		ret->graph[x][i] = residuals[i];
//    	}
//
//    	/*scale the results matrix by number of seeds & overwrite H*/
//    	for(i = 0; i < H_ptr->rows;i++){
//        	for(j = 0; j < H_ptr->cols; j++){
//			if(avgH_ptr->graph[i][j] != NULL_FLAG)
//            			H_ptr->graph[i][j] = (avgH_ptr->graph[i][j]/seeds);
//			else
//				H_ptr->graph[i][j] = NULL_FLAG;
//        	}
//    	}
//
//    	free_matrix(avgH_ptr);
//	free_matrix(backupW_ptr);
//	free(residuals);
//
//    	return ret;
//}


/*** nmf ***/
/*
** This does usual nmf, varying all columns in W and H (if requested) reps times.
*/
void nmf(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, double residuals[], int reps, int vary_w, double w_resistance, int vary_h, double h_resistance){
    	/*Housekeeping variables*/
    	int x = 0; 
    	matrix * WH_ptr= init_matrix(W_ptr->rows, H_ptr->cols);

    	/*** Do NMF iterative updating on W and/or H ***/
    	for(x = 0; x < reps; x++){
		//printProgress((double)(x+1)/reps);
        	if(vary_w)
            		updateW(V_ptr,W_ptr,H_ptr,WH_ptr,w_resistance);
			
        	if(vary_h)
            		updateH(V_ptr,W_ptr,H_ptr,WH_ptr,h_resistance);

		mult_matrices(W_ptr,H_ptr, WH_ptr);
        	residuals[x] = calc_residual(V_ptr, WH_ptr);
    	}
    	free_matrix(WH_ptr);
}

/*
** Calculates the residual between V and WH according to objective function. 
** As nmf proceeds this should settle to local maximum
*/
double calc_residual(matrix *V_ptr, matrix *WH_ptr){
    	double residual = 0;
    	int i, mu;
    	for(i = 0; i < V_ptr->rows; i++){
        	for(mu = 0; mu < V_ptr->cols; mu++){
			if(V_ptr->graph[i][mu] != NULL_FLAG){
            			if(WH_ptr->graph[i][mu] == 0){
                			residual += V_ptr->graph[i][mu]*log(MIN_VAL) - MIN_VAL; //catch divide by zero with very small value
            			}else{
                			residual += V_ptr->graph[i][mu]*log(WH_ptr->graph[i][mu]) - WH_ptr->graph[i][mu];
            			}
			}
        	}
    	}
    	return residual;
}

/* updateW
** 
** Multiplies W and H, then sums the ratios of the WH (guesses and clusters) matrix with V (the data). It 
** then changes the W matrix towards what the summed ratios suggest will make WH more like V. 
**
*/
void updateW(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, matrix *WH_ptr, double w_resistance){
    	int i = 0, a = 0, mu = 0;
    	double sum = 0, ratio = 0;
		mult_matrices(W_ptr, H_ptr, WH_ptr);
		matrix * new_W_ptr = copy_matrix(W_ptr);

    	/*For each position in W ... */
    	for(i = 0; i < W_ptr->rows; i++){
        	for(a = 0; a < W_ptr->cols; a++){
            	/*for all columns mu in WH...*/
            	for(mu = 0; mu < H_ptr->cols; mu++){
					if(V_ptr->graph[i][mu] != NULL_FLAG && WH_ptr->graph[i][mu] != NULL_FLAG){
                		if(WH_ptr->graph[i][mu] == 0){ //catch /0 error
                    		ratio = 1;    
                		}else{
                    		ratio = V_ptr->graph[i][mu]/WH_ptr->graph[i][mu]; 
                		}
                		/*Multiply by H at a, curr col*/
                		ratio = (ratio * H_ptr->graph[a][mu]);
                		/*Add to a sum*/
               			sum += ratio;
					}
            	}
        		/*Times the position i,a by the sum*/
				if(W_ptr->graph[i][a] != NULL_FLAG)
					new_W_ptr->graph[i][a] = W_ptr->graph[i][a] * (1+(sum-1)*w_resistance);
        		sum = 0;
       		}
    	}
        
    	/*Scale by column - This ensures the "vote table" aspect of the W matrix*/        
    	for(a = 0; a < W_ptr->cols; a++){
        	for(i = 0; i < W_ptr->rows; i++){
			if(W_ptr->graph[i][a] != NULL_FLAG)
            			sum += new_W_ptr->graph[i][a];
        	}
        	for(i = 0; i < W_ptr->rows; i++){
			if(W_ptr->graph[i][a] != NULL_FLAG)
            			W_ptr->graph[i][a] = (new_W_ptr->graph[i][a]/sum);
        	}    
        sum = 0;
    	}
		free_matrix(new_W_ptr);
}


/* updateH
**
** Only used when -vary_h option is added. This updates H in the same way as W is updated; WH is  calculated, and the 
** differences between WH and V are found by summing ratios at each point. H is then updated (NB: row by row, as opposed to 
** col by col with W) to make WH resemble V more. H is also not scaled, like W is after updating.
*/
void updateH(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, matrix *WH_ptr, double h_resistance){
    	int i = 0, a = 0, mu = 0;
    	double sum = 0;
		matrix * new_H_ptr = copy_matrix(H_ptr);
    	mult_matrices(W_ptr, H_ptr, WH_ptr);

    	for(a = 0; a < H_ptr->rows; a++){
        	for(mu = 0; mu < H_ptr->cols; mu++){
            	for(i = 0; i < W_ptr->rows; i++){
					if(V_ptr->graph[i][mu] != NULL_FLAG && WH_ptr->graph[i][a] != NULL_FLAG){
                		if(WH_ptr->graph[i][mu] == 0){ //catch /0 effect
                    		sum += W_ptr->graph[i][a];
                		}else{
                    		sum += ((V_ptr->graph[i][mu]/WH_ptr->graph[i][mu])*W_ptr->graph[i][a]);
						}
                	}
            	}
				if(H_ptr->graph[a][mu] !=  NULL_FLAG)
					new_H_ptr->graph[a][mu] = H_ptr->graph[a][mu]* (1+(sum-1)*h_resistance);
            	sum = 0;
        	}
    	}
		for(i = 0; i < H_ptr->rows; i++){
			for(a = 0; a < H_ptr->cols; a++){
				H_ptr->graph[i][a] = new_H_ptr->graph[i][a];
			}
		}
		free_matrix(new_H_ptr);
}


/*Contrain values in each column of a model - if value has fallen below 1/m remove its effect*/
void constrainSparsity(matrix *model){
	int i,a;
	for(a = 0; a < model->cols; a++){
		int nonZeroEntries = 0;
		for(i = 0; i < model->rows; i++){
			if(model->graph[i][a] != 0 && model->graph[i][a] != NULL_FLAG)
				nonZeroEntries++;
		}
		for(i = 0; i < model->rows; i++){
			if(model->graph[i][a] < 1/nonZeroEntries){
				model->graph[i][a] = 0;
			}
		}
	}
}

/*i is the fraction complete*/
void printProgress(double percentDone){
    	int j = 0;
    	//double percentDone = i*100;
    	if(percentDone == 0){
       		fprintf(stderr, "Progress:[");
       		for(j = 0; j <= 100; j++) fprintf(stderr," ");
        	fprintf(stderr, "]");
        	fflush(stderr);
    	}else if(percentDone <= 100){
        	fprintf(stderr,"\rProgress:[");
        	for(j = 0; j <= percentDone; j++) fprintf(stderr, "=");
        	for(j = j; j <= 100; j++) fprintf(stderr, " ");
        	fprintf(stderr,"]");
        	fflush(stderr);
    	}
}
