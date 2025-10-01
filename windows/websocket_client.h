#pragma once
#include "pch.h"

#include <iostream>
#include<functional>
#include<atomic>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Sockets.h>
#include <winrt/Windows.Storage.Streams.h>

#include "utils.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

struct WebSocketClient
{
    MessageWebSocket socket{ nullptr };
    DataWriter writer{ nullptr };

    std::function<void(std::wstring)> onMessage;
    std::function<void(uint16_t, std::wstring)> onClosed;

    std::atomic<bool> connected{ false };

    bool connect(std::wstring const& url)
    {
        try
        {
            socket = MessageWebSocket();
            socket.Control().MessageType(SocketMessageType::Utf8);

            socket.MessageReceived([this](MessageWebSocket const&,
                MessageWebSocketMessageReceivedEventArgs const& args) {
                    try
                    {
                        DataReader reader(args.GetDataReader());
                        reader.UnicodeEncoding(UnicodeEncoding::Utf8);
                        hstring msg = reader.ReadString(reader.UnconsumedBufferLength());
                        write_log(L"new message", msg);
                        if (onMessage) {
                            onMessage(msg.c_str());
                        }
                    }
                    catch (winrt::hresult_error const& e) {
                        write_log(L"Error in MessageReceived: ", e.message().c_str());
                        if (onClosed) onClosed((uint16_t)-9999, e.message().c_str());
                    }
                    catch (...) {
                        write_log(L"Error message received");
                        if (onClosed) onClosed((uint16_t)-9999, L"Error message received");
                    }
                });

            socket.Closed([this](IInspectable const&, WebSocketClosedEventArgs const& e)
                {
                    connected = false;
                    write_log(L"Socket closed: ", e.Reason().c_str());
                    if (onClosed) onClosed(e.Code(), e.Reason().c_str());
                });

            try {
                auto connectOp = socket.ConnectAsync(Uri(url));
                if (connectOp.wait_for(std::chrono::seconds(20)) == winrt::Windows::Foundation::AsyncStatus::Completed) {
                    connectOp.get();
                }
                else {
                    connectOp.Cancel();
                    throw winrt::hresult_error(E_FAIL, L"Connect time out");
                }
                write_log(L"Socket connect: ", L"Connect Successfully");
            }
            catch (winrt::hresult_error const& e) {
                write_log(L"Error connect: ", e.message().c_str());
                connected = false;
                return false;
            }
            catch (...) {
                write_log("Error connect during unknow");
                connected = false;
                return false;
            }
            try {
                writer = DataWriter(socket.OutputStream());
            }
            catch (winrt::hresult_error const& e) {
                write_log(L"Failed to create DataWriter: ", e.message().c_str());
                connected = false;
                return false;
            }
            catch (...) {
                write_log("Error connect during unknow");
                connected = false;
                return false;
            }

            connected = true;
            return true;
        }
        catch (winrt::hresult_error const& e)
        {
            write_log(L"Connect failed: ", e.message().c_str());
            connected = false;
            return false;
        }
        catch (...) {
            write_log(L"Connect failed: ", L"uk");
            connected = false;
            return false;
        }
    }

    void disconnect()
    {
        try {
            if (writer) {
                writer = nullptr;
            }
            if (socket) {
                socket.Close();
                socket = nullptr;
            }
        }
        catch (winrt::hresult_error const& e) {
            write_log(L"Error closing socket: ", e.message().c_str());
        }
        catch (...) {
            write_log(L"Unknown error closing socket");
        }
        connected = false;
    }

    void send(std::wstring const& msg)
    {
        if (!connected) {
            write_log(L"Send failed: ", L"no connection");
            return;
        }
        try
        {
            writer.WriteString(msg);
            auto storeOp = writer.StoreAsync();
            if (storeOp.wait_for(std::chrono::seconds(10)) == winrt::Windows::Foundation::AsyncStatus::Completed) {
                storeOp.get();
            }
            else {
                storeOp.Cancel();
                throw winrt::hresult_error(E_FAIL, L"Send time out");
            }
            write_log(L"[Send] ", winrt::hstring(msg));
        }
        catch (winrt::hresult_error const& e)
        {
            write_log(L"Send failed: ", e.message().c_str());
            connected = false;
            if (onClosed) onClosed((uint16_t)-8888, L"Error message failed");
        }
        catch (...) {
            write_log(L"Send failed: ", L"uk");
            connected = false;
        }
    }

    bool isConnected() const { return connected; }
};