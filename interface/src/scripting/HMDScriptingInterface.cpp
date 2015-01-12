//
//  HMDScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Thijs Wenker on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HMDScriptingInterface.h"

HMDScriptingInterface& HMDScriptingInterface::getInstance() {
    static HMDScriptingInterface sharedInstance;
    return sharedInstance;
}
