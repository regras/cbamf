# cbamf
Common Blocks and Approaximate Matching Functions

The source codes provided here are a result of a study present on the following papers:

    . Moia, V.H.G., Breitinger, F., and Henriques, M.A.A. (2019). The impact of excluding common blocks for approximate matching. Computers & Security. Status: Submitted.
  
    . Moia, V.H.G., Breitinger, F., and Henriques, M.A.A. (2019). Understanding the effects of removing common blocks on approximate matching scores under different scenarios for digital forensic investigations. In XIX Brazilian Symposium on information and computational systems security (pp. 1-14). São Paulo-SP, Brazil: Brazilian Computer Society (SBC).

How to create the common feature database and populate it using an Approximate Matching function:

  1. Create the common feature database (folder: creating_common_feature_database).
  2. Extract the features of a given data set using one of the feature extraction tools provided here (folder: Feature_extraction_tools).
  3. Run the following SQL query on the common feature database:
  
    QUERY: INSERT INTO common_features_sdhash (HASH, CONT, CONT_DIFF) SELECT HASH, COUNT(HASH) HASH, count(distinct ID_OBJ) HASH_DIFF_FILES FROM features_sdhash GROUP BY hash HAVING COUNT(HASH) > 0;
    
  4. Use the approximate matching tool of your choice (NCF_sdhash or NCF_mrsh-v2) to create the digest of a given file / set of files.
