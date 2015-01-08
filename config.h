//
/// \file config.h
/// \author James Marlowe
/// \brief functions for config file reading
//

#include <fstream>
#include "bson/json/jsonloader.h"

bson::Document parse_config(const std::string & fname);
