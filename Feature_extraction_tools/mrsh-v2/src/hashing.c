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

    File: hashing.c
*/

#include "../header/hashing.h"
#include "../header/util_sql.h"
#include "../header/config.h"
#include "../header/util.h"
#include <stdio.h>
#include <openssl/md5.h>
#include <string.h>
#include <sqlite3.h> 
#include <libgen.h> // fix segmentation fault caused by basename()

struct features_obj{
	char hash[40];
	char offset[20];
	unsigned int size;
	struct features_obj *next;
};

struct feature_set{
	int id_obj; 
	struct features_obj *feature_set; 
	sqlite3 *db;
};

uint32 roll_hashx(unsigned char c, uchar window[], uint32 rhData[])
{
    rhData[2] -= rhData[1];
    rhData[2] += (ROLLING_WINDOW * c);

    rhData[1] += c;
    rhData[1] -= window[rhData[0] % ROLLING_WINDOW];

    window[rhData[0] % ROLLING_WINDOW] = c;
    rhData[0]++;

    /* The original spamsum AND'ed this value with 0xFFFFFFFF which
       in theory should have no effect. This AND has been removed 
       for performance (jk) */
    rhData[3] = (rhData[3] << 5); //& 0xFFFFFFFF;
    rhData[3] ^= c;

    return rhData[1] + rhData[2] + rhData[3];
}

uint32 djb2x(unsigned char c, uchar window[], unsigned int n)
{
    unsigned long hash = 5381;
    int i;
    unsigned char tmp;
    window[n % ROLLING_WINDOW] = c;

    for(i=0;i<7;i++)
    {
        tmp = window[(n+i) % ROLLING_WINDOW],
            hash = ((hash << 5) + hash) + tmp;
    }

    return hash;
}


int hashFileToFingerprint(FINGERPRINT *fingerprint, FILE *handle)
{
    unsigned long  bytes_read;   //stores the number of characters read from input file
    unsigned int   i;
    unsigned char  *byte_buffer     = NULL;
    unsigned int last_block_index = 0;
    uint64 rValue, hashvalue=0;

    /*we need this arrays for our extended rollhash function*/
    uchar window[ROLLING_WINDOW] = {0};
    uint32 rhData[4]             = {0};

    if((byte_buffer = (unsigned char*)malloc(sizeof(unsigned char)*fingerprint->filesize))==NULL)
        return -1;

    // bytes_read stores the number of characters we read from our file
    fseek(handle,0L, SEEK_SET);	
    if((bytes_read = fread(byte_buffer,sizeof(unsigned char),fingerprint->filesize,handle))==0)
        return -1;

    short first = 1;

    for(i=0; i<bytes_read; i++)
    {
        /*  
         * rValue = djb2x(byte_buffer[i],window,i);  
         */ 
        rValue  = roll_hashx(byte_buffer[i], window, rhData);  

        if (rValue % BLOCK_SIZE == BLOCK_SIZE-1) // || chunk_index >= BLOCK_SIZE_MAX)
        {

        	#ifdef network
        	if (first == 1){
        		first=0;
        		last_block_index = i+1;
        		if(i+SKIPPED_BYTES < bytes_read)
        		       i += SKIPPED_BYTES;
        		continue;
        	}
			#endif

        	hashvalue = fnv64Bit(byte_buffer, last_block_index, i); //,current_index, FNV1_64_INIT);
	       	add_hash_to_fingerprint(fingerprint, hashvalue); //printf("%i %llu \n", i, hashvalue);

            last_block_index = i+1;

            if(i+SKIPPED_BYTES < bytes_read)
            	i += SKIPPED_BYTES;
        }
    }

    #ifndef network
    	hashvalue = fnv64Bit(byte_buffer, last_block_index, bytes_read-1);
    	add_hash_to_fingerprint(fingerprint, hashvalue);
	#endif

    free(byte_buffer);
    return 1;	
}

int hashPacketBuffer(FINGERPRINT *fingerprint, const unsigned char *packet, const size_t length)
{
    unsigned int i;
    unsigned int last_block_index = 0;
    uint64 rValue, hashvalue=0;
    bool first = 1;

    uchar window[ROLLING_WINDOW] = {0};
    uint32 rhData[4]             = {0};


    for(i=0; i<length;i++)
    {
        rValue  = roll_hashx(packet[i], window, rhData);  

        if (rValue % BLOCK_SIZE == BLOCK_SIZE-1) 
        {

			#ifdef network
        	if (first == 1){
        		first=0;
        		last_block_index = i+1;
        		if(i+SKIPPED_BYTES < length)
        		       i += SKIPPED_BYTES;
        		continue;
        	}
			#endif
        		hashvalue = fnv64Bit(packet, last_block_index, i); //,current_index, FNV1_64_INIT);
        		add_hash_to_fingerprint(fingerprint, hashvalue);

        		last_block_index = i+1;

            if(i+SKIPPED_BYTES < length)
            	i += SKIPPED_BYTES;
        }
    }

#ifndef network
    	hashvalue = fnv64Bit(packet, last_block_index, length-1);
    	add_hash_to_fingerprint(fingerprint, hashvalue);
#endif

    return 1;
}


void print_md5value(unsigned char *md5_value)
{
    int i;
    for(i=0;i<MD5_DIGEST_LENGTH;i++)
        printf("%02x",md5_value[i]);
    puts("");
}


unsigned char* retornar_string(unsigned char pBuffer[], int start, int end) {

    unsigned char* string_saida = (unsigned char*)malloc(sizeof(unsigned char)*(end - start));

   int u = end-start;
   printf("SIZE: %d - last position: %x\n", u, pBuffer[end]);

   int j = 0;
   int i = start;
   while( i <= end ) {
	string_saida[j] = pBuffer[i];
	printf("%x", string_saida[j]);
	j++;
	printf(" (%x) ", pBuffer[i++]);
	  //string[j] = pBuffer[i++];
	
   }

	printf("\n");

   return string_saida;
 }

int hashFile_features_extraction(unsigned int filesize, char *filename, FILE *handle, sqlite3 *db)
{
    unsigned long  bytes_read;   //stores the number of characters read from input file
    unsigned int   i;
    unsigned char  *byte_buffer     = NULL;
    unsigned int last_block_index = 0;
    uint64 rValue, hashvalue=0;
    unsigned char *results = NULL;
    int num_features = 0;	// counting the number of features


	int close_db=0;

	if(db == NULL){

    		/* Open database */
    		db = open_connection(DATA_BASE);
		close_db=1;
	}

    char* name = basename(filename);

    /* Verifying if the registry already exists and if does getting ID */
    int id_obj = getting_id_from_objects_tb(name, db);

    //printf("\nID: %d\n", id_obj);

    if (id_obj < 0) {

       	/* CREATING A NEW REGISTRY */
	id_obj = inserting_new_obj_into_objects_tb(name, get_filename_ext(filename), get_file_size(filename), db);

	if(id_obj < 0){
		printf("\nError! Object could not be inserted into database!\n");
		return -1;
	}
	
    }
    else{
	/* Removing existing features before inserting new ones
		(avoid duplicate entries)
	 */
	remove_existing_features(db, id_obj);
    }



    struct features_obj *features = NULL;	
    struct features_obj *last;// = (struct features_obj*) malloc(sizeof(features_obj));

    /*we need this arrays for our extended rollhash function*/
    uchar window[ROLLING_WINDOW] = {0};
    uint32 rhData[4]             = {0};

    if((byte_buffer = (unsigned char*)malloc(sizeof(unsigned char)*filesize))==NULL)
        return -1;

    // bytes_read stores the number of characters we read from our file
    fseek(handle,0L, SEEK_SET);	
    if((bytes_read = fread(byte_buffer,sizeof(unsigned char),filesize,handle))==0)
        return -1;

    short first = 1;

    for(i=0; i<bytes_read; i++)
    {
        /*  
         * rValue = djb2x(byte_buffer[i],window,i);  
         */ 
        rValue  = roll_hashx(byte_buffer[i], window, rhData);  

        if (rValue % BLOCK_SIZE == BLOCK_SIZE-1) // || chunk_index >= BLOCK_SIZE_MAX)
        {

        	#ifdef network
        	if (first == 1){
        		first=0;
        		last_block_index = i+1;
        		if(i+SKIPPED_BYTES < bytes_read)
        		       i += SKIPPED_BYTES;
        		continue;
        	}
			#endif

        	hashvalue = fnv64Bit(byte_buffer, last_block_index, i); //,current_index, FNV1_64_INIT);

		int size_fet = (i-last_block_index)+1;

		//unsigned char *results = malloc(size*sizeof(unsigned char*));
		results = malloc(sizeof(unsigned char*) * size_fet);
		unsigned char c[3];
		results[0]='\0';	// ensures the memory is an empty string

		int z = last_block_index;
		while( z <= i) {
			sprintf(c, "%.2x ", (unsigned)byte_buffer[z++]);
			//printf("%s", c);
			strcat(results, c);
			//printf("%x-", (unsigned)byte_buffer[z++]);
		}

		/*
		// adding feature to database
		
		int id_fet = getting_feature_id((char*) hashvalue, db);

	        printf("ID_FET: %d\n", id_fet);

	        if (id_fet < 0) {
	       		// CREATING A NEW FEATURE
			printf("\n\nINSERTING\n\n");
			id_fet = inserting_new_feature(id_obj, (char*) hashvalue, size_fet, (char*) last_block_index, db);
			printf("\n\nID FEATURE CRIADA: %d\n\n", id_fet);
	        }
		else {
			printf("\n\nUPDATING\n\n");
			updating_feature(id_obj, id_fet, (char*) last_block_index, db);
		}

	       	//add_hash_to_fingerprint(fingerprint, hashvalue); //printf("%i %llu \n", i, hashvalue);

		*/

		if(features != NULL && features->size > 0){
	
			struct features_obj *temp;
			temp = (struct features_obj*) malloc(sizeof(struct features_obj));
			sprintf(temp->hash, "%X", (char*)hashvalue);
			sprintf(temp->offset, "%X", (char*)last_block_index);
			temp->size = size_fet;
			temp->next = NULL;
			last->next = temp;
			last = temp;
		}
		else {

			features = (struct features_obj*) malloc(sizeof(struct features_obj));

			sprintf(features->hash, "%X", (char*)hashvalue);
			sprintf(features->offset, "%X", (char*)last_block_index);
			features->size = size_fet;
			features->next = NULL;
			last = features;
		}

            	last_block_index = i+1;

		num_features++;

            	if(i+SKIPPED_BYTES < bytes_read)
            		i += SKIPPED_BYTES;

	   	free(results);
        }
    }

    #ifndef network
    	hashvalue = fnv64Bit(byte_buffer, last_block_index, bytes_read-1);
	//printf("FEATURE ==> %X\t - OFFSET: %d\t - SIZE: %d\n", hashvalue, last_block_index, (bytes_read-1 - last_block_index));
	#endif

    sqlite3_stmt* stmt = prepared_insert_feature_statement(db);

    struct features_obj *temp = features;
    
    while(features != NULL){
	temp = features;
	/* adding feature to database */
	inserting_new_feature_prepared_stmt(id_obj, temp->hash, temp->size, temp->offset, db, stmt);
	features = temp->next;
	free(temp); 
    }

    free(features);
    finalize_prepared_stmt(stmt);
    free(byte_buffer);

	if(close_db > 0)
    		/* Close database */
    		close_connection(db);

    return num_features;	
}




int hashFile_features_extraction_checking(unsigned int filesize, char *filename, FILE *handle, sqlite3 *db)
{
    unsigned long  bytes_read;   //stores the number of characters read from input file
    unsigned int   i;
    unsigned char  *byte_buffer     = NULL;
    unsigned int last_block_index = 0;
    uint64 rValue, hashvalue=0;
    unsigned char *results = NULL;
    int num_features = 0;	// counting the number of features


	int close_db=0;

	if(db == NULL){

    		/* Open database */
    		db = open_connection(DATA_BASE);
		close_db=1;
	}

    sqlite3_stmt* smtp = prepared_statement_select_num_features(db);
    int num_features_db = return_num_features_object_by_name(basename(filename), smtp);

    /*we need this arrays for our extended rollhash function*/
    uchar window[ROLLING_WINDOW] = {0};
    uint32 rhData[4]             = {0};

    if((byte_buffer = (unsigned char*)malloc(sizeof(unsigned char)*filesize))==NULL)
        return -1;

    // bytes_read stores the number of characters we read from our file
    fseek(handle,0L, SEEK_SET);	
    if((bytes_read = fread(byte_buffer,sizeof(unsigned char),filesize,handle))==0)
        return -1;

    short first = 1;

    for(i=0; i<bytes_read; i++)
    {
        /*  
         * rValue = djb2x(byte_buffer[i],window,i);  
         */ 
        rValue  = roll_hashx(byte_buffer[i], window, rhData);  

        if (rValue % BLOCK_SIZE == BLOCK_SIZE-1) // || chunk_index >= BLOCK_SIZE_MAX)
        {

        	#ifdef network
        	if (first == 1){
        		first=0;
        		last_block_index = i+1;
        		if(i+SKIPPED_BYTES < bytes_read)
        		       i += SKIPPED_BYTES;
        		continue;
        	}
			#endif

        	hashvalue = fnv64Bit(byte_buffer, last_block_index, i); //,current_index, FNV1_64_INIT);

		int size_fet = (i-last_block_index)+1;

		//unsigned char *results = malloc(size*sizeof(unsigned char*));
		results = malloc(sizeof(unsigned char*) * size_fet);
		unsigned char c[3];
		results[0]='\0';	// ensures the memory is an empty string

		int z = last_block_index;
		while( z <= i) {
			sprintf(c, "%.2x ", (unsigned)byte_buffer[z++]);
			//printf("%s", c);
			strcat(results, c);
			//printf("%x-", (unsigned)byte_buffer[z++]);
		}

            	last_block_index = i+1;

		num_features++;

            	if(i+SKIPPED_BYTES < bytes_read)
            		i += SKIPPED_BYTES;

	   	free(results);
        }
    }

    #ifndef network
    	hashvalue = fnv64Bit(byte_buffer, last_block_index, bytes_read-1);
	//printf("FEATURE ==> %X\t - OFFSET: %d\t - SIZE: %d\n", hashvalue, last_block_index, (bytes_read-1 - last_block_index));
	#endif

    finalize_prepared_stmt(smtp);
    free(byte_buffer);

    if( num_features_db != num_features)
        printf("\t\tDIFFERENCE!\n\t\t\tNUM FEATURES DB: %d / NUM EXTRACTED FEATURES: %d\n", num_features_db, num_features);

	if(close_db > 0)
    		/* Close database */
    		close_connection(db);

    return num_features;	
}


int hashFile_features_extraction_no_context(unsigned int filesize, char *filename, FILE *handle)
{
    unsigned long  bytes_read;   //stores the number of characters read from input file
    unsigned int   i;
    unsigned char  *byte_buffer     = NULL;
    unsigned int last_block_index = 0;
    uint64 hashvalue=0;

    if((byte_buffer = (unsigned char*)malloc(sizeof(unsigned char)*filesize))==NULL)
        return -1;

    // bytes_read stores the number of characters we read from our file
    fseek(handle,0L, SEEK_SET);

    if((bytes_read = fread(byte_buffer,sizeof(unsigned char),filesize,handle))==0)
        return -1;

    //printf("\nFile name: %s\n", filename);

    for(i=(SLIDING_WINDOW-1); i<bytes_read; i++)
    {
	hashvalue = fnv64Bit(byte_buffer, last_block_index, i);
	//printf("FEATURE ==> %16X\t - OFFSET: %d\t - SIZE: %d\n", hashvalue, last_block_index, (i - last_block_index)+1);

	last_block_index = last_block_index+1;
    }

    free(byte_buffer);
    return 1;	
}


