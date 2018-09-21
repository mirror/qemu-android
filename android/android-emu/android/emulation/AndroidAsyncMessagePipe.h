// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidPipe.h"

#include <array>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// This is a utility class that can help implement message-based remote calls
// between the host and guest, with optional out-of-band responses.
//
// Usage:
// 1. Inherit AndroidMessagePipe, and implement OnMessage to receive callbacks
//    when messages are received.
// 2. Call Send(data) to send a response to the client.
//
// VERY IMPORTANT: this function runs on a QEMU's CPU thread while BQL is
// locked. This means the OnMessage must be quick, if any complex operation must
// be done do it on another thread and call send(data) from that thread.
//
// 3. Register the service
// void registerMyXYZService() {
//    android::AndroidPipe::Service::add(new
//        android::AndroidAsyncMessagePipe::Service<YourPipe>("MyXYZService");
// }
//
// AndroidAsyncMessagePipe implements a simple packet-based protocol on top of
// qemu pipe service.  The length of each packet is encoded as a little-endian
// uint32 immediately before the message:
//
// <uint32 length> <length bytes of data>
//
// (Little-endian is for historical reasons for compatibility with
// AndroidMessagePipe-based pipes)

namespace android {

struct AsyncMessagePipeHandle {
    int id = -1;

    bool isValid() const { return id >= 0; }
};

class AndroidAsyncMessagePipe : public AndroidPipe {
public:
    struct PipeArgs {
        AsyncMessagePipeHandle handle;
        void* hwPipe;
        const char* args;
        base::Stream* loadStream;
        std::function<void(AsyncMessagePipeHandle)> deleter;
    };

    //
    // Pipe Service
    //

    template <typename PipeType>
    class Service : public AndroidPipe::Service {
    public:
        Service(const char* serviceName) : AndroidPipe::Service(serviceName) {}

        bool canLoad() const override { return true; }

        virtual AndroidPipe* load(void* hwPipe,
                                  const char* args,
                                  base::Stream* stream) final {
            return createPipe(hwPipe, args, stream);
        }

        virtual AndroidPipe* create(void* hwPipe, const char* args) final {
            return createPipe(hwPipe, args, nullptr);
        }

        void destroyPipe(AsyncMessagePipeHandle handle) {
            base::AutoLock lock(mLock);
            const auto it = mPipes.find(handle.id);
            if (it == mPipes.end()) {
                LOG(WARNING)
                        << "destroyPipe could not find pipe id " << handle.id;
                return;
            }

            mPipes.erase(it);
        }

        struct PipeRef {
            PipeType* const pipe;
            base::AutoLock lock;
        };

        base::Optional<PipeRef> getPipe(AsyncMessagePipeHandle handle) {
            base::AutoLock lock(mLock);
            const auto it = mPipes.find(handle.id);
            if (it == mPipes.end()) {
                LOG(VERBOSE) << "getPipe could not find pipe id " << handle.id;
                return {};
            }

            return PipeRef{it->second.get(), std::move(lock)};
        }

    private:
        AndroidPipe* createPipe(void* hwPipe,
                                const char* args,
                                base::Stream* stream) {
            base::AutoLock lock(mLock);
            AsyncMessagePipeHandle handle = nextPipeHandle();
            PipeArgs pipeArgs = {handle, hwPipe, args, stream,
                                 std::bind(&Service::destroyPipe, this,
                                           std::placeholders::_1)};
            std::unique_ptr<PipeType> pipe(
                    new PipeType(this, std::move(pipeArgs)));
            AndroidPipe* pipePtr = pipe.get();
            mPipes[handle.id] = std::move(pipe);
            return pipePtr;
        }

        AsyncMessagePipeHandle nextPipeHandle() {
            AsyncMessagePipeHandle handle;
            handle.id = mNextId++;
            return handle;
        }

        base::Lock mLock;
        int mNextId = 0;
        std::unordered_map<int, std::unique_ptr<PipeType>> mPipes;
    };

    AndroidAsyncMessagePipe(AndroidPipe::Service* service, PipeArgs&& args);

    void send(std::vector<uint8_t>&& data);
    void send(const std::vector<uint8_t>& data);
    virtual void onMessage(const std::vector<uint8_t>& data) = 0;

    void onSave(base::Stream* stream) override;
    virtual void onLoad(base::Stream* stream);

    AsyncMessagePipeHandle getHandle() { return mHandle; }

private:
    bool allowRead() const;

    int readBuffers(const AndroidPipeBuffer* buffers, int numBuffers);
    int writeBuffers(AndroidPipeBuffer* buffers, int numBuffers);

    void onGuestClose(PipeCloseReason reason) final { mDeleter(mHandle); }
    unsigned onGuestPoll() const final;
    void onGuestWantWakeOn(int flags) final {}

    // Rename onGuestRecv/onGuestSend to be in the context of the host.
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) final {
        return writeBuffers(buffers, numBuffers);
    }

    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) final {
        return readBuffers(buffers, numBuffers);
    }

    struct OutgoingPacket {
        size_t offset = 0;
        std::vector<uint8_t> data;

        OutgoingPacket(std::vector<uint8_t>&& data);
        explicit OutgoingPacket(base::Stream* stream);

        size_t length() const { return sizeof(uint32_t) + data.size(); }
        bool complete() const;

        // Copy the packet contents to the AndroidPipeBuffer, updating
        // writeOffset based on how much was written.
        void copyToBuffer(AndroidPipeBuffer& buffer, size_t* writeOffset);

        // Serialize to a stream.
        void serialize(base::Stream* stream) const;
    };

    struct IncomingPacket {
        size_t headerBytesRead = 0;
        uint32_t messageLength = 0;
        std::vector<uint8_t> data;

        IncomingPacket() = default;
        explicit IncomingPacket(base::Stream* stream);

        bool lengthKnown() const;
        base::Optional<size_t> bytesRemaining() const;
        bool complete() const;

        // Read the packet data from an AndroidPipeBuffer, updating the
        // readOffset based on how much was written.
        //
        // Returns the number of bytes read.
        size_t copyFromBuffer(const AndroidPipeBuffer& buffer,
                              size_t* readOffset);

        // Serialize to a stream.
        void serialize(base::Stream* stream) const;
    };

    mutable std::recursive_mutex mMutex;
    const AsyncMessagePipeHandle mHandle;
    const std::function<void(AsyncMessagePipeHandle)> mDeleter;

    base::Optional<IncomingPacket> mIncomingPacket;
    std::list<OutgoingPacket> mOutgoingPackets;
};

typedef std::function<void(std::vector<uint8_t>&&)> PipeSendFunction;
typedef std::function<void(const std::vector<uint8_t>&,
                           const PipeSendFunction& func)>
        OnMessageCallbackFunction;

// Helper to register a simple message pipe service, passing a lambda as an
// onMessage handler.
//
// Takes a callback with this signature:
// void onMessageCallback(const std::vector<uint8_t>& data,
//         sendFunction(std::vector<uint8_t>&& response));
//
// When ready to send a callback, simply invoke sendFunction, which works from
// within the onMessageCallback or from any other thread.
void registerSimpleAndroidAsyncMessagePipeService(
        const char* serviceName,
        OnMessageCallbackFunction onMessageCallback);

}  // namespace android
