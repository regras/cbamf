/*
    Code adapted from sdhash (https://github.com/sdhash/sdhash)

    Vitor Hugo Galhardo Moia, Ph.D student
    Department of Computer Engineering and Industrial Automation (DCA)
    School of Electrical and Computer Engineering (FEEC)
    University of Campinas (UNICAMP)
    Campinas, SP, Brazil 13083-852
    Email: vhgmoia@dca.fee.unicamp.br / vitormoia@gmail.com
    page: http://www.dca.fee.unicamp.br/~vhgmoia/

    March, 28th 2019

    File: feature_extraction_sdhash.c
    Purpose: Extract sdhash features from objects and insert them into a database

    INPUT: 2 arguments.
	_ database: Path of the SQLite3 database to store the extracted features.
	_ list_of_files: Path of a txt file containing all objects that will have their features extracted.

    OUTPUT: None.
*/

#include <iostream>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>

#include "util_sql.h"
#include <pthread.h>

using namespace std;

/***** PARAMETERS *****/

#define KB 1024
#define MB (KB*KB)
#define ALLOC_ONLY	1
#define ALLOC_ZERO	2
#define ALLOC_AUTO	3
#define ERROR_EXIT	1
#define BINS		1000
#define ENTR_POWER      10        
#define ENTR_SCALE	(BINS*(1 << ENTR_POWER))
#define MIN_FILE_SIZE   512
#define SIZE_LINE 200

/* Choose which hash function to use */

//#define SHA1
//#define SIZE_HASH_BUFFER 40
//#define HASH_OUTPUT_SIZE 20

/*      OR      */

#define FNV64
#define SIZE_HASH_BUFFER 16
#define HASH_OUTPUT_SIZE 16


/* Objects responsible to temporarily storing the features */

struct features_obj{
	char hash[40];
	char offset[20];
	unsigned int size;
	features_obj *next;
};

struct feature_set{
	int id_obj; 
	features_obj *feature_set; 
	sqlite3 *db;
};

/* Database connection objects */
sqlite3 *db;

/* Variables to count the number of features */
int num_obj_features=0;
long num_global_features=0;

/* sdhash parameters */
uint32_t max_elem_dd = 192;
uint32_t max_elem = 160;
uint32_t bf_size = 256;
uint32_t pop_win_size=64;
uint32_t threshold = 16;
uint32_t entr_win_size=64;
uint32_t  block_size = 4*KB;

uint32_t BF_CLASS_MASKS[] = { 0x7FF, 0x7FFF, 0x7FFFF, 0x7FFFFF, 0x7FFFFFF, 0xFFFFFFFF};
uint8_t BITS[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
static uint64_t ENTROPY_64_INT[65];
uint32_t ENTR64_RANKS[] = {
    000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
    101, 102, 106, 112, 108, 107, 103, 100, 109, 113, 128, 131, 141, 111, 146, 153, 148, 134, 145, 110,
    114, 116, 130, 124, 119, 105, 104, 118, 120, 132, 164, 180, 160, 229, 257, 211, 189, 154, 127, 115,
    129, 142, 138, 125, 136, 126, 155, 156, 172, 144, 158, 117, 203, 214, 221, 207, 201, 123, 122, 121,
    135, 140, 157, 150, 170, 387, 390, 365, 368, 341, 165, 166, 194, 174, 184, 133, 139, 137, 149, 173,
    162, 152, 159, 167, 190, 209, 238, 215, 222, 206, 205, 181, 176, 168, 147, 143, 169, 161, 249, 258,
    259, 254, 262, 217, 185, 186, 177, 183, 175, 188, 192, 195, 182, 151, 163, 199, 239, 265, 268, 242,
    204, 197, 193, 191, 218, 208, 171, 178, 241, 200, 236, 293, 301, 256, 260, 290, 240, 216, 237, 255,
    232, 233, 225, 210, 196, 179, 202, 212, 420, 429, 425, 421, 427, 250, 224, 234, 219, 230, 220, 269,
    247, 261, 235, 327, 332, 337, 342, 340, 252, 187, 223, 198, 245, 243, 263, 228, 248, 231, 275, 264,
    298, 310, 305, 309, 270, 266, 251, 244, 213, 227, 273, 284, 281, 318, 317, 267, 291, 278, 279, 303,
    452, 456, 453, 446, 450, 253, 226, 246, 271, 277, 295, 302, 299, 274, 276, 285, 292, 289, 272, 300,
    297, 286, 314, 311, 287, 283, 288, 280, 296, 304, 308, 282, 402, 404, 401, 415, 418, 313, 320, 307,
    315, 294, 306, 326, 321, 331, 336, 334, 316, 328, 322, 324, 325, 330, 329, 312, 319, 323, 352, 345,
    358, 373, 333, 346, 338, 351, 343, 405, 389, 396, 392, 411, 378, 350, 388, 407, 423, 419, 409, 395,
    353, 355, 428, 441, 449, 474, 475, 432, 457, 448, 435, 462, 470, 467, 468, 473, 426, 494, 487, 506,
    504, 517, 465, 459, 439, 472, 522, 520, 541, 540, 527, 482, 483, 476, 480, 721, 752, 751, 728, 730,
    490, 493, 495, 512, 536, 535, 515, 528, 518, 507, 513, 514, 529, 516, 498, 492, 519, 508, 544, 547,
    550, 546, 545, 511, 532, 543, 610, 612, 619, 649, 691, 561, 574, 591, 572, 553, 551, 565, 597, 593,
    580, 581, 642, 578, 573, 626, 696, 584, 585, 595, 590, 576, 579, 583, 605, 569, 560, 558, 570, 556,
    571, 656, 657, 622, 624, 631, 555, 566, 564, 562, 557, 582, 589, 603, 598, 604, 586, 577, 588, 613,
    615, 632, 658, 625, 609, 614, 592, 600, 606, 646, 660, 666, 679, 685, 640, 645, 675, 681, 672, 747,
    723, 722, 697, 686, 601, 647, 677, 741, 753, 750, 715, 707, 651, 638, 648, 662, 667, 670, 684, 674,
    693, 678, 664, 652, 663, 639, 680, 682, 698, 695, 702, 650, 676, 669, 665, 688, 687, 701, 700, 706,
    683, 718, 703, 713, 720, 716, 735, 719, 737, 726, 744, 736, 742, 740, 739, 731, 711, 725, 710, 704,
    708, 689, 729, 727, 738, 724, 733, 692, 659, 705, 654, 690, 655, 671, 628, 634, 621, 616, 630, 599,
    629, 611, 620, 607, 623, 618, 617, 635, 636, 641, 637, 633, 644, 653, 699, 694, 714, 734, 732, 746,
    749, 755, 745, 757, 756, 758, 759, 761, 763, 765, 767, 771, 773, 774, 775, 778, 782, 784, 786, 788,
    793, 794, 797, 798, 803, 804, 807, 809, 816, 818, 821, 823, 826, 828, 829, 834, 835, 839, 843, 846,
    850, 859, 868, 880, 885, 893, 898, 901, 904, 910, 911, 913, 916, 919, 922, 924, 930, 927, 931, 938,
    940, 937, 939, 941, 934, 936, 932, 933, 929, 928, 926, 925, 923, 921, 920, 918, 917, 915, 914, 912,
    909, 908, 907, 906, 900, 903, 902, 905, 896, 899, 897, 895, 891, 894, 892, 889, 883, 890, 888, 879,
    887, 886, 882, 878, 884, 877, 875, 872, 876, 870, 867, 874, 873, 871, 869, 881, 863, 865, 864, 860,
    853, 855, 852, 849, 857, 856, 862, 858, 861, 854, 851, 848, 847, 845, 844, 841, 840, 837, 836, 833,
    832, 831, 830, 827, 824, 825, 822, 820, 819, 817, 815, 812, 814, 810, 808, 806, 805, 799, 796, 795,
    790, 787, 785, 783, 781, 777, 776, 772, 770, 768, 769, 764, 762, 760, 754, 743, 717, 712, 668, 661,
    643, 627, 608, 594, 587, 568, 559, 552, 548, 542, 539, 537, 534, 533, 531, 525, 521, 510, 505, 497,
    496, 491, 486, 485, 478, 477, 466, 469, 463, 458, 460, 444, 440, 424, 433, 403, 410, 394, 393, 385,
    377, 379, 382, 383, 380, 384, 372, 370, 375, 366, 354, 363, 349, 357, 347, 364, 367, 359, 369, 360,
    374, 344, 376, 335, 371, 339, 361, 348, 356, 362, 381, 386, 391, 397, 399, 398, 412, 408, 414, 422,
    416, 430, 417, 434, 400, 436, 437, 438, 442, 443, 447, 406, 451, 413, 454, 431, 455, 445, 461, 464,
    471, 479, 481, 484, 489, 488, 499, 500, 509, 530, 523, 538, 526, 549, 554, 563, 602, 596, 673, 567,
    748, 575, 766, 709, 779, 780, 789, 813, 811, 838, 842, 866, 942, 935, 944, 943, 947, 952, 951, 955,
    954, 957, 960, 959, 967, 966, 969, 962, 968, 953, 972, 961, 982, 979, 978, 981, 980, 990, 987, 988,
    984, 983, 989, 985, 986, 977, 976, 975, 973, 974, 970, 971, 965, 964, 963, 956, 958, 524, 950, 948,
    949, 945, 946, 800, 801, 802, 791, 792, 501, 502, 503, 000, 000, 000, 000, 000, 000, 000, 000, 000,
    000 };


/** sdbf class */
class sdbf {

    friend std::ostream& operator<<(std::ostream& os, const sdbf& s); ///< output operator
    friend std::ostream& operator<<(std::ostream& os, const sdbf *s); ///< output operator

public:

    /// to create by reading from an open stream
    sdbf(const char *name, std::istream *ifs, uint32_t dd_block_size, uint64_t msize) ; 

private:

    void sdbf_create();
    static void gen_chunk_ranks( uint8_t *file_buffer, const uint64_t chunk_size, uint16_t *chunk_ranks, uint16_t carryover);
    static void gen_chunk_scores( const uint16_t *chunk_ranks, const uint64_t chunk_size, uint16_t *chunk_scores, int32_t *score_histo);
    void gen_chunk_hash( uint8_t *file_buffer, const uint64_t chunk_pos, const uint16_t *chunk_scores, const uint64_t chunk_size, int id_obj, sqlite3 *db);
    static void gen_block_hash( uint8_t *file_buffer, uint64_t file_size, const uint64_t block_num, const uint16_t *chunk_scores, const uint64_t block_size, class sdbf *hashto,uint32_t rem, uint32_t threshold, int32_t allowed);
    void gen_chunk_sdbf( uint8_t *file_buffer, uint64_t file_size, uint64_t chunk_size, int id_obj, sqlite3 *db);
    void gen_block_sdbf_mt( uint8_t *file_buffer, uint64_t file_size, uint64_t block_size);
    
public:
    uint8_t  *buffer;        // Beginning of the BF cluster
    uint32_t  max_elem;      // Max number of elements per filter (n)
private:

    // from the C structure 
    uint32_t  bf_count;      // Number of BFs
    uint32_t  bf_size;       // BF size in bytes (==m/8)
    uint32_t  hash_count;    // Number of hash functions used (k)
    uint32_t  mask;          // Bit mask used (must agree with m)
    uint32_t  last_count;    // Actual number of elements in last filter (n_last); 
                                                         // ZERO means look at elem_counts value 
    uint16_t *elem_counts;   // Individual elements counts for each BF (used in dd mode)
    uint32_t  dd_block_size; // Size of the base block in dd mode
    uint64_t orig_file_size; // size of the original file
};



/***** ENTROPY FUNCTIONS *****/

/** Entropy lookup table setup--int64 version (to be called once) */
void entr64_table_init_int() {
    uint32_t i;
    ENTROPY_64_INT[0] = 0;
    for( i=1; i<=64; i++) {
        double p = (double)i/64;
        ENTROPY_64_INT[i] = (uint64_t) ((-p*(log(p)/log((double)2))/6)*ENTR_SCALE);
    }
}

/** Baseline entropy computation for a 64-byte buffer--int64 version (to be called periodically) */
uint64_t entr64_init_int( const uint8_t *buffer, uint8_t *ascii) {
    uint32_t i;

    //copy 0 to the 256 first positions of ascii array.
    memset( ascii, 0, 256);

    for( i=0; i<64; i++) {
        uint8_t bf = buffer[i];
        ascii[bf]++;
    }

    uint64_t entr=0;
    for( i=0; i<256; i++)
    if( ascii[i])
        entr += ENTROPY_64_INT[ ascii[i]];
    
    //printf("\nEntropy: %zu\n", entr);
    return entr;
}

/** Incremental (rolling) update to entropy computation--int64 version */
uint64_t entr64_inc_int( uint64_t prev_entropy, const uint8_t *buffer, uint8_t *ascii) {
  if( buffer[0] == buffer[64])
    return prev_entropy;

  uint32_t old_char_cnt = ascii[buffer[0]];
  uint32_t new_char_cnt = ascii[buffer[64]];

  ascii[buffer[0]]--;
  ascii[buffer[64]]++;
 
  if( old_char_cnt == new_char_cnt+1)
    return prev_entropy;

  int64_t old_diff = int64_t(ENTROPY_64_INT[old_char_cnt])   - int64_t(ENTROPY_64_INT[old_char_cnt-1]);
  int64_t new_diff = int64_t(ENTROPY_64_INT[new_char_cnt+1]) - int64_t(ENTROPY_64_INT[new_char_cnt]);

  int64_t entropy =int64_t(prev_entropy) - old_diff + new_diff;
  if( entropy < 0)
    entropy = 0;
  else if( entropy > ENTR_SCALE)
    entropy = ENTR_SCALE;

  return (uint64_t)entropy;
}


/***** GENERAL FUNCTIONS *****/

/**
 * Allocate memory, check result, and print error (if necessary).
 */
void *alloc_check( uint32_t alloc_type, uint64_t mem_bytes, const char *fun_name, const char *var_name, uint32_t error_action) {
    void *mem_chunk = NULL;

    switch( alloc_type) {
        case ALLOC_ONLY:
            mem_chunk = malloc( mem_bytes);
            break;
        case ALLOC_AUTO:
            mem_chunk = malloc( mem_bytes);
            break;
        case ALLOC_ZERO:
            mem_chunk = calloc( 1, mem_bytes);
            break;
        default:
            return NULL;
    }
    if( mem_chunk == NULL) {
        fprintf( stderr, "Could not allocate %dKB (%dMB) for %s in function %s(). System message: \"%s\".", 
                        (int)(mem_bytes >> 10), (int)(mem_bytes >> 20), var_name, fun_name, strerror( errno));
        if( error_action == ERROR_EXIT) {
            fprintf( stderr, " Exiting.\n");
            exit(-1);
        }
    }
    return mem_chunk;
}


/***** FNV-1a HASH FUNCTION *****/

 void fnv1a(const void* data, size_t numBytes, unsigned char *fnv_hash) {

    const uint64_t Prime = 1099511628211;
    const uint64_t Seed  = 0x811C9DC5A9B3C6D8;

    uint64_t hash = Seed;

    assert(data);
    const unsigned char* ptr = (const unsigned char*)data;
    while (numBytes--)
      hash = (*ptr++ ^ hash) * Prime;
      // same as hash = fnv1a(*ptr++, hash); but much faster in debug mode
    
    sprintf((char*)fnv_hash, "%16" PRIx64 "\n", hash);
}



/***** SDBF FUNCTIONS *****/

/** Create and initialize an sdbf structure ready for stream mode. */
void 
sdbf::sdbf_create() {
    this->bf_size = bf_size;
    this->hash_count = 5;
    this->mask = BF_CLASS_MASKS[0];
    this->bf_count = 1;
    this->last_count = 0;
    this->elem_counts= 0;
    this->orig_file_size = 0;
    this->buffer = NULL;
}


/**
 * Generate SHA1/FNV-1a hashes and add them to the SDBF--original stream version.
 */
void 
sdbf::gen_chunk_hash( uint8_t *file_buffer, const uint64_t chunk_pos, const uint16_t *chunk_scores, const uint64_t chunk_size, int id_obj, sqlite3 *db) {

    uint64_t i;
    uint8_t sha1_hash[HASH_OUTPUT_SIZE];

    features_obj *features = NULL;	
    features_obj *last;

    if (chunk_size > pop_win_size) {
        for( i=0; i<chunk_size-pop_win_size; i++) {
            if( chunk_scores[i] > threshold) {
		
		num_obj_features++;	//counting the number of features
		uint64_t offset = i+chunk_pos;
		char buf[SIZE_HASH_BUFFER];

#ifdef SHA1

                //SHA1( file_buffer+offset, pop_win_size, sha1_hash);
                //for(int n=0; n < 20; n++){
                //	sprintf((char*)&buf[n*2], "%02X", sha1_hash[n]);
                    //printf("%02X", sha1_hash[n]);
                //}

#endif
#ifdef FNV64

                int j = 0;
                fnv1a(file_buffer+offset, pop_win_size, sha1_hash);
                for (j=0; j < SIZE_HASH_BUFFER; j++)
                    (buf[j]) = sha1_hash[j];

                buf[j] = '\0';
#endif

                //printf("\nHASH: %s \n", buf);

                if(features != NULL && features->size > 0){

                    features_obj *temp = (struct features_obj*) malloc(sizeof(features_obj));
                    strcpy(temp->hash, buf);
                    sprintf(temp->offset, "%X", (char*)offset);
                    temp->size = pop_win_size;
                    temp->next = NULL;
                    last->next = temp;
                    last = temp;
                }
                else {

                    features = (struct features_obj*) malloc(sizeof(features_obj));
                    strcpy(features->hash, buf);
                    sprintf(features->offset, "%X", (char*)offset);
                    features->size = pop_win_size;
                    features->next = NULL;
                    last = features;
                }
            }
        }
    }

    sqlite3_stmt* stmt = prepared_insert_feature_statement(db);
    features_obj *temp;

    while(features != NULL){
	    temp = features;
	    /* adding feature to database */
	    inserting_new_feature_prepared_stmt(id_obj, temp->hash, temp->size, temp->offset, db, stmt);
	    features = temp->next;
	    free(temp);
    }

    free(features);
    finalize_prepared_stmt(stmt);
}


/**
 * Generate scores for a ranks chunk.
 */
void 
sdbf::gen_chunk_scores( const uint16_t *chunk_ranks, const uint64_t chunk_size, uint16_t *chunk_scores, int32_t *score_histo) { 
    uint64_t i, j;
    uint32_t pop_win = pop_win_size;
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

    int64_t limit = int64_t(chunk_size)-int64_t(entr_win_size);
    if(limit >0) {
        for (offset=0; offset<limit; offset++) {
            // Initial/sync entropy calculation (entropy of the first window)
            if ( offset % block_size == 0) {
                entropy = entr64_init_int( file_buffer+offset, ascii);
            // Incremental entropy update (much faster)
            } else {
                entropy = entr64_inc_int( entropy, file_buffer+offset-1, ascii);
            }
            chunk_ranks[offset] = ENTR64_RANKS[entropy >> ENTR_POWER];
        }
    }
    free( ascii);
}

/**
 * Generate SDBF hash for a buffer--stream version.
 */
void
sdbf::gen_chunk_sdbf( uint8_t *file_buffer, uint64_t file_size, uint64_t chunk_size, int id_obj, sqlite3 *db) {

    assert( chunk_size > pop_win_size);

    uint32_t i, k, sum;
    int32_t score_histo[66];  // Score histogram 
    
    /* broke file into pieces of size chunk_size */
    // Chunk-based computation
    uint64_t qt = file_size/chunk_size;
    uint64_t rem = file_size % chunk_size;

    uint64_t chunk_pos = 0;
    uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (chunk_size)*sizeof( uint16_t), "gen_chunk_sdbf", "chunk_ranks", ERROR_EXIT);
    uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (chunk_size)*sizeof( uint16_t), "gen_chunk_sdbf", "chunk_scores", ERROR_EXIT);

    /* Hashing each piece */
    for( i=0; i < qt; i++, chunk_pos+=chunk_size) {
        gen_chunk_ranks( file_buffer+chunk_size*i, chunk_size, chunk_ranks, 0);
        memset( score_histo, 0, sizeof( score_histo));
        gen_chunk_scores( chunk_ranks, chunk_size, chunk_scores, score_histo);

        // Calculate thresholding paremeters
        for( k=65, sum=0; k>=threshold; k--) {
            if( (sum <= this->max_elem) && (sum+score_histo[k] > this->max_elem))
                break;
            sum += score_histo[k];
        }

        gen_chunk_hash( file_buffer, chunk_pos, chunk_scores, chunk_size, id_obj, db);
    } 
    if( rem > 0) {

	/* hashing the piece not multiple of chunk_size */
        gen_chunk_ranks( file_buffer+qt*chunk_size, rem, chunk_ranks, 0);
        gen_chunk_scores( chunk_ranks, rem, chunk_scores, 0);
        gen_chunk_hash( file_buffer, chunk_pos, chunk_scores, rem, id_obj, db);
    }

    free( chunk_ranks);
    free( chunk_scores);

}

/**
 * Generate SHA1 hashes and add them to the SDBF--block-aligned version.
 */
void 
sdbf::gen_block_hash( uint8_t *file_buffer, uint64_t file_size, const uint64_t block_num, const uint16_t *chunk_scores, const uint64_t block_size, class sdbf *hashto, uint32_t rem, uint32_t threshold_param, int32_t allowed) {

    /* NOT IMPLEMENTED */
}

/** 
    dd-mode hash generation.
*/
void
sdbf::gen_block_sdbf_mt( uint8_t *file_buffer, uint64_t file_size, uint64_t block_size) {

    // Deal with the "tail" if necessary
    uint64_t qt = file_size/block_size;
    uint64_t rem = file_size % block_size;

    if( rem >= MIN_FILE_SIZE) {
        uint16_t *chunk_ranks = (uint16_t *)alloc_check( ALLOC_ONLY, (block_size)*sizeof( uint16_t), "gen_block_sdbf_mt", "chunk_ranks", ERROR_EXIT);
        uint16_t *chunk_scores = (uint16_t *)alloc_check( ALLOC_ZERO, (block_size)*sizeof( uint16_t), "gen_block_sdbf_mt", "chunk_scores", ERROR_EXIT);

        gen_chunk_ranks( file_buffer+block_size*qt, rem, chunk_ranks, 0);
        gen_chunk_scores( chunk_ranks, rem, chunk_scores, NULL);

	    /* ( NOT IMPLEMENTED ) */
        gen_block_hash( file_buffer, file_size, qt, chunk_scores, block_size, this, rem, threshold, this->max_elem);     

        free( chunk_ranks);
        free( chunk_scores);
    }
}

/**
    Generates a new sdbf, with a maximum size read from an open stream.
    dd_block_size enables block mode.
    \param name name of stream
    \param ifs open istream to read raw data from
    \param dd_block_size size of block to divide data with. 0 is off.
    \param msize amount of data to read and process

    \throws exception if file cannot be opened or too small 
*/
sdbf::sdbf(const char *name, std::istream *ifs, uint32_t dd_block_size, uint64_t msize) {

    entr64_table_init_int();

    uint64_t chunk_size;
    uint8_t *bufferinput;

    bufferinput = (uint8_t*)alloc_check(ALLOC_ZERO, sizeof(uint8_t)*msize,"sdbf_hash_stream", "buffer input", ERROR_EXIT);

    name = basename(name);

    /* Verifying if the registry already exists and if does getting ID */
    int id_obj = getting_id_from_objects_tb(name, db);

    //printf("\nID: %d\n", id_obj);

    if (id_obj < 0) {

       	/* CREATING A NEW REGISTRY */
	    id_obj = inserting_new_obj_into_objects_tb(name, get_filename_ext(name), msize, db);

	    if(id_obj < 0){
		    printf("\nError! Object could not be inserted into database!\n");
		    return;
	    }
    }
    else {
	/* Removing existing features before inserting new ones
		(avoid duplicate entries)
	 */
	remove_existing_features(db, id_obj);
    }

    /* Extracts msize character from the stream (file) and stores them into bufferinput */
    ifs->read((char*)bufferinput,msize);

    chunk_size = ifs->gcount();

    //If file size is lesser than 512 bytes
    if (chunk_size < MIN_FILE_SIZE) {
        free(bufferinput);
        throw -3; // too small
    }

    sdbf_create();

    this->orig_file_size=chunk_size;
    if (!dd_block_size) {  // single stream mode should not be used but we'll support it anyway

        this->max_elem = max_elem;
        gen_chunk_sdbf(bufferinput, msize, 32*MB, id_obj, db);

    } 
    else { // block mode
	this->max_elem = max_elem_dd;
        uint64_t dd_block_cnt =  msize/dd_block_size;
        if( msize % dd_block_size >= MIN_FILE_SIZE)
            dd_block_cnt++;
        this->bf_count = dd_block_cnt;
        this->dd_block_size = dd_block_size;
        this->buffer = (uint8_t *)alloc_check( ALLOC_ZERO, dd_block_cnt*bf_size, "sdbf_hash_dd", "this->buffer", ERROR_EXIT);
        this->elem_counts = (uint16_t *)alloc_check( ALLOC_ZERO, sizeof( uint16_t)*dd_block_cnt, "sdbf_hash_dd", "this->elem_counts", ERROR_EXIT);
        gen_block_sdbf_mt( bufferinput, msize, dd_block_size);
    }

    free(bufferinput);
}


void sdbf_hash_files(char *name) {

    struct stat file_stat;
    ifstream *is = new ifstream();

    if(stat(name, &file_stat) != 0)
        return;

    is->open(name, ios::binary);

    try 
    {
        sdbf(name, is, 0, file_stat.st_size);
    } 
    catch (int e) 
    {
	if (e==-2)
	   exit(-2);
    }
    is->close();
    delete is;
}


/***** MAIN FUNCTION *****/

int main(int argn, char *argv[]){

	//To compile the code, add: -lssl -lcrypto

	printf("\n**************** SDHASH FEATURE EXTRACTION ****************\n\n");

	if(argn < 3){
		printf("Missing parameters: \n" \
		"\t1. Database name;\n"\
		"\t2. List of files (txt file).\n");
		return -1;
	}

	FILE *arq;
	char* list = argv[2];
	int num_files=0;
	char linha[SIZE_LINE];
	
	arq = fopen(list, "r");

	if(!arq){
		fprintf(stderr, "Couldn't open %s\n", list);
		exit(1);
	}

	printf("Openning database: ");

	/* Open database */
	db = open_connection(argv[1]);

	printf("[OK]\nStarting process:");
	
	while (!feof(arq))
	{
		if( fgets (linha, SIZE_LINE, arq)!=NULL )
		{
			char *pos;
			if ((pos=strchr(linha, '\n')) != NULL)
				*pos = '\0';

			printf("\n\tProcessing file: %s\n", linha);
			sdbf_hash_files(linha);
			printf("\t\tNum. features: %d", num_obj_features);
			num_global_features+= num_obj_features;
			num_obj_features=0;
			num_files++;
		}
	}

	/* Close file */
	fclose(arq);

	printf("\nProcess complete.\nClosing database: ");

	/* Close database */
    	close_connection(db);

	printf("[OK]\n\nStatistics: \n\tNumber of files processed: %d\n\tNumber of features extracted: %d\n\n", num_files, num_global_features);

	printf("Exiting program...\n");

	return 0;
	
}
