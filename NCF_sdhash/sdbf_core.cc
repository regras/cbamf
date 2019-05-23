/**
 * sdbf_core.c: Similarity digest calculation functions.
 * authors Vassil Roussev, Candice Quates
 *
 * Code adapted from sdhash (https://github.com/sdhash/sdhash)
 *
 * Complemented by:
 *	Vitor Hugo Galhardo Moia, Ph.D student
 * 	Department of Computer Engineering and Industrial Automation (DCA)
 *	School of Electrical and Computer Engineering (FEEC)
 *	University of Campinas (UNICAMP)
 *	Campinas, SP, Brazil 13083-852
 *	Email: vhgmoia@dca.fee.unicamp.br / vitormoia@gmail.com
 *	page: http://www.dca.fee.unicamp.br/~vhgmoia/
 *
 * March, 28th 2019	
 *
 * Changes:
 * 	- Use of FNV hash function instead of SHA-1
 * 	- Removing common features based on a database of previous extracted features
 */

#include <boost/thread/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include <vector>
#include "sdbf_class.h"
#include "sdbf_defines.h"

#include <sqlite3.h>

#include <boost/filesystem.hpp>
namespace fs=boost::filesystem;

/* Database path */
const char* DATA_BASE = "database_common_features.db";

#define MAXIMUM_NUM_COMMON_FEAT 3 // only features with 1 or 2 occurencies are accepted
//#define MAXIMUM_NUM_COMMON_FEAT 5
//#define MAXIMUM_NUM_COMMON_FEAT 10
//#define MAXIMUM_NUM_COMMON_FEAT 20
//#define MAXIMUM_NUM_COMMON_FEAT 50
//#define MAXIMUM_NUM_COMMON_FEAT 100



/* STARTING DB FUNCTIONS */

sqlite3* open_connection(const char *db_name){
    sqlite3 *db;

    int rc = sqlite3_open(db_name, &db);

    if(rc)
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));

    return db;
}

void close_connection(sqlite3 *db){

    int rc = sqlite3_close(db);
    char *zErrMsg = 0;

    if( rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error (close connection): %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return;
}

int checking_for_feature_on_db(char* hash_feature, sqlite3 *db, sqlite3_stmt *stmt) {

    //Binding parameters...

    if (sqlite3_bind_text(stmt, 1, hash_feature, strlen(hash_feature), SQLITE_TRANSIENT) != SQLITE_OK) {
        printf("\nCould not bind text.\n");
        return 0;
    }

    //Executing sql statement...
    int count=0;
	
    if ( sqlite3_step(stmt) == SQLITE_ROW ){
        do
        {
            for(int i=0; i < sqlite3_column_count(stmt); i++)
                count  = sqlite3_column_int(stmt, i);
        }while(sqlite3_step(stmt) == SQLITE_ROW);
    }

    //Cleaning parameters values...
    sqlite3_reset(stmt);

    return count;
}

sqlite3_stmt* prepare_select_counter_common_features_stmt(sqlite3 *db){

    char sql[200];
    sql[0]='\0';

    sqlite3_stmt *stmt;	
    strcpy(sql, "SELECT CONT_DIFF FROM common_features_sdhash where HASH=?");
	
    if ( sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("\nCould not prepare statement.");
    	return NULL;
    }

    return stmt;
}

void finalize_prepared_stmt(sqlite3_stmt *stmt){
	sqlite3_finalize(stmt);
}


/* ENDING DB FUNCTIONS */



/* STARTING FNV HASH FUNCTION */

/// hash FNV a block of memory
 void fnv1a(const void* data, size_t numBytes, uint32_t *fnv_hash) {

    uint64_t extract0;
    uint64_t extract1;
    uint64_t extract2;
    uint64_t extract3;
    uint64_t extract4;

    uint64_t mask0 = 0xFFF0000000000000;	//Take 12 bits of 64 hash
    uint64_t mask1 = 0x000FFF0000000000;
    uint64_t mask2 = 0x000000FFF0000000;
    uint64_t mask3 = 0x000000000FFF0000;
    //uint64_t mask4 = 0x000000000000FFF0;
    uint64_t mask4 = 0x000000000000FFFF;

    fnv_hash[0] = 0x00000000;
    fnv_hash[1] = 0x00000000;
    fnv_hash[2] = 0x00000000;
    fnv_hash[3] = 0x00000000;
    fnv_hash[4] = 0x00000000;


    const uint64_t Prime = 1099511628211; //   16777619
    const uint64_t Seed  = 0x811C9DC5A9B3C6D8;

    uint64_t hash = Seed;

    assert(data);
    const unsigned char* ptr = (const unsigned char*)data;
    while (numBytes--)
        hash = (*ptr++ ^ hash) * Prime;
  	
    //printf("HASH DE 64: 0x%" PRIx64 "\n", hash);

    extract0 = (mask0 & hash) ;
    extract1 = (mask1 & hash) ;
    extract2 = (mask2 & hash) ;
    extract3 = (mask3 & hash) ;
    extract4 = (mask4 & hash) ;

    extract0 = extract0 >>= 8;
    extract0 = extract0 >>= 8;
    extract0 = extract0 >>= 8;
    extract0 = extract0 >>= 8;
    extract0 = extract0 >>= 8;
    extract0 = extract0 >>= 8;
    extract0 = extract0 >>= 4;

    extract1 = extract1 >>= 8;
    extract1 = extract1 >>= 8;
    extract1 = extract1 >>= 8;
    extract1 = extract1 >>= 8;
    extract1 = extract1 >>= 8;

    extract2 = extract2 >>= 8;
    extract2 = extract2 >>= 8;
    extract2 = extract2 >>= 8;
    extract2 = extract2 >>= 4;

    extract3 = extract3 >>= 8;
    extract3 = extract3 >>= 8;

    //extract4 = extract4 >>= 4;
    extract4 = extract4;

    fnv_hash[0] = fnv_hash[0]|extract0;
    fnv_hash[1] = fnv_hash[1]|extract1;
    fnv_hash[2] = fnv_hash[2]|extract2;
    fnv_hash[3] = fnv_hash[3]|extract3;
    fnv_hash[4] = fnv_hash[4]|extract4;

    /*
    printf("FNV_HASH 0: 0x%" PRIx32 "\n", fnv_hash[0]);
    printf("FNV_HASH 1: 0x%" PRIx32 "\n", fnv_hash[1]);
    printf("FNV_HASH 2: 0x%" PRIx32 "\n", fnv_hash[2]);
    printf("FNV_HASH 3: 0x%" PRIx32 "\n", fnv_hash[3]);
    printf("FNV_HASH 4: 0x%" PRIx32 "\n\n", fnv_hash[4]);
    
    printf("HASH: %X", fnv_hash[4]);
    */
}

/* ENDING FNV HASH FUNCTION */



boost::mutex index_output;
/**
 * Generate ranks for a file chunk.
 */
void 
sdbf::gen_chunk_ranks( uint8_t *file_buffer, const uint64_t chunk_size, uint16_t *chunk_ranks, uint16_t carryover) {
    int64_t offset, entropy=0;
    uint8_t *ascii = (uint8_t *)alloc_check( ALLOC_ZERO, 256, "gen_chunk_ranks", "ascii", ERROR_EXIT);;

    if( carryover > 0) {
        memcpy( chunk_ranks, chunk_ranks+chunk_size-carryover, carryover*sizeof(uint16_t));
    }
    memset( chunk_ranks+carryover,0, (chunk_size-carryover)*sizeof( uint16_t));
    //for( offset=0; offset<chunk_size-config->entr_win_size; offset++) { 
    int64_t limit = int64_t(chunk_size)-int64_t(config->entr_win_size);
    if(limit >0) {
        for (offset=0; offset<limit; offset++) {
            // Initial/sync entropy calculation
            if ( offset % config->block_size == 0) {
                entropy = config->entr64_init_int( file_buffer+offset, ascii);
            // Incremental entropy update (much faster)
            } else {
                entropy = config->entr64_inc_int( entropy, file_buffer+offset-1, ascii);
            }
            chunk_ranks[offset] = config->ENTR64_RANKS[entropy >> ENTR_POWER];
        }
    }
    free( ascii);
}

/**
 * Generate scores for a ranks chunk.
 */
void 
sdbf::gen_chunk_scores( const uint16_t *chunk_ranks, const uint64_t chunk_size, uint16_t *chunk_scores, int32_t *score_histo) { 
    uint64_t i, j;
    uint32_t pop_win = config->pop_win_size;
    uint64_t min_pos = 0;
    uint16_t min_rank = chunk_ranks[min_pos]; 

    memset( chunk_scores, 0, chunk_size*sizeof( uint16_t));
    if (chunk_size > pop_win) {
        for( i=0; i<chunk_size-pop_win; i++) {
            // try sliding on the cheap    
            if( i>0 && min_rank>0) {
                while( chunk_ranks[i+pop_win] >= min_rank && i<min_pos && i<chunk_size-pop_win+1) {
                    if( chunk_ranks[i+pop_win] == min_rank)
                        min_pos = i+pop_win;
                    chunk_scores[min_pos]++;
                    i++;
                }
            }      
            min_pos = i;
            min_rank = chunk_ranks[min_pos];
            for( j=i+1; j<i+pop_win; j++) {
                if( chunk_ranks[j] < min_rank && chunk_ranks[j]) {
                    min_rank = chunk_ranks[j];
                    min_pos = j;
                } else if( min_pos == j-1 && chunk_ranks[j] == min_rank) {
                    min_pos = j;
                }
            }
            if( chunk_ranks[min_pos] > 0) {
                chunk_scores[min_pos]++;
            }
        }
        // Generate score histogram (for b-sdbf signatures)
        if( score_histo) {
            for( i=0; i<chunk_size-pop_win; i++)
                score_histo[chunk_scores[i]]++;
        }
    }
}



/**
 * Generate FNV hashes and add them to the SDBF--original stream version.
 */
void 
sdbf::gen_chunk_hash( uint8_t *file_buffer, const uint64_t chunk_pos, const uint16_t *chunk_scores, const uint64_t chunk_size) {
    uint64_t i;
    uint32_t sha1_hash[5];
    uint32_t bf_count = this->bf_count;
    uint32_t last_count = this->last_count;
    uint8_t *curr_bf = this->buffer + (bf_count-1)*(this->bf_size);

    //int cont_discarded=0;
    //int num_features=0;
    sqlite3 *db;

    /* Open database */
    db = open_connection(DATA_BASE);
    sqlite3_stmt* stmt = prepare_select_counter_common_features_stmt(db);

    if (chunk_size > config->pop_win_size) {
        for( i=0; i<chunk_size-config->pop_win_size; i++) {
            if( chunk_scores[i] > config->threshold) {

		//Hashing feature
                //SHA1( file_buffer+chunk_pos+i, config->pop_win_size, (uint8_t *)sha1_hash);
		fnv1a( file_buffer+chunk_pos+i, config->pop_win_size, (uint32_t *)sha1_hash);

		char buf[16];
		int size=0;
		//num_features++;

		//Preparing hash for query db
		for(int n=0; n < 5; n++){
                    if(n == 0){
		        if(sha1_hash[n] == 0)
		            sprintf(buf + size, "   ");
			else
			    sprintf(buf + size, "%3x", sha1_hash[n]);
			//printf("%04x---", sha1_hash[n]);
			size=size+strlen(buf);
		    }
		    else if(n == 4){
		        sprintf(buf + size, "%04x", sha1_hash[n]);
			//printf("%04x", sha1_hash[n]);
		    }
		    else{
		        sprintf(buf + size, "%03x", sha1_hash[n]);
			//printf("%03x", sha1_hash[n]);
			size=size+3;
		    }
		}

		//printf("\n\tHASH: %s \n", buf);

		//Checking if it is a common feature
		int common=0;
		common = checking_for_feature_on_db(buf, db, stmt);

		if(common < MAXIMUM_NUM_COMMON_FEAT){
		//if(common <= MAXIMUM_NUM_COMMON_FEAT){
                    uint32_t bits_set = bf_sha1_insert( curr_bf, 0, (uint32_t *)sha1_hash);
                    //printf("\nHASH: %s (%d)", buf, common);
                    
		    // Avoid potentially repetitive features
		    if( !bits_set)
		        continue;
		    if (this->info!=NULL)
		        if (this->info->index) {
		            bool ins=this->info->index->insert_sha1((uint32_t*)sha1_hash);
                            if (ins==false)
                                continue;
		        }
		        last_count++;
		        if( last_count == this->max_elem) {
		            curr_bf += this->bf_size;
		            bf_count++;
		            last_count = 0;
		        }
		}
		/*else{
			//cont_discarded++;
			printf("\tFeature %s discarded (%d)!\n", buf, common);
		}
		*/
            }
        }
    }
    this->bf_count = bf_count;
    this->last_count = last_count;//

    //printf("\nTotal number of features: %d\n", num_features);
    //printf("\nFeatures discarded: %d\n", cont_discarded);
    finalize_prepared_stmt(stmt);

    /* Close database */
    close_connection(db);
}

/**
 * Generate SHA1 hashes and add them to the SDBF--block-aligned version.
 */
void 
sdbf::gen_block_hash( uint8_t *file_buffer, uint64_t file_size, const uint64_t block_num, const uint16_t *chunk_scores, \
             const uint64_t block_size, class sdbf *hashto, uint32_t rem, uint32_t threshold, int32_t allowed) {

    uint8_t  *bf = hashto->buffer + block_num*(hashto->bf_size);  // BF to be filled
    uint8_t  *data = file_buffer + block_num*block_size;  // Start of data
    uint32_t  i, hash_cnt=0, sha1_hash[5];
    uint32_t  max_offset = (rem > 0) ? rem : block_size;
    uint32_t hashes[193][5];
    uint32_t num_indexes= 0;
    if (hashto->info != NULL)
        if (hashto->info->setlist != NULL) 
            num_indexes=hashto->info->setlist->size();
    vector<uint32_t> match (num_indexes);
    hashto->reset_indexes(&match);
    int hashindex=0;

    //int cont_discarded;
    sqlite3 *db;

    /* Open database */
    db = open_connection(DATA_BASE);

    sqlite3_stmt* stmt = prepare_select_counter_common_features_stmt(db);

    for( i=0; i<max_offset-config->pop_win_size && hash_cnt< config->max_elem_dd; i++) {
        if(  chunk_scores[i] > threshold || 
            (chunk_scores[i] == threshold && allowed > 0)) {
                
		//Hashing feature
		//SHA1( data+i, config->pop_win_size, (uint8_t *)sha1_hash);
		fnv1a(data+i, config->pop_win_size, (uint32_t *)sha1_hash);

		char buf[16];
		int size=0;
		//printf("\n\tHASH: ");

		//Preparing hash for query db
		for(int n=0; n < 5; n++){

			if(n == 0){
				sprintf(buf + size, "%01x", sha1_hash[n]);
				//printf("%04x", sha1_hash[n]);
				size=size+strlen(buf);
			}
			else if(n == 4){
				sprintf(buf + size, "%04x", sha1_hash[n]);
				//printf("%04x", sha1_hash[n]);
			}
			else{
				sprintf(buf + size, "%03x", sha1_hash[n]);
				//printf("%03x", sha1_hash[n]);
				size=size+3;
			}
		}
		

		//printf("\n\tHASH: %s \n", buf);
		//printf("%s\n", buf);

		//Checking if it is a common feature
		int common=0;
		common = checking_for_feature_on_db(buf, db, stmt);

		//printf("Checking result: %d\n", common);

		if(common < MAXIMUM_NUM_COMMON_FEAT){
		//if(common <= MAXIMUM_NUM_COMMON_FEAT){

			uint32_t bits_set = bf_sha1_insert( bf, 0, (uint32_t *)sha1_hash);
		        // Avoid potentially repetitive features
		        if( !bits_set)
		            continue; 
		        if (num_indexes == 0) {
		            if (hashto->info !=NULL)
		                if (hashto->info->index) 
		                    hashto->info->index->insert_sha1((uint32_t*)sha1_hash);
		        } else {
		            if (hash_cnt % 4 ==0) {
		                hashes[hashindex][0]=sha1_hash[0]; hashes[hashindex][1]=sha1_hash[1]; hashes[hashindex][2]=sha1_hash[2]; hashes[hashindex][3]=sha1_hash[3]; hashes[hashindex][4]=sha1_hash[4];
		                bool any=hashto->check_indexes((uint32_t*)hashes[hashindex], &match);
		                if (any) 
		                   hashindex++;
		                if (hashindex >=192) 
		                   hashindex=192;  // no more than N matches per chunk
		            }
		        }
		        hash_cnt++;
		        if( chunk_scores[i] == threshold) 
		            allowed--;

		}
		/*
		else{
			cont_discarded++;
			printf("Feature descartada!\n");
		}
		*/
        }
    }
    if (num_indexes > 0 && hashto->info->search_first==false && hashto->info->search_deep==false) {
        // set level only for plug into assistance
        hashto->print_indexes(_FP_THRESHOLD,&match,block_num);

    } 
    hashto->reset_indexes(&match);
    hashto->elem_counts[block_num] = hash_cnt;

    //printf("\nFeatures descartadas: %d\n", cont_discarded);
    finalize_prepared_stmt(stmt);

    /* Close database */
    close_connection(db);

}

/**
 * Generate SDBF hash for a buffer--stream version.
 */
void
sdbf::gen_chunk_sdbf( uint8_t *file_buffer, uint64_t file_size, uint64_t chunk_size) {
    assert( chunk_size > config->pop_win_size);
    
    uint32_t i, k, sum;
    int32_t score_histo[66];  // Score histogram 
    uint64_t buff_size = ((file_size >> 11) + 1) << 8; // Estimate sdbf size (reallocate later)
    buff_size = (buff_size < 256) ? 256 : buff_size;                // Ensure min size
    this->buffer = (uint8_t *)alloc_check( ALLOC_ZERO, buff_size, "gen_chunk_sdbf", "sdbf_buffer", ERROR_EXIT);

    // Chunk-based computation
    uint64_t qt = file_size/chunk_size;
    uint64_t rem = file_size % chunk_size;

    uint64_t chunk_pos = 0;
    uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (chunk_size)*sizeof( uint16_t), "gen_chunk_sdbf", "chunk_ranks", ERROR_EXIT);
    uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (chunk_size)*sizeof( uint16_t), "gen_chunk_sdbf", "chunk_scores", ERROR_EXIT);

    for( i=0; i<qt; i++, chunk_pos+=chunk_size) {
        gen_chunk_ranks( file_buffer+chunk_size*i, chunk_size, chunk_ranks, 0);
        memset( score_histo, 0, sizeof( score_histo));
        gen_chunk_scores( chunk_ranks, chunk_size, chunk_scores, score_histo);

        // Calculate thresholding paremeters
        for( k=65, sum=0; k>=config->threshold; k--) {
            if( (sum <= this->max_elem) && (sum+score_histo[k] > this->max_elem))
                break;
            sum += score_histo[k];
        }
        //allowed = this->max_elem-sum;
        gen_chunk_hash( file_buffer, chunk_pos, chunk_scores, chunk_size);
    } 
    if( rem > 0) {
        gen_chunk_ranks( file_buffer+qt*chunk_size, rem, chunk_ranks, 0);
        gen_chunk_scores( chunk_ranks, rem, chunk_scores, 0);
        gen_chunk_hash( file_buffer, chunk_pos, chunk_scores, rem);
    }

    // Chop off last BF if its membership is too low (eliminates some FPs)
    if( this->bf_count > 1 && this->last_count < this->max_elem/8) {
        this->bf_count = this->bf_count-1;
        this->last_count = this->max_elem;
    }
    // Trim BF allocation to size
    if( this->bf_count*this->bf_size < buff_size) {
        this->buffer = (uint8_t*)realloc_check( this->buffer, (this->bf_count*this->bf_size));
    }
    free( chunk_ranks);
    free( chunk_scores);

}

/**
 * Worker thread for multi-threaded block hash generation.  // NOT iN CLASS?
 */
void *
sdbf::thread_gen_block_sdbf( void *task_param) {
    uint32_t i, k, sum, allowed;
    int32_t  score_histo[66];
    blockhash_task_t *hashtask = (blockhash_task_t *)task_param;
    uint64_t block_size = hashtask->block_size;
    uint8_t *buffer = hashtask->buffer;
    uint64_t file_size = hashtask->file_size;
    
    uint64_t qt = file_size/block_size;

    uint64_t chunk_pos = 0;
    uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (block_size)*sizeof( uint16_t), "gen_block_sdbf", "chunk_ranks", ERROR_EXIT);
    uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (block_size)*sizeof( uint16_t), "gen_block_sdbf", "chunk_scores", ERROR_EXIT);

    for( i=hashtask->tid; i<qt; i+=hashtask->tcount, chunk_pos+=hashtask->tcount*block_size) {
        gen_chunk_ranks( buffer+block_size*i, block_size, chunk_ranks, 0);
        memset( score_histo, 0, sizeof( score_histo));
        gen_chunk_scores( chunk_ranks, block_size, chunk_scores, score_histo);
        // Calculate thresholding paremeters
        for( k=65, sum=0; k>=config->threshold; k--) {
            if( (sum <= config->max_elem_dd) && (sum+score_histo[k] > config->max_elem_dd))
                break;
            sum += score_histo[k];
        }
        allowed = config->max_elem_dd-sum;
        gen_block_hash( buffer, file_size, i, chunk_scores, block_size, hashtask->sdbf, 0, k, allowed);
    } 
    free( chunk_ranks);
    free( chunk_scores);
    return NULL;
}

/** 
    dd-mode hash generation.
*/
void
sdbf::gen_block_sdbf_mt( uint8_t *file_buffer, uint64_t file_size, uint64_t block_size,  uint32_t thread_cnt) {
        
    blockhash_task_t *tasks = (blockhash_task_t *) alloc_check( ALLOC_ONLY, thread_cnt*sizeof( blockhash_task_t), "gen_block_sdbf_mt", "tasks", ERROR_EXIT);
    boost::thread *hash_pool[MAX_THREADS];
    uint32_t t;
    for( t=0; t<thread_cnt; t++) {
        tasks[t].tid = t;
        tasks[t].tcount = thread_cnt;
        tasks[t].buffer = file_buffer;
        tasks[t].file_size = file_size;
        tasks[t].block_size = block_size;
        tasks[t].sdbf = this;
        hash_pool[t] = new boost::thread(sdbf::thread_gen_block_sdbf,tasks+t);
    }
    for( t=0; t<thread_cnt; t++) {
        hash_pool[t]->join();
    }
    // Deal with the "tail" if necessary
    uint64_t qt = file_size/block_size;
    uint64_t rem = file_size % block_size;

    if( rem >= MIN_FILE_SIZE) {
        uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (block_size)*sizeof( uint16_t), "gen_block_sdbf_mt", "chunk_ranks", ERROR_EXIT);
        uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (block_size)*sizeof( uint16_t), "gen_block_sdbf_mt", "chunk_scores", ERROR_EXIT);

        gen_chunk_ranks( file_buffer+block_size*qt, rem, chunk_ranks, 0);
        gen_chunk_scores( chunk_ranks, rem, chunk_scores, NULL);
        gen_block_hash( file_buffer, file_size, qt, chunk_scores, block_size, this, rem, config->threshold, this->max_elem);     

        free( chunk_ranks);
        free( chunk_scores);
    }
    for( t=0; t<thread_cnt; t++) {
         delete hash_pool[t];
    }
    free(tasks);
}


/**
 * Calculates the score between two digests
 */
int 
sdbf::sdbf_score( sdbf *sdbf_1, sdbf *sdbf_2, uint32_t sample) {

    double max_score, score_sum = -1;
    uint32_t i;
    uint32_t bf_count_1, rand_offset;
    sdbf_task_t *tasklist = NULL;

    if( !sdbf_1->hamming)
        sdbf_1->compute_hamming();
    if( !sdbf_2->hamming)
        sdbf_2->compute_hamming();
        
    // if sampling, set sample count here. 
    if ((sample > 0 ) && (sdbf_1->bf_count > sample)) {
        bf_count_1 = sample;
    } else {
        bf_count_1 = sdbf_1->bf_count;
    }
    // Make sure |sdbf_1| <<< |sdbf_2| 
    // does this matter anymore?
    if( (bf_count_1 > sdbf_2->bf_count) ||
        (bf_count_1 == sdbf_2->bf_count && 
            ((get_elem_count( sdbf_1, bf_count_1-1) > get_elem_count( sdbf_2, sdbf_2->bf_count-1)) ||
              strcmp( (char*)sdbf_1->hashname, (char*)sdbf_2->hashname) > 0 ))) {
            sdbf *tmp = sdbf_1;
            sdbf_1 = sdbf_2;
            sdbf_2 = tmp;
            bf_count_1 = sdbf_1->bf_count;
    }
   
    tasklist = (sdbf_task_t *) alloc_check( ALLOC_ZERO, sizeof( sdbf_task_t), "sdbf_score", "tasklist", ERROR_EXIT);
    tasklist[0].tid = 0;
    tasklist[0].tcount = 1;
    tasklist[0].ref_sdbf = sdbf_1;
    tasklist[0].tgt_sdbf = sdbf_2;
    tasklist[0].done = 0;
    rand_offset = 1 ;
    srand(time(NULL));
    int sparsect=0;
    for( i=0; i< bf_count_1; i++) {
        if (sample > 0 && bf_count_1 > sample)  {
            rand_offset = rand() % (uint32_t)(sdbf_1->bf_count / sample);
        }
        tasklist[0].ref_index=i*rand_offset;
        max_score = sdbf_max_score( tasklist);
        score_sum = (score_sum < 0) ? max_score : score_sum + max_score;
        if (get_elem_count( sdbf_1, i) < MIN_ELEM_COUNT ) 
            sparsect++;
    }
    uint64_t denom = bf_count_1;
    // improving the average.
    if (bf_count_1 > 1) 
        denom-=sparsect;
    free(tasklist);
    return (score_sum < 0) ? -1 : boost::math::round( 100.0*score_sum/(denom));
    
}

/**
 * Given a BF and an SDBF, calculates the maximum match (0-100)
 */

double 
sdbf::sdbf_max_score( sdbf_task_t *task) {
    assert( task != NULL);
        
    double score, max_score=-1;
    uint32_t i, s1, s2, min_est, max_est, match, cut_off, slack=48;
    uint32_t bf_size = task->ref_sdbf->bf_size;
    uint16_t *bf_1, *bf_2;

    s1 = get_elem_count( task->ref_sdbf, task->ref_index);
    if( s1 < MIN_ELEM_COUNT)
        return 0;
    bf_1 = (uint16_t *)(task->ref_sdbf->buffer + task->ref_index*bf_size);
    uint32_t e1_cnt = task->ref_sdbf->hamming[task->ref_index];
    uint32_t comp_cnt = task->tgt_sdbf->bf_count;
    for( i=task->tid; i<comp_cnt; i+=task->tcount) {
        bf_2 = (uint16_t *)(task->tgt_sdbf->buffer + i*bf_size);
        s2 = get_elem_count( task->tgt_sdbf, i);
        if( task->ref_sdbf->bf_count > 1 && s2 < MIN_ELEM_COUNT)
            continue;
        uint32_t e2_cnt = task->tgt_sdbf->hamming[i];

        // Max/min number of matching bits & zero cut off
        max_est = (e1_cnt < e2_cnt) ? e1_cnt : e2_cnt;
        min_est = bf_match_est( 8*bf_size, task->ref_sdbf->hash_count, s1, s2, 0);
        cut_off = boost::math::round( SD_SCORE_SCALE*(double)(max_est-min_est)+(double)min_est);
        // Find matching bits
        if (config->popcnt) {
            match = bf_bitcount_cut_256_asm( (uint8_t *)bf_1, (uint8_t *)bf_2, cut_off, slack);
            if( match > 0) {
                match = bf_bitcount_cut_256_asm( (uint8_t *)bf_1, (uint8_t *)bf_2, 0, 0);
            }
        } else {
            match = bf_bitcount_cut_256( (uint8_t *)bf_1, (uint8_t *)bf_2, cut_off, slack);
            if( match > 0) {
                match = bf_bitcount_cut_256( (uint8_t *)bf_1, (uint8_t *)bf_2, 0, 0);
            }

        }
        score = (match <= cut_off) ? 0 : (double)(match-cut_off)/(max_est-cut_off);
        max_score = (score > max_score) ? score : max_score;
    }

    task->result = max_score;

    return max_score;
}

void
sdbf::print_indexes(uint32_t threshold, vector<uint32_t> *matches, uint64_t pos){
    uint32_t count=this->info->setlist->size();
    std::stringstream build;
    bool any=false;
    for (uint32_t i=0;i<count;i++) {
        if (matches->at(i) > threshold) {
            build << this->name() <<" [" << pos << "] |" << this->info->setlist->at(i)->name() << "|" << matches->at(i) << endl;
            any=true;
        }
    }
    if (any) {
        boost::lock_guard<boost::mutex> lock(index_output);
        index_results.append(build.str());
    }
}

void 
sdbf::reset_indexes(vector<uint32_t> *matches){
    std::fill(matches->begin(),matches->end(),0);
    if (matches->size()==1) 
    matches->at(0)=0;
}

bool
sdbf::check_indexes(uint32_t* sha1, vector<uint32_t>* matches) {
    uint32_t count=this->info->setlist->size();
    bool any=false;
    for (uint32_t i=0;i<count;i++) {
        if (this->info->setlist->at(i)->index->query_sha1(sha1)) {
            matches->at(i)++;
            any=true;
        }
    }
    return any;
}    
