#include <BlkChnClient.hpp>

#include <iomanip>
#include <HttpClient.hpp>
#include <Log.hpp>
#include <MD5.hpp>
#include <Platform.hpp>
#include <SafePtr.hpp>


namespace elastos {

/* =========================================== */
/* === static variables initialize =========== */
/* =========================================== */
std::shared_ptr<BlkChnClient> BlkChnClient::gBlkChnClient {};

/* =========================================== */
/* === static function implement ============= */
/* =========================================== */
int BlkChnClient::InitInstance(std::weak_ptr<Config> config, std::weak_ptr<SecurityManager> sectyMgr)
{
    if(gBlkChnClient.get() != nullptr) {
        return 0;
    }

    struct Impl: BlkChnClient {
        Impl(std::weak_ptr<Config> config, std::weak_ptr<SecurityManager> sectyMgr) : BlkChnClient(config, sectyMgr) {
        }
    };

    HttpClient::InitGlobal();
    gBlkChnClient = std::make_shared<Impl>(config, sectyMgr);

    return 0;
}

std::shared_ptr<BlkChnClient> BlkChnClient::GetInstance()
{
    assert(gBlkChnClient.get() != nullptr);
    return gBlkChnClient;
}

/* =========================================== */
/* === class public function implement  ====== */
/* =========================================== */
int BlkChnClient::setConnectTimeout(uint32_t milliSecond)
{
    mConnectTimeoutMS = milliSecond;
    return 0;
}

int BlkChnClient::downloadAllDidProps(const std::string& did, std::map<std::string, std::string>& propMap)
{
    auto sectyMgr = SAFE_GET_PTR(mSecurityManager);
    auto config = SAFE_GET_PTR(mConfig);

    auto agentGetProps = config->mDidChainConfig->mAgentApi.mGetDidProps;
    std::string agentGetPropsPath = agentGetProps + did;

    std::string propArrayStr;
    int ret = getDidPropFromDidChn(agentGetPropsPath, propArrayStr);
    if(ret < 0) {
        return ret;
    }

    std::string keyPath;
    ret = getPropKeyPath(keyPath);
    if(ret < 0) {
        return ret;
    }

    Json jsonPropArray = Json::parse(propArrayStr);
    for(const auto& it: jsonPropArray){
        std::string propKey = it["key"];
        std::string propValue = it["value"];

        size_t pos = propKey.find(keyPath);
        if (pos != std::string::npos) {
            propKey.erase(pos, keyPath.length());
        }

        propMap[propKey] = propValue;
    }

    return 0;
}

int BlkChnClient::uploadAllDidProps(const std::map<std::string, std::string>& propMap)
{
    auto sectyMgr = SAFE_GET_PTR(mSecurityManager);
    auto config = SAFE_GET_PTR(mConfig);

    std::string keyPath;
    int ret = getPropKeyPath(keyPath);
    if(ret < 0) {
        return ret;
    }

    Json jsonPropProt = Json::object();
    Json jsonPropArray = Json::array();
    for(const auto& prop: propMap) {
        std::string propKey = keyPath + prop.first;
        std::string propValue = prop.second;
        Json jsonProp = Json::object();
        jsonProp[DidProtocol::Name::Key] = propKey;
        jsonProp[DidProtocol::Name::Value] = propValue;
        if(prop.second.empty() == true) {
            jsonProp[DidProtocol::Name::Status] = DidProtocol::Value::Status::Deprecated;
        } else {
            jsonProp[DidProtocol::Name::Status] = DidProtocol::Value::Status::Normal;
        }

        jsonPropArray.push_back(jsonProp);
    }

    jsonPropProt[DidProtocol::Name::Tag] = DidProtocol::Value::Tag;
    jsonPropProt[DidProtocol::Name::Ver] = DidProtocol::Value::Ver;
    jsonPropProt[DidProtocol::Name::Status] = DidProtocol::Value::Status::Normal;
    jsonPropProt[DidProtocol::Name::Properties] = jsonPropArray;

    auto propProtStr = jsonPropProt.dump();
    Log::I(Log::TAG, "BlkChnClient::uploadAllDidProps() propProt: %s", propProtStr.c_str());
    std::vector<uint8_t> originBytes(propProtStr.begin(), propProtStr.end());
    std::vector<uint8_t> signedBytes(propProtStr.begin(), propProtStr.end());
    ret = sectyMgr->signData(originBytes, signedBytes);
    if(ret < 0) {
        return ret;
    }

    std::string pubKey;
    ret = sectyMgr->getPublicKey(pubKey);
    if(ret < 0) {
        return ret;
    }

    std::string msgStr = MD5::MakeHexString(originBytes);
    std::string sigStr = MD5::MakeHexString(signedBytes);

    // did prop key, sign, make {msg, sig, pub}
    std::string reqBody = std::string("{")
        + "\"pub\":\"" + pubKey + "\","
        + "\"msg\":\"" + msgStr + "\","
        + "\"sig\":\"" + sigStr + "\""
        + "}";

    auto didConfigUrl = config->mDidChainConfig->mUrl;
    auto agentUploadPath = config->mDidChainConfig->mAgentApi.mUploadDidProps;
    std::string agentUploadUrl = didConfigUrl + agentUploadPath;
    std::string authHeader;
    ret = sectyMgr->getDidAgentAuthHeader(authHeader);
    if(ret < 0) {
        return ret;
    }
    Log::I(Log::TAG, "reqBody=%s", reqBody.c_str());

    HttpClient httpClient;
    httpClient.url(agentUploadUrl);
    httpClient.setHeader("Content-Type", "application/json");
    httpClient.setHeader("X-Elastos-Agent-Auth", authHeader);
    ret = httpClient.syncPost(reqBody);
    if(ret < 0) {
        return ErrCode::HttpClientError + ret;
    }

    std::string respBody;
    ret = httpClient.getResponseBody(respBody);
    if(ret < 0) {
        return ErrCode::HttpClientError + ret;
    }
    Log::I(Log::TAG, "respBody=%s", respBody.c_str());

    Json jsonResp = Json::parse(respBody);
    if(jsonResp["status"] != 200) {
        return ErrCode::BlkChnSetPropError;
    }

    return 0;
}

int BlkChnClient::downloadDidProp(const std::string& did, const std::string& key, std::string& prop)
{
    prop = "";

    auto config = SAFE_GET_PTR(mConfig);

    std::string keyPath;
    int ret = getPropKeyPath(keyPath);
    if(ret < 0) {
        return ret;
    }

    auto agentGetProps = config->mDidChainConfig->mAgentApi.mGetDidProps;
    auto agentDidProp = config->mDidChainConfig->mAgentApi.mDidProp;
    std::string agentGetPropPath = agentGetProps + did + agentDidProp + keyPath + key;

    std::string propArrayStr;
    ret = getDidPropFromDidChn(agentGetPropPath, propArrayStr);
    if(ret < 0) {
        return ret;
    }

    Json jsonPropArray = Json::parse(propArrayStr);
    for(const auto& it: jsonPropArray) {
        std::string propKey = it["key"];
        if(keyPath + key != propKey) {
            continue;
        }

        prop = it["value"];
        return 0;
    }

    return ErrCode::BlkChnGetPropError;
}

int BlkChnClient::uploadDidProp(const std::string& key, const std::string& prop)
{
    throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " Unimplemented!!!");
}

int BlkChnClient::getDidPropHistory(const std::string& did, const std::string& key, std::vector<std::string>& values)
{
    values.clear();

    auto config = SAFE_GET_PTR(mConfig);

    std::string keyPath;
    int ret = getPropKeyPath(keyPath);
    if(ret < 0) {
        return ret;
    }

    auto agentGetProps = config->mDidChainConfig->mAgentApi.mGetDidProps;
    auto agentDidPropHistory = config->mDidChainConfig->mAgentApi.mDidPropHistory;
    std::string agentGetPropHistoryPath = agentGetProps + did + agentDidPropHistory + keyPath + key;

    std::string propArrayStr;
    ret = getDidPropFromDidChn(agentGetPropHistoryPath, propArrayStr);
    if(ret < 0) {
        return ret;
    }

    Json jsonPropArray = Json::parse(propArrayStr);
    for(const auto& it: jsonPropArray) {
        values.push_back(it["value"]);
    }

    return 0;
}

int BlkChnClient::downloadHumanInfo(const std::string& did, std::shared_ptr<HumanInfo>& humanInfo)
{
    humanInfo = std::make_shared<HumanInfo>();

    std::string pubKey;
    int ret = downloadDidProp(did, "PublicKey", pubKey);
    if(ret < 0) {
        return ret;
    }

    std::string expectedDid;
    ret = SecurityManager::GetDid(pubKey, expectedDid);
    if(ret < 0) {
        return ret;
    }

    ret = humanInfo->setHumanInfo(HumanInfo::Item::ChainPubKey, pubKey);
    if(ret < 0) {
        return ret;
    }

    std::vector<std::string> propHistory;
    ret = getDidPropHistory(did, "CarrierID", propHistory);
    if(ret < 0) {
        return ret;
    }

    for(const auto& it: propHistory) {
        HumanInfo::CarrierInfo carrierInfo;
        ret = humanInfo->deserialize(it, carrierInfo);
        if(ret < 0) {
            Log::W(Log::TAG, "BlkChnClient::downloadHumanInfo() Failed to sync CarrierId: %s", it.c_str());
            continue; // ignore error
        }

        ret = humanInfo->addCarrierInfo(carrierInfo, HumanInfo::Status::Offline);
        if(ret < 0) {
            if(ret == ErrCode::IgnoreMergeOldInfo) {
                Log::I(Log::TAG, "BlkChnClient::downloadHumanInfo() Ignore to sync CarrierId: %s", it.c_str());
            } else {
                Log::W(Log::TAG, "BlkChnClient::downloadHumanInfo() Failed to sync carrier info. CarrierId: %s", it.c_str());
            }
            continue; // ignore error
        }

        Log::I(Log::TAG, "BlkChnClient::downloadHumanInfo() Success to sync CarrierId: %s", it.c_str());
    }

    return 0;
}

int BlkChnClient::uploadHumanInfo(const std::shared_ptr<HumanInfo>& humanInfo)
{
    std::string pubKey;
    int ret = humanInfo->getHumanInfo(HumanInfo::Item::ChainPubKey, pubKey);
    if(ret < 0) {
        return ret;
    }

    std::string devId;
    ret = Platform::GetCurrentDevId(devId);
    if(ret < 0) {
        return ret;
    }

    HumanInfo::CarrierInfo carrierInfo;
    ret = humanInfo->getCarrierInfoByDevId(devId, carrierInfo);
    if(ret < 0) {
        return ret;
    }

    std::string carrierInfoStr;
    ret = humanInfo->serialize(carrierInfo, carrierInfoStr);
    if(ret < 0) {
        return ret;
    }

    std::map<std::string, std::string> propMap;
    propMap["PublicKey"] = pubKey;
    propMap["CarrierID"] = carrierInfoStr;
    ret = uploadAllDidProps(propMap);
    if(ret < 0) {
        return ret;
    }


    return 0;
}

/* =========================================== */
/* === class protected function implement  === */
/* =========================================== */


/* =========================================== */
/* === class private function implement  ===== */
/* =========================================== */
BlkChnClient::BlkChnClient(std::weak_ptr<Config> config, std::weak_ptr<SecurityManager> sectyMgr)
    : mConfig(config)
    , mSecurityManager(sectyMgr)
    , mTaskThread()
    , mConnectTimeoutMS(10000)
{
}

BlkChnClient::~BlkChnClient()
{
}

int BlkChnClient::getDidPropFromDidChn(const std::string& path, std::string& result)
{
    auto sectyMgr = SAFE_GET_PTR(mSecurityManager);
    auto config = SAFE_GET_PTR(mConfig);

    auto didConfigUrl = config->mDidChainConfig->mUrl;
    std::string agentUrl = didConfigUrl + path;

    HttpClient httpClient;
    httpClient.url(agentUrl);
    int ret = httpClient.syncGet();
    if(ret < 0) {
        return ErrCode::HttpClientError + ret;
    }

    std::string respBody;
    ret = httpClient.getResponseBody(respBody);
    if(ret < 0) {
        return ErrCode::HttpClientError + ret;
    }
    Log::I(Log::TAG, "respBody=%s", respBody.c_str());

    Json jsonResp = Json::parse(respBody);
    if(jsonResp["status"] != 200) {
        return ErrCode::BlkChnGetPropError;
    }

    result = jsonResp["result"];
    if(result.empty() == true) {
        return ErrCode::BlkChnEmptyPropError;
    }

    return 0;
}

int BlkChnClient::getPropKeyPath(std::string& keyPath)
{
    auto sectyMgr = SAFE_GET_PTR(mSecurityManager);

    std::string appId;
    int ret = sectyMgr->getDidPropAppId(appId);
    if(ret < 0) {
        return ret;
    }

    keyPath = "Apps/" + appId + "/";
    return 0;
}


} // namespace elastos
