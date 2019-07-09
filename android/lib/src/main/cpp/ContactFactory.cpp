//
//  Elastos.SDK.Contact.cpp
//
//  Created by mengxk on 19/03/16.
//  Copyright © 2016 mengxk. All rights reserved.
//

#include <ContactFactory.hpp>

#include <Elastos.SDK.Contact.hpp>
#include "Log.hpp"

/***********************************************/
/***** static variables initialize *************/
/***********************************************/

/***********************************************/
/***** static function implement ***************/
/***********************************************/

void ContactFactory::SetLogLevel(int level)
{
    elastos::Contact::Factory::SetLogLevel(level);

    return;
}

int ContactFactory::SetLocalDataDir(const std::string& dir)
{
    int ret = elastos::Contact::Factory::SetLocalDataDir(dir);

    return ret;
}

/***********************************************/
/***** class public function implement  ********/
/***********************************************/
ContactFactory::ContactFactory()
{
    Log::I(Log::TAG, "%s", __PRETTY_FUNCTION__);
}
ContactFactory::~ContactFactory()
{
    Log::I(Log::TAG, "%s", __PRETTY_FUNCTION__);
}

/***********************************************/
/***** class protected function implement  *****/
/***********************************************/


/***********************************************/
/***** class private function implement  *******/
/***********************************************/