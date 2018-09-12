/**
*  \file      DrmControllerRegisters.cpp
*  \version   3.0.0.1
*  \date      September 2018
*  \brief     Class DrmControllerRegisters defines low level procedures
*             for access to all registers.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#include <HAL/DrmControllerRegisters.hpp>

/** DrmControllerRegisters
*   \brief Class constructor.
*   \param[in] f_read_reg32 function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] f_write_reg32 function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
**/
DrmControllerLibrary::DrmControllerRegisters::DrmControllerRegisters(t_drmReadRegisterFunction f_read_reg32,
                                                                     t_drmWriteRegisterFunction f_write_reg32)
 : f_read_reg32(f_read_reg32),
   f_write_reg32(f_write_reg32)
{ }

/** ~DrmControllerRegisters
*   \brief Class destructor.
**/
DrmControllerLibrary::DrmControllerRegisters::~DrmControllerRegisters() { }
