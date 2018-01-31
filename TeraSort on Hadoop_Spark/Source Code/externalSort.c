#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

int NUM_FILE_PARTITIONS  = 10;
long PARTITION_SIZE = 10;
int THREAD_COUNT=2;
int OUT_FILE_NUM=0;
int PROG_TYPE=1;
char **INPUT_FILES=NULL;
int FILE_COUNT=0;
struct arg_struct {
    int arg1;
    char **arg2;
};
pthread_mutex_t lock;

//function to clear junk files
void clearFiles() {
    int i;
    for (i=0;i<OUT_FILE_NUM;i++) {
        char file[10]="";char str[5];
        sprintf(str, "%d", i);
        strcat(file, "out");
        strcat(file, str);
	strcat(file, ".dat");
        if(access(file,F_OK ) != -1 ) {
            remove(file);
        }      
    }
}

//function to merge two sorted files
char *mergingFiles(char *file1,char *file2) {
    char *eof1;char *eof2;
    char *line1=(char*) malloc(100);
    char *line2=(char*) malloc(100);
    char *file=(char*) malloc(sizeof(char) * 10);            
    sprintf(file, "out%d.dat", OUT_FILE_NUM);
    FILE *fp3 = fopen(file,"w");
    FILE *fp1 = fopen(file1,"r");
    FILE *fp2 = fopen(file2,"r");
    OUT_FILE_NUM++;
    eof1=fgets(line1,100,fp1);
    eof2=fgets(line2,100,fp2);
    while (eof1 && eof2) {
        char* first=(char*) malloc(10);
        char* second=(char*) malloc(10);
        sprintf(first,"%.10s",line1);
        sprintf(second, "%.10s",line2);	
	//memcpy(first,line1,10);
        //memcpy(second,line2,10);
        if (strcmp(first,second)<0 || strcmp(first,second)==0) {
            fprintf(fp3,"%s",line1);
            eof1=fgets(line1,100,fp1);
        } else {
            fprintf(fp3,"%s",line2);
            eof2=fgets(line2,100,fp2);
        }
    free(first);
    free(second);
    }
    if (eof1) {
        while (eof1) {
            fprintf(fp3,"%s",line1);
            eof1=fgets(line1,100,fp1);
        }
    } else {
        while (eof2) {
            fprintf(fp3,"%s",line2);
            eof2=fgets(line2,100,fp2);
        }
    }
    fclose(fp3);
    fclose(fp2);
    fclose(fp1);
    free(line1);
    free(line2);
    remove(file1);
    remove(file2);
    return file;
}

//function that sends two files to function mergingFiles from available sorted files
void mergeFiles(int partitions,char *in_files[]) {
    int i,j=0;
    int file_count = (partitions/2) + (partitions%2);
    char *Next_in_file[file_count];
    for (i=0;i<partitions;i+=2) {    
        if ((i+1)<partitions) {
            Next_in_file[j] = mergingFiles(in_files[i],in_files[i+1]);
        } else {
            Next_in_file[j]=in_files[i];
        }        
        j++;
    }
    j--;
    if (j > 0) {
        mergeFiles(j+1,Next_in_file);      
    } else {
        INPUT_FILES[FILE_COUNT]=Next_in_file[j];
        FILE_COUNT++;              
    }    
}

void *mergetwofiles(void *arg) { 
    pthread_mutex_lock(&lock);
    struct arg_struct *args = (struct arg_struct *)arg;
    mergeFiles(args -> arg1,args -> arg2);
    pthread_mutex_unlock(&lock);    
    pthread_exit(NULL);    
}

//This function allocates available sorted files to each thread to perform merge  
void mergeInputFiles() {
    int individual_array_size=NUM_FILE_PARTITIONS/THREAD_COUNT;
    char ***array = (char***)malloc((NUM_FILE_PARTITIONS) * sizeof(char**));
    struct arg_struct args[THREAD_COUNT];
    pthread_t thread_id[THREAD_COUNT];
    int thread[THREAD_COUNT]; int i;
    for (i=0;i<THREAD_COUNT;i++) {
        if(i==(THREAD_COUNT-1)) {
            individual_array_size=NUM_FILE_PARTITIONS-((NUM_FILE_PARTITIONS/THREAD_COUNT)*i);
        }
        array[i] = (char**)malloc(individual_array_size * sizeof(char*));
        int k;
        for(k = 0; k < individual_array_size; k++){
            array[i][k] = (char*) malloc(sizeof(char) * k * 10);
        }           
        int l,j=(NUM_FILE_PARTITIONS/THREAD_COUNT)*i;
        for(l=0;l<individual_array_size;l++) {
            array[i][l]=INPUT_FILES[j];
            j++;
        }
        args[i].arg1 = individual_array_size;
        args[i].arg2 = array[i];
        thread[i] = pthread_create(&(thread_id[i]),NULL,mergetwofiles,(void *)&args[i]);
    }        
    int k;
    for (k=0;k<THREAD_COUNT;k++) {
      pthread_join(thread_id[k], NULL);
    }
    pthread_mutex_destroy(&lock);
    free(array);
}

void generateOutput() {
    FILE *fp1=fopen(INPUT_FILES[FILE_COUNT-1], "r");
    char *file=(char*) malloc(sizeof(char) * 20);   
    if(PROG_TYPE == 1) {
        file="outputfile_128GB.dat";
    } else if (PROG_TYPE == 2) {
        file="outfile_1TB.dat";
    } else {
        file="outfile_1TB_8nodes.dat";
    }
    FILE *fp2=fopen(file, "w");
    char *a=(char*) malloc(sizeof(char) * 100);
	size_t linesize=100;
	if (fp2 != NULL) {
	    while (getline(&a, &linesize, fp1) != -1 && *a != '\n') {
		fputs(a, fp2);
	    }
	}
    printf("Final sorted records are in the file %s\n",file);
    free(a);
    free(INPUT_FILES);
}

//this function merges all the sorted files generated by each thread into one single file
void mergeIntoOnefile() {
    char **FINAL_FILES = (char**)malloc(THREAD_COUNT * sizeof(char*));
    int k;
    for(k = 0; k < THREAD_COUNT; k++){
        FINAL_FILES[k] = (char*) malloc(sizeof(char) * 10);
    }
    int i;
    int j=FILE_COUNT-THREAD_COUNT;
    for (i=0;i<THREAD_COUNT;i++) {
        FINAL_FILES[i]=INPUT_FILES[j];
        j++;
    }
    mergeFiles(THREAD_COUNT,FINAL_FILES);
    int x;
    for(x = 0; x < THREAD_COUNT; x++) {
		free(FINAL_FILES[x]);
	}
    free(FINAL_FILES);
}

void merge(char *array[],int low,int mid,int high) {
    int low1,low2,i;char *temp[high];
    for(low1=low,low2=mid+1,i=low; low1 <= mid && low2 <= high; i++) {
        char* first=(char*) malloc(10);
        char* second=(char*) malloc(10);
        //memcpy(first,array[low1],10);
        //memcpy(second,array[low2],10);
	sprintf(first, "%.10s", array[low1]);
	sprintf(second, "%.10s", array[low2]);
        if(strcmp(first,second)<0 || strcmp(first,second)==0) {
            temp[i]=array[low1++];
        } else {
            temp[i]=array[low2++];
        }
	free(first);
        free(second); 
    }
    while(low1 <= mid)    
        temp[i++]=array[low1++];

    while(low2 <= high)   
        temp[i++]=array[low2++];

    for(i=low; i <= high; i++)
        array[i]=temp[i];
}

void implementMergeSort(char *array[],int l,int r){
    if (l<r) {
        int m=(l+r)/2;
        implementMergeSort(array,l,m);
        implementMergeSort(array,m+1,r);
        merge(array,l,m,r);
    }
}

//this function sorts array elements and writes them into a file
void sortArrays(char *array[],int array_size) {
    implementMergeSort(array,0,array_size-1);
    char *file=(char*) malloc(sizeof(char) * 10);
    sprintf(file, "out%d.dat", OUT_FILE_NUM);
    OUT_FILE_NUM++;
    FILE *fpout = fopen(file, "w"); 
    int i;
    for(i=0;i<PARTITION_SIZE;i++){
        fprintf(fpout,"%s",array[i]);	
    }
    fclose(fpout);  
    INPUT_FILES[FILE_COUNT]=file;
    FILE_COUNT++;  
}

//this function splits input files into multiple files
void splitInputFile() {
    char *file = (char*) malloc(sizeof(char) * 10);
    if(PROG_TYPE == 1) {
        file="inputfile_128GB.dat";
    } else {
        file="inputfile_1TB.dat";
    }
    FILE *fp=fopen(file, "r");
    int i;
   
    for (i=0;i<NUM_FILE_PARTITIONS;i++) {
	int x;
	char **input_array = (char**)malloc(PARTITION_SIZE * 100);
    	for(x = 0; x < NUM_FILE_PARTITIONS; x++) {
        	input_array[x] = (char*) malloc(100);
    	}
        int j;size_t linesize=100;
        for(j=0;j<PARTITION_SIZE;j++) {
            if(getline(&input_array[j],&linesize,fp)!=-1) {
                //skip;
            } else {
		break;
	    }
        }
        sortArrays(input_array,j);
        int z;
        for(z = 0; z < NUM_FILE_PARTITIONS; z++) {
        	free(input_array[z]);
    	}
	free(input_array);       
    }
    fclose(fp);  
}

void initializeData() {
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("mutex initialization failed\n");
        exit(0);
    }
    INPUT_FILES = (char**)malloc((NUM_FILE_PARTITIONS+THREAD_COUNT) * sizeof(char*));
    int i;
    for(i = 0; i < NUM_FILE_PARTITIONS; i++){
        INPUT_FILES[i] = (char*) malloc(sizeof(char) * 10);
    }
}

int main(int argc, char *argv[]) {
    int arg;
    while((arg=getopt(argc, argv, "n:p:t:s:")) != -1) {
        switch(arg) {
            case 'n':
            NUM_FILE_PARTITIONS=atoi(optarg);
            break;
            case 'p':
            PARTITION_SIZE=atoi(optarg);
            break;
            case 't':
            THREAD_COUNT=atoi(optarg);
            break;
            case 's':
            PROG_TYPE=atoi(optarg);
            break;
            default:
            break;
        }
    }

    initializeData();
    time_t begin,end;
    begin= time(NULL);
    
    splitInputFile();
    mergeInputFiles();
    mergeIntoOnefile();
    end = time(NULL);
    generateOutput();  
    if (PROG_TYPE == 1) {
        printf("Total time taken to sort 128GB data is %.6lf seconds\n",difftime(end, begin));
    } else if(PROG_TYPE == 2){
        printf("Total time taken to sort 1TB data is %.6lf seconds\n",difftime(end, begin));
    } else {
        printf("Total time taken to sort 1TB data on 8 nodes is %.6lf seconds\n",difftime(end, begin));
    }
    clearFiles();
}