/**
 * @file factoryDatabase.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in database factory
 * @version 0.1
 * @date 2020-01-26
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/factoryDatabase.h"


AbsDatabase* DatabaseFactory::CreateDatabase(int type, string path) {
    switch (type) {
        case LEVEL_DB: 
            fprintf(stderr, "Database: using LevelDB.\n");
            return new LeveldbDatabase(path);
            break;
        case IN_MEMORY:
            fprintf(stderr, "Database: using In-Memory Index.\n");
            return new InMemoryDatabase(path);
            break;
        default:
            break;
    }
    fprintf(stderr, "Database Factory: error type.\n");
    return NULL;
}
