//
//  Elastos.SDK.Contact.cpp
//
//  Created by mengxk on 19/03/16.
//  Copyright © 2016 mengxk. All rights reserved.
//


#include <Contact.hpp>

#include <SafePtr.hpp>
#include <JsonDefine.hpp>
#include <CompatibleFileSystem.hpp>
#include <Platform.hpp>
#include <MD5.hpp>

#ifndef WITH_CROSSPL

namespace elastos {
namespace sdk {

/***********************************************/
/***** static variables initialize *************/
/***********************************************/

/***********************************************/
/***** static function implement ***************/
/***********************************************/


/***********************************************/
/***** class public function implement  ********/
/***********************************************/
std::shared_ptr<Contact::UserInfo> Contact::getUserInfo()
{
    if(mContactImpl->isStarted() == false) {
        return nullptr;
    }

    auto weakUserMgr = mContactImpl->getUserManager();
    auto userMgr =  SAFE_GET_PTR_DEF_RETVAL(weakUserMgr, nullptr);

    std::shared_ptr<Contact::UserInfo> userInfo;
    int ret = userMgr->getUserInfo(userInfo);
    CHECK_AND_RETDEF(ret, nullptr);

    return userInfo;
}

std::vector<std::shared_ptr<Contact::FriendInfo>> Contact::listFriendInfo()
{
    std::vector<std::shared_ptr<elastos::FriendInfo>> friendList;

    if(mContactImpl->isStarted() == false) {
        return friendList;
    }

    auto weakFriendMgr = mContactImpl->getFriendManager();
    auto friendMgr =  SAFE_GET_PTR_DEF_RETVAL(weakFriendMgr, friendList);

    int ret = friendMgr->getFriendInfoList(friendList);
    CHECK_AND_RETDEF(ret, friendList);

    return friendList;
}

int Contact::sendMessage(const std::string& friendCode, Channel chType, std::shared_ptr<Message> message)
{
    auto dataInfo = message->data->toData();
    std::span<uint8_t> data(dataInfo.data(), dataInfo.size());
    auto ret = message->syncMessageToNative(static_cast<int>(message->type), &data,
                                            message->cryptoAlgorithm,
                                            message->nanoTime,
                                            message->replyToNanoTime);
    CHECK_ERROR(ret);

    ret = ContactBridge::sendMessage(friendCode, chType, message);
    CHECK_ERROR(ret);

    return 0;
}

Contact::Message::Message(Type type, const std::vector<uint8_t>& data, std::string cryptoAlgorithm,
                          int64_t nanoTime, int64_t replyToNanoTime)
        : type(type)
        , data()
        , cryptoAlgorithm(cryptoAlgorithm)
        , nanoTime(nanoTime)
        , replyToNanoTime(replyToNanoTime)
{
    switch (type) {
        case Type::MsgText:
            this->data = std::make_shared<TextData>();
            break;
        case Type::MsgBinary:
            this->data = std::make_shared<BinaryData>();
            break;
        case Type::MsgFile:
            this->data = std::make_shared<FileData>();
            break;
        default:
            Log::E(Log::TAG, "Unprocessed Message Type");
            break;
    }
    if(this->data != nullptr) {
        this->data->fromData(data);
    }
}

std::string Contact::Message::TextData::toString()
{
    return this->text;
}

std::vector<uint8_t> Contact::Message::TextData::toData()
{
    return std::vector<uint8_t>(this->text.begin(), this->text.end());
}
void Contact::Message::TextData::fromData(const std::vector<uint8_t>& data)
{
    this->text = std::string(data.begin(), data.end());
}

std::string Contact::Message::BinaryData::toString()
{
    auto jsonInfo = elastos::Json(this->binary);
    return jsonInfo.dump();
}

std::vector<uint8_t> Contact::Message::BinaryData::toData()
{
    return this->binary;
}
void Contact::Message::BinaryData::fromData(const std::vector<uint8_t>& data)
{
    this->binary = data;
}

Contact::Message::FileData::FileData(const std::string& filepath)
{
    elastos::Platform::GetCurrentDevId(this->devId);
    auto file = elastos::filesystem::path(filepath);
    this->name = file.filename();
    this->size = elastos::filesystem::file_size(file);
    this->md5 = elastos::MD5::Get(file);
}

std::string Contact::Message::FileData::toString()
{
    auto jsonInfo = elastos::Json::object();
    jsonInfo[elastos::JsonKey::DeviceId] = this-> devId;
    jsonInfo[elastos::JsonKey::Name] = this-> name;
    jsonInfo[elastos::JsonKey::Size] = this-> size;
    jsonInfo[elastos::JsonKey::Md5] = this-> md5;
    return jsonInfo.dump();
}

std::vector<uint8_t> Contact::Message::FileData::toData()
{
    auto dataId = toString();
    return std::vector<uint8_t>(dataId.begin(), dataId.end());
}
void Contact::Message::FileData::fromData(const std::vector<uint8_t>& data)
{
    auto jsonInfo = elastos::Json::parse(data);
    this-> devId = jsonInfo[elastos::JsonKey::DeviceId];
    this-> name = jsonInfo[elastos::JsonKey::Name];
    this-> size = jsonInfo[elastos::JsonKey::Size];
    this-> md5 = jsonInfo[elastos::JsonKey::Md5];
}


/***********************************************/
/***** class protected function implement  *****/
/***********************************************/


/***********************************************/
/***** class private function implement  *******/
/***********************************************/


} // namespace sdk
} // namespace elastos

#endif // WITH_CROSSPL

