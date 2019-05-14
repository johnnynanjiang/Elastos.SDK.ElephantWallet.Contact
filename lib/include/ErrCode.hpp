#ifndef _ELASTOS_ERRCODE_HPP_
#define _ELASTOS_ERRCODE_HPP_

#include <string>

namespace elastos {

class ErrCode {
public:
    /*** type define ***/

    /*** static function and variable ***/
    constexpr static const int UnknownError = -1;
    constexpr static const int InvalidArgument = -2;
    constexpr static const int IOSystemException = -4;
    constexpr static const int NetworkException = -5;

    constexpr static const int InvalidLocalDataDir = -50;
    constexpr static const int NoSecurityListener = -51;
    constexpr static const int BadSecurityValue = -52;
    constexpr static const int KeypairError = -53;

    constexpr static const int ChannelFailedMultiOpen = -200;
    constexpr static const int ChannelFailedNewCarrier = -201;
    constexpr static const int ChannelFailedRunCarrier = -202;
    constexpr static const int ChannelNotFound = -203;
    constexpr static const int ChannelNotReady = -204;


    constexpr static const int StdSystemErrorIndex = -1000;

    static std::string ToString(int errCode);

    /*** class function and variable ***/

private:
    /*** type define ***/

    /*** static function and variable ***/

    /*** class function and variable ***/
    explicit ErrCode() = delete;
    virtual ~ErrCode() = delete;

}; // class ErrCode

} // namespace elastos

#endif /* _ELASTOS_ERRCODE_HPP_ */
