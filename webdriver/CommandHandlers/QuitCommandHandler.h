// Licensed to the Software Freedom Conservancy (SFC) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership. The SFC licenses this file
// to you under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef WEBDRIVER_IE_QUITCOMMANDHANDLER_H_
#define WEBDRIVER_IE_QUITCOMMANDHANDLER_H_

#include "webdriver/MBCommandHandler.h"
#include "webdriver/MBCommandExecutor.h"

namespace webdriver {

class QuitCommandHandler : public MBCommandHandler {
public:
    QuitCommandHandler(void);
    virtual ~QuitCommandHandler(void);

protected:
    void ExecuteInternal(const MBCommandExecutor& executor, const ParametersMap& command_parameters, Response* response);
};

} // namespace webdriver

#endif // WEBDRIVER_IE_QUITCOMMANDHANDLER_H_
