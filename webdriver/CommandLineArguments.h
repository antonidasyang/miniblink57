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

#ifndef COMMANDLINEARGUMENTS_H_
#define COMMANDLINEARGUMENTS_H_

#include <algorithm>
#include <map>
#include <string>
#include <vector>

class CommandLineArguments {
public:
    CommandLineArguments(int arg_count, char* arg_array[]);
    virtual ~CommandLineArguments(void);

    std::string GetValue(std::string arg_name, std::string default_value);
    bool is_help_requested(void) const
    {
        return this->is_help_requested_;
    }
    bool is_version_requested(void) const
    {
        return this->is_version_requested_;
    }

private:
    void ParseArguments(int argc, char* argv[]);
    int GetSwitchDelimiterLength(std::string arg);

    bool is_help_requested_;
    bool is_version_requested_;
    std::map<std::string, std::string> args_map_;
};

#endif // COMMANDLINEARGUMENTS_H_