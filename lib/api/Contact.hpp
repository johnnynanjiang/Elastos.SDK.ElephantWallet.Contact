/**
 * @file	Contact.hpp
 * @brief	Contact
 * @details	
 *
 * @author	xxx
 * @author	<xxx@xxx.com>
 * @copyright	(c) 2012 xxx All rights reserved.
 **/

#ifndef _ELASTOS_SDK_CONTACT_HPP_
#define _ELASTOS_SDK_CONTACT_HPP_

#ifndef WITH_CROSSPL

#include <Elastos.SDK.Contact.hpp>
#include <ContactBridge.hpp>
#include <ContactChannel.hpp>
#include <ContactDataListener.hpp>
#include <ContactFactory.hpp>
#include <ContactListener.hpp>
#include <ContactMessage.hpp>
#include <memory>

class ElaphantContact : public ContactBridge {
public:
    /*** type define ***/
    class Factory final: public ContactFactory {
       public:
        static std::shared_ptr<ElaphantContact> Create() {
            struct Impl : ElaphantContact {
            };

            return std::make_shared<Impl>();
        }

        // void SetDeviceId(const std::string& devId) {
        //     ContactFactory.SetDeviceId(devId);
        //     // UserInfo.SetCurrDevId(devId);
        // }

        private:
         explicit Factory() = default;
         virtual ~Factory() = default;
    }; // class Factory

    using UserInfo = elastos::UserInfo;

    class Message: public ContactMessage {
    public:
        class MsgData {
        public:
            virtual std::string toString() = 0;
        };

        class TextData: public  MsgData {
        public:
            explicit TextData(const std::string& text)
                    : text(text) {
            }
            virtual ~TextData() = default;
            virtual std::string toString() override;

            const std::string text;
        };

        class BinaryData: public MsgData {
        public:
            explicit BinaryData(const std::vector<uint8_t>& binary)
                    : binary(std::move(binary)) {
            }
            virtual ~BinaryData() = default;
            virtual std::string toString() override;

            const std::vector<uint8_t> binary;
        };

//        class FileData: public MsgData {
//        public:
//            explicit FileData(std::fstream& file) {
////                devId = Platform::GetCurrDevId();
////                name = file.getName();
//                size = file.tellg();
////                md5 = Utils.getMD5Sum(file);
//            }
//
//            // fix json decode and encode different issue
//            static std::string ConvertId(const std::string& id) {
////                FileData fileData = new Gson().fromJson(id, FileData.class);
////                if (fileData == null) {
////                    Log.w(Contact.TAG, "FileData.ConvertId() 0 Failed to convert " + id);
////                }
//
//                return fileData.toString();
//            }
//
//        virtual std::string toString() override {
//        auto jsonInfo = elastos::Json::object();
//            jsonInfo[JsonKey::Text] = text;
//            return jsonInfo->dump(2);
//        }
//
//            const std::string devId;
//            const std::string name;
//            const int64_t size;
//            const std::string md5;
//        };

        explicit Message(Type type, std::shared_ptr<MsgData> data, const std::string& cryptoAlgorithm)
                : type(type)
                , data(data)
                , cryptoAlgorithm(cryptoAlgorithm)
                , timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()
                            ).count()) {
        }

        explicit Message(Type type, const std::vector<uint8_t>& data, std::string cryptoAlgorithm, int64_t timestamp);

        virtual ~Message() = default;

        const Type type;
        std::shared_ptr<MsgData> data;
        const std::string cryptoAlgorithm;
        const int64_t timestamp;
    }; // class Message

    class Listener: public ContactListener {
    public:
        virtual void onReceivedMessage(const std::string& humanCode, ContactChannel channelType,
                                       std::shared_ptr<Message> msgInfo) = 0;

    private:
        virtual void onReceivedMessage(const std::string& humanCode, ContactChannel channelType,
                                       std::shared_ptr<elastos::MessageManager::MessageInfo> msgInfo) override {
            auto message = std::make_shared<Message>(msgInfo->mType, msgInfo->mPlainContent, msgInfo->mCryptoAlgorithm, msgInfo->mTimeStamp);
            onReceivedMessage(humanCode, channelType, message);
        };
    }; // class Listener

    class DataListener: public ContactDataListener {
    }; // class DataListener

    /*** static function and variable ***/
    static std::shared_ptr<Message> MakeTextMessage(const std::string& text, const std::string& cryptoAlgorithm) {
        auto data = std::make_shared<Message::TextData>(text);
        auto msg = std::make_shared<Message>(Message::Type::MsgText, data, cryptoAlgorithm);
        return msg;
    }

    static std::shared_ptr<Message> MakeBinaryMessage(const std::vector<uint8_t>& binary, const std::string& cryptoAlgorithm) {
        auto data = std::make_shared<Message::BinaryData>(binary);
        auto msg = std::make_shared<Message>(Message::Type::MsgBinary, data, cryptoAlgorithm);
        return msg;
    }

//    static std::shared_ptr<Message> MakeFileMessage(std::fstream& file, const std::string& cryptoAlgorithm) {
//        auto data = std::shared_ptr<FileData>(file);
//        auto msg = std::make_shared<Message>(MsgFile, data, cryptoAlgorithm);
//        return msg;
//    }
//
    /*** class function and variable ***/
    std::shared_ptr<ElaphantContact::UserInfo> getUserInfo();
    int sendMessage(const std::string& friendCode, ContactChannel chType, std::shared_ptr<Message> message);

private:
    /*** type define ***/

    /*** static function and variable ***/

    /*** class function and variable ***/
    explicit ElaphantContact() = default;
    virtual ~ElaphantContact() = default;


}; // class Contact

#endif // WITH_CROSSPL

#endif /* _ELASTOS_SDK_CONTACT_HPP_ */