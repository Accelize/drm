/*
Copyright (C) 2018, Accelize

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <fstream>

#include "log.h"

namespace Accelize {
    namespace DRM {

        eLogLevel sLogVerbosity = eLogLevel::ERROR;
        eLogFormat sLogFormat = eLogFormat::SHORT;
        std::ostream *sLogStream = &std::cout;


        void initLog() {
            /*std::string file_path("test.log");

            std::ofstream *ofs = new std::ofstream(file_path);
            if (!ofs->is_open())
                Throw( DRM_BadUsage, "Failed to open file ", file_path );
            sLogStream = ofs;*/
        }

        void uninitLog() {
            //delete sLogStream;
        }
    }
}
