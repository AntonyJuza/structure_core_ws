/*
    AppInterface_Desktop.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "AppInterface.h"

int main(int argc, char *argv[]) {
    if(argc > 1){
        AppInterface::cliCmdHandler(argc, argv);
    }
    else{
        AppInterface::setup();
        AppInterface::runUntilWindowClosed();
        AppInterface::teardown();
    }
    return 0;
}
