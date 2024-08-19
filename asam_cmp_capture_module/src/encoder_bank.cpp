#include <asam_cmp_capture_module/encoder_bank.h>

BEGIN_NAMESPACE_ASAM_CMP_CAPTURE_MODULE

void EncoderBank::init(uint16_t deviceId)
{
    for (int i = 0; i < encodersCount; ++i){
        encoders[i].setDeviceId(deviceId);
        encoders[i].setStreamId(i);
    }
}

std::vector<std::vector<uint8_t>> EncoderBank::encode(uint8_t encoderInd, const ASAM::CMP::Packet& packet, const ASAM::CMP::DataContext& dataContext)
{
    std::scoped_lock lock(encoderSyncs[encoderInd]);
    return encoders[encoderInd].encode(packet, dataContext);
}

END_NAMESPACE_ASAM_CMP_CAPTURE_MODULE
