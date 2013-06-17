/*    Copyright 2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "mongo/base/init.h"

MONGO_INITIALIZER_GROUP(default, MONGO_NO_PREREQUISITES, MONGO_NO_DEPENDENTS)

MONGO_INITIALIZER_GROUP(globalVariableConfigurationStarted, MONGO_NO_PREREQUISITES, MONGO_NO_DEPENDENTS)
MONGO_INITIALIZER_GROUP(globalVariablesDeclared, ("globalVariableConfigurationStarted"), MONGO_NO_DEPENDENTS)
MONGO_INITIALIZER_GROUP(globalVariablesSet, ("globalVariablesDeclared"), MONGO_NO_DEPENDENTS)
MONGO_INITIALIZER_GROUP(globalVariablesConfigured, ("globalVariablesDeclared"), ("default"))
