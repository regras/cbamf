/*
    Code adapted from mrsh-v2 (https://www.fbreitinger.de/?page_id=218)

    Vitor Hugo Galhardo Moia, Ph.D studant
    Department of Computer Engineering and Industrial Automation (DCA)
    School of Electrical and Computer Engineering (FEEC)
    University of Campinas (UNICAMP)
    Campinas, SP, Brazil 13083-852
    Email: vhgmoia@dca.fee.unicamp.br / vitormoia@gmail.com
    page: http://www.dca.fee.unicamp.br/~vhgmoia/

    March, 28th 2019

    File: main.c
    Purpose: Extract mrsh-v2 features from objects and insert them into a database

    INPUT: see help.
    OUTPUT: None.
*/

/* 
 * File:   main.c
 * Author: Frank Breitinger
 * Created on 28. April 2013, 19:15
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <limits.h>		/* PATH_MAX */
#include "../header/main.h"
#include "../header/helper.h";
#include "../header/util.h";
#include <sqlite3.h> 


// Global variable for the different modes
MODES *mode;

extern char *optarg;
extern int optind;
int num_global_features = 0; //counting the number of features

#define SIZE_LINE 200

static void show_help (void) {
    printf ("\nmrsh-v2  by Frank Breitinger\n"
    		"Copyright (C) 2013 \n"
    		"\n"
    		"Usage: mrsh-v2 [-cgpfrhezys] [-t val] [-Ll LIST] [FILE/DIR/LIST]* \n"
            "OPTIONS: -c: Compares [FILE/DIR] against [FILE/DIR]. \n"
            "         -g: Generates and compares all files in [FILE/DIR]* against each other. \n"
            "         -L: Compare [LIST] against itself or [LIST] against [LIST]. \n"
    		"         -l: Compare [LIST] against [FILE/DIR]* . \n"
            "         -p: Print similarity digest as hex of all [FILE/DIR]*. \n"
            "         -f: Turns into file comparison mode which is better for getting exact similarity between files. \n"
            "         -r: Reads directories recursive. \n"
    		"         -t: All comparison yielding a score >= val are printed, i.e., 0 print all comparisons, \n"
//    		"         -i: Very small inputs cannot be matched reliably only against large files. These comparisons are ignored. \n"
            "         -h: Print this help message \n"

	    " \n\nNew functions (by Vitor Moia): \n"
	    "\n         -e: Extract features from FILE and insert into database\n\t\t Ex.: mrsh-v2 -e FILE"
	    "\n         -z: Extract features from a list of files and insert inyo database\n\t\t Ex.: mrsh-v2 -z list_of_files database_path"
            "\n         -y: Check the number of extracted features and database stored features for a list of files"
            "\n         -s: Extract features from FILE using a sliding fixed-size window and insert into database\n\t\t Ex.: mrsh-v2 -s FILE\n"
		);
}

static void initalizeDefaultModes(){
	mode = (MODES *)malloc(sizeof(MODES));
	mode->compare = false;
	mode->gen_compare = false;
	mode->compareLists = false;
	mode->file_comparison = false;
	mode->helpmessage = false;
	mode->print = false;
	mode->threshold = 1;
	mode->recursive = false;
	mode->path_list_compare = false;

	mode->extract_features = false;
	mode->extract_features_fw = false;
	mode->extract_features_list=false;
	mode->extract_features_list_check=false;
}

int main(int argc, char **argv){

	int i;
	initalizeDefaultModes();

	char *listName = NULL;

	while ((i=getopt(argc,argv,"cesyzgL:l:pfrt:h")) != -1) {
	    switch(i) {
	    	case 'c':	mode->compare = true; break;
	    	case 'g':	mode->gen_compare = true; break;
	    	case 'L':	mode->compareLists = true; listName = optarg; break;
	    	case 'l':	mode->path_list_compare = true; listName = optarg; break;
	    	case 'p':	mode->print = true; break;
	    	case 'f':	mode->file_comparison = true; break;
	    	case 'r':	mode->recursive = true; break;
	    	case 't': 	mode->threshold = atoi(optarg);  break;
	    	case 'h':	mode->helpmessage = true; break;


		case 'e':	mode->extract_features = true; break;
		case 's':	mode->extract_features_fw = true; break;
		case 'z':	mode->extract_features_list = true; break;
		case 'y':	mode->extract_features_list_check = true; break;


	    	default: 	mode->helpmessage = true;
	    				fprintf(stderr,"[*] Unknown option(s) \n");
	    				 break;
	    }
	  }


	if(mode->helpmessage) {
	    	show_help();
	    	exit(0);
	}

	//read all arguments, create the fingerprint, and print it to stdout
	if(mode->print || optind==1) {
	  FINGERPRINT_LIST *fpl = init_empty_fingerprintList();
	  for (int j = optind; j < argc; j++)
		  addPathToFingerprintList(fpl, argv[j]);
	  print_fingerprintList(fpl);
	  fingerprintList_destroy(fpl);
	  exit(1);
	}

	//set a threshold
	if(mode->threshold>100 || mode->threshold<0)
		  fatal_error("Threshold value needs to be a number between 0 and 100");

	//compare all-against-all
	if(mode->gen_compare) {
	  FINGERPRINT_LIST *fpl = init_empty_fingerprintList();
	  for (int j = optind; j < argc; j++)
		  addPathToFingerprintList(fpl, argv[j]);
	  all_against_all_comparsion(fpl);
	  fingerprintList_destroy(fpl);
	}


	  // compare one or two fingerprint lists
	  if(mode->compareLists) {
		  FINGERPRINT_LIST *fpl1 = init_fingerprintList_for_ListFile(listName);

		  //in case there is only one List
		  if((argc - optind) == 0){
			  all_against_all_comparsion(fpl1);

		  //an additional parameter means 2 lists...
		  } else if ((argc - optind) == 1){
			  FINGERPRINT_LIST *fpl2 = init_fingerprintList_for_ListFile(argv[optind]);
			  fingerprint_list_comparsion(fpl1, fpl2);
			  fingerprintList_destroy(fpl2);

		  //otherwise it is an error
		  } else
			  fatal_error("Compare lists only except two lists. Change amount of input parameters");

		  fingerprintList_destroy(fpl1);
	  }


	  //compares LIST against [FILE/DIR]*
	  if(mode->path_list_compare){
		  FINGERPRINT_LIST *fpl1 = init_fingerprintList_for_ListFile(listName);
		  FINGERPRINT_LIST *fpl2 = init_empty_fingerprintList();
		  for (int j = optind; j < argc; j++)
		  			  addPathToFingerprintList(fpl2, argv[j]);

		  fingerprint_list_comparsion(fpl1, fpl2);

		  fingerprintList_destroy(fpl1);fingerprintList_destroy(fpl2);
	  }


	  //compare two arguments which each other eg. dir/file file/dir file/file or dir/dir
	  if(mode->compare) {
		  FINGERPRINT_LIST *fpl1 = init_empty_fingerprintList();
	  	  FINGERPRINT_LIST *fpl2 = init_empty_fingerprintList();

	  	  addPathToFingerprintList(fpl1, argv[optind]);
	  	  addPathToFingerprintList(fpl2, argv[optind+1]);

	  	  fingerprint_list_comparsion(fpl1, fpl2);

	  	  fingerprintList_destroy(fpl1);
	  	  fingerprintList_destroy(fpl2);
	    }


		


		//extracting features from the object
	   if(mode->extract_features) {

		printf("FEATURE EXTRACTION OF: \n\t%s\n", argv[optind]);

		featureExtractionMRSH(argv[optind]);

	   }

	
	   //extracting features from the object using a fixed-size sliding window
	   if(mode->extract_features_fw) {

		printf("FEATURE EXTRACTION OF: \n\t%s\n", argv[optind]);

		featureExtraction(argv[optind]);

	   }

	
	   if(mode->extract_features_list){
		printf("FEATURE EXTRACTION OF: %s - DATABASE: %s\n", argv[optind], argv[optind+1]);
		featureExtractionMRSH_list(argv[optind], argv[optind+1]);

	   }

	   if(mode->extract_features_list_check){
		printf("FEATURE EXTRACTION CHECKING OF: %s - DATABASE: %s\n", argv[optind], argv[optind+1]);
		featureExtractionMRSH_list_checking(argv[optind], argv[optind+1]);
	   }

		free(mode);

		exit(0);
}




/*
 * adds a path to a fingerprints list. may be recursive depending on the parameters
 */
void addPathToFingerprintList(FINGERPRINT_LIST *fpl, char *filename){
	DIR *dir;
	struct dirent *ent;
	const int max_path_length = 1024;

	char *cur_dir = (char *)malloc(max_path_length);
	getcwd(cur_dir, max_path_length);

	//in case of a dir
	if (is_dir(filename)) {
			dir = opendir (filename);
			chdir(filename);

			//run through all files of the dir
		  	while ((ent = readdir (dir)) != NULL) {

		  		//if we found a file, generate hash value and add it
		  		if(is_file(ent->d_name)) {
		  			FILE *file = getFileHandle(ent->d_name);
		  			FINGERPRINT *fp = init_fingerprint_for_file(file, ent->d_name);
		  			add_new_fingerprint(fpl, fp);
		  		}

		  		//when we found a dir and recursive mode is on, go deeper
		  		else if(is_dir(ent->d_name) && mode->recursive) {
		  			if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
		  			    continue;
		  			addPathToFingerprintList(fpl, ent->d_name);
		  		}
		  	}
		  	chdir(cur_dir);
		  	closedir (dir);
	}

	//in case we we have only a file
	else if(is_file(filename)) {
		FILE *file = getFileHandle(filename);
			FINGERPRINT *fp = init_fingerprint_for_file(file, filename);
			add_new_fingerprint(fpl, fp);
	}

	return;
}



/*
 * EXTRACT features from a file - MRSH-v2
 */
void featureExtractionMRSH(char *filename){
	DIR *dir;
	struct dirent *ent;
	const int max_path_length = 1024;

	char *cur_dir = (char *)malloc(max_path_length);
	getcwd(cur_dir, max_path_length);

	//in case of a dir
	if (is_dir(filename)) {
			dir = opendir (filename);
			chdir(filename);

			//run through all files of the dir
		  	while ((ent = readdir (dir)) != NULL) {

		  		//if we found a file, generate hash value and add it
		  		if(is_file(ent->d_name)) {
		  			FILE *file = getFileHandle(ent->d_name);
		  			init_fingerprint_for_file_NO_BF(file, ent->d_name, NULL);
		  		}

		  		//when we found a dir and recursive mode is on, go deeper
		  		else if(is_dir(ent->d_name) && mode->recursive) {
		  			if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
		  			    continue;
		  			featureExtractionMRSH(ent->d_name);
		  		}
		  	}
		  	chdir(cur_dir);
		  	closedir (dir);
	}

	//in case we we have only a file
	else if(is_file(filename)) {
		FILE *file = getFileHandle(filename);
		init_fingerprint_for_file_NO_BF(file, filename, NULL);
	}

	free(cur_dir);

	return;
}


/*
 * EXTRACT features from a list of files - MRSH-v2
 */
void featureExtractionMRSH_list(char *list, char *database){

	FILE *arq;
	int num_files=0;
	int num_obj_features=0;
	char linha[SIZE_LINE];
	sqlite3 *db;

	printf("\n**************** MRSH-v2 FEATURE EXTRACTION ****************\n\n");

	arq = fopen(list, "r");

	if(!arq){
		fprintf(stderr, "Couldn't open %s\n", list);
		exit(1);
	}

	printf("Openning database: ");

	/* Open database */
	db = open_connection(database);

	printf("[OK]\nStarting process:");

	while (!feof(arq))
	{
		if( fgets (linha, SIZE_LINE, arq)!=NULL )
		{
			char *pos;
			if ((pos=strchr(linha, '\n')) != NULL)
				*pos = '\0';

			printf("\n\tProcessing file: %s\n", linha);
			FILE *file = getFileHandle(linha);
			num_obj_features = init_fingerprint_for_file_NO_BF(file, linha, db);
			printf("\t\tNum. features: %d", num_obj_features);
			num_global_features+= num_obj_features;
			num_obj_features=0;
			num_files++;
		}
	}

	fclose(arq);

	printf("\nProcess complete.\nClosing database: ");

	/* Close database */
    	close_connection(db);
	
	printf("[OK]\n\nStatistics: \n\tNumber of files processed: %d\n\tNumber of features extracted: %d\n\n", num_files, num_global_features);

	printf("Exiting program...\n");

	return 0;
}


/*
 * EXTRACT features from a list of files - MRSH-v2
 */
void featureExtractionMRSH_list_checking(char *list, char *database){

	FILE *arq;
	int num_files=0;
	int num_obj_features=0;
	char linha[SIZE_LINE];
	sqlite3 *db;

	printf("\n**************** MRSH-v2 FEATURE EXTRACTION CHECKING ****************\n\n");

	arq = fopen(list, "r");

	if(!arq){
		fprintf(stderr, "Couldn't open %s\n", list);
		exit(1);
	}

	printf("Openning database: ");

	/* Open database */
	db = open_connection(database);

	printf("[OK]\nStarting process:");

	while (!feof(arq))
	{
		if( fgets (linha, SIZE_LINE, arq)!=NULL )
		{
			char *pos;
			if ((pos=strchr(linha, '\n')) != NULL)
				*pos = '\0';

			printf("\n\tProcessing file: %s\n", linha);
			FILE *file = getFileHandle(linha);
			num_obj_features = init_fingerprint_for_file_checking(file, linha, db);
			//printf("\t\tNum. features: %d", num_obj_features);
			num_global_features+= num_obj_features;
			num_obj_features=0;
			num_files++;
		}
	}

	fclose(arq);

	printf("\nProcess complete.\nClosing database: ");

	/* Close database */
    	close_connection(db);
	
	printf("[OK]\n\nStatistics: \n\tNumber of files processed: %d\n\tNumber of features extracted: %d\n\n", num_files, num_global_features);

	printf("Exiting program...\n");

	return 0;
}


/*
 * EXTRACT features from a file - sliding window
 */
void featureExtraction(char *filename){
	DIR *dir;
	struct dirent *ent;
	const int max_path_length = 1024;

	char *cur_dir = (char *)malloc(max_path_length);
	getcwd(cur_dir, max_path_length);

	//in case of a dir
	if (is_dir(filename)) {
			dir = opendir (filename);
			chdir(filename);

			//run through all files of the dir
		  	while ((ent = readdir (dir)) != NULL) {

		  		//if we found a file, generate hash value and add it
		  		if(is_file(ent->d_name)) {
		  			FILE *file = getFileHandle(ent->d_name);
		  			init_fingerprint_for_file_NO_BF_NO_CONTEXT(file, ent->d_name);
		  		}

		  		//when we found a dir and recursive mode is on, go deeper
		  		else if(is_dir(ent->d_name) && mode->recursive) {
		  			if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
		  			    continue;
		  			featureExtraction(ent->d_name);
		  		}
		  	}
		  	chdir(cur_dir);
		  	closedir (dir);
	}

	//in case we we have only a file
	else if(is_file(filename)) {
		FILE *file = getFileHandle(filename);
		init_fingerprint_for_file_NO_BF_NO_CONTEXT(file, filename);
	}

	return;
}


