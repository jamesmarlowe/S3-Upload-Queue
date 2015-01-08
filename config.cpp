//
/// \file config.cpp
/// \author James Marlowe
/// \brief Implementation of bson for config file reading
//

#include "config.h"

bson::Document parse_config(const std::string & fname)
{
  std::ifstream fin(fname.c_str());
  bson::JSON_Loader loader;
  bson::Document confd(loader.parse(fin));
  return confd;
}

