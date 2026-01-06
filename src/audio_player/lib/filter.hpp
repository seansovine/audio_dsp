#ifndef FILTER_H_
#define FILTER_H_

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Initial implementation of IIR lowpass filter.
// Can be generalized and optimized more later.

class IIRLowpassFilter {
    // Coefficents of transfer function num / denom polynomials.

    // Number of a / b coefficents. (Here the same # for each.)
    static constexpr uint64_t FILTER_SIZE = 6;

    // clang-format off
    static constexpr double mBCoeffs[FILTER_SIZE] = {0.00319064, -0.00927144,  0.00608712,  0.00608712, -0.00927144, 0.00319064};
    static constexpr double mACoeffs[FILTER_SIZE] = {1.        , -4.84007379,  9.40031811, -9.1568206 ,  4.4733176 , -0.87672867};
    // clang-format on

    // Circular buffers to hold previous input / output values.
    //
    // We store previous input values to simplify use and initialization.

    static constexpr uint64_t BUFFER_LEN = 32;

    // Left channel data.
    double mPrevInputsL[BUFFER_LEN] = {0.0f};
    double mPrevOutputsL[BUFFER_LEN] = {0.0f};

    // Right channel data.
    double mPrevInputsR[BUFFER_LEN] = {0.0f};
    double mPrevOutputsR[BUFFER_LEN] = {0.0f};

    // Pointer to last element written in buffers.
    uint64_t mLastIoIdxL = FILTER_SIZE - 1;
    uint64_t mLastIoIdxR = FILTER_SIZE - 1;

    // User-supplied parameters.

    // Size of write buffer outgoing to audio device.
    size_t mWriteBufferSize = 0;
    // If == 2 then in/out buffers are interleaved.
    size_t mNChannels = 1;

  public:
    IIRLowpassFilter(size_t mWriteBufferSize, size_t nChannels)
        : mWriteBufferSize(mWriteBufferSize),
          mNChannels(nChannels) {
    }

    void fillBuffer(const float *inBuffer, float *outBuffer, const float mix) {
        for (size_t i = 0; i < mWriteBufferSize / mNChannels; i++) {
            const float *lInPtr = inBuffer + (mNChannels * i);
            float *lOutPtr = outBuffer + (mNChannels * i);

            double next = getNextL(*lInPtr);
            setIOL(*lInPtr, next);
            assert(!std::isnan(next));

            *lOutPtr = mix * next + *lInPtr;

            if (mNChannels == 2) {
                const float *rInPtr = lInPtr + 1;
                float *rOutPtr = lOutPtr + 1;

                next = getNextR(*rInPtr);
                setIOR(*rInPtr, next);
                assert(!std::isnan(next));

                *rOutPtr = mix * next + *rInPtr;
            }
        }
    }

  private:
    // offset: Offset from current sample number.
    __attribute__((always_inline)) inline double getIL(int64_t offset) {
        return mPrevInputsL[(mLastIoIdxL + BUFFER_LEN - (offset - 1)) % BUFFER_LEN];
    }
    __attribute__((always_inline)) inline double getOL(int64_t offset) {
        return mPrevOutputsL[(mLastIoIdxL + BUFFER_LEN - (offset - 1)) % BUFFER_LEN];
    }

    __attribute__((always_inline)) inline double getIR(int64_t offset) {
        return mPrevInputsR[(mLastIoIdxR + BUFFER_LEN - (offset - 1)) % BUFFER_LEN];
    }
    __attribute__((always_inline)) inline double getOR(int64_t offset) {
        return mPrevOutputsR[(mLastIoIdxR + BUFFER_LEN - (offset - 1)) % BUFFER_LEN];
    }

    void setIOL(double iValue, double oValue) {
        mLastIoIdxL = (mLastIoIdxL + 1) % BUFFER_LEN;
        mPrevInputsL[mLastIoIdxL] = iValue;
        mPrevOutputsL[mLastIoIdxL] = oValue;
    }

    void setIOR(double iValue, double oValue) {
        mLastIoIdxR = (mLastIoIdxR + 1) % BUFFER_LEN;
        mPrevInputsR[mLastIoIdxR] = iValue;
        mPrevOutputsR[mLastIoIdxR] = oValue;
    }

    double getNextL(const double newSample) {
        double out = mBCoeffs[0] * newSample;
        for (uint32_t i = 1; i < FILTER_SIZE; i++) {
            out += mBCoeffs[i] * getIL(i);
            out -= mACoeffs[i] * getOL(i);
        }
        return out;
    }

    double getNextR(const double newSample) {
        double out = mBCoeffs[0] * newSample;
        for (uint32_t i = 1; i < FILTER_SIZE; i++) {
            out += mBCoeffs[i] * getIR(i);
            out -= mACoeffs[i] * getOR(i);
        }
        return out;
    }
};

#endif // FILTER_H_
