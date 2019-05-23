# cbamf
Common Blocks and the Approaximate Matching Functions

The source codes provided here are a result of a study present on the following papers:

    . Moia, V.H.G., Breitinger, F., and Henriques, M.A.A. (2019). The impact of excluding common blocks for approximate matching. Computers & Security. Status: Submitted.
  
    . Moia, V.H.G., Breitinger, F., and Henriques, M.A.A. (2019). Improving the similarity detection on digital forensics investigations using approximate matching functions by removing common blocks (still in progress).

How to create the common feature database and populate it using an Approximate Matching function

  1. Create the common feature database (folder: creating_common_feature_database).
  2. Extract the features of a given data set using one of the feature extraction tools provided here (folder: Feature_extraction_tools).
  3. Run the following SQL queries to update the common features tables of the DB:
  
    QUERY: INSERT INTO common_features_sdhash (HASH, CONT, CONT_DIFF) SELECT HASH, COUNT(HASH) HASH, count(distinct ID_OBJ) HASH_DIFF_FILES FROM features_sdhash GROUP BY hash HAVING COUNT(HASH) > 0;
    
  4. Use the approximate matching tool of your choice (NCF_sdhash or NCF_mrsh-v2) to create the digest of a given file / set of files.
