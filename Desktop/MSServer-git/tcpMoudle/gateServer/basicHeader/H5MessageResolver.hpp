#ifndef H5MESSAGERESOLVER_HPP_INCLUDED
#define H5MESSAGERESOLVER_HPP_INCLUDED

#include "ConnImplSt.hpp"
#include "sha1.h"
#include "base64.h"

class H5MessageResolver : public MessageResolver
{
    public:
        virtual int handShake( char * msgBuf, char * respond, int & rLen )
        {
            //
            std::istringstream shakeStr(msgBuf);
            std::string request;
            std::getline(shakeStr, request);
            std::cout<<request<<std::endl;
            //
            std::string header;
            header = shakeStr.str();
            //std::cout<<header<<std::endl;
            std::string::size_type posSt = 0;
            std::string::size_type posEnd = 0;
            posSt = header.find("Sec-WebSocket-Key:") + sizeof("Sec-WebSocket-Key:");
            posEnd = header.find("\r", posSt);
            //printf("%d, %d\n", posSt, posEnd);
            std::string key = header.substr(posSt, posEnd-posSt);
            std::cout<<key<<std::endl;
            //
            strcat(respond, "HTTP/1.1 101 Switching Protocols\r\n");
            strcat(respond, "Connection: upgrade\r\n");
            strcat(respond, "Sec-WebSocket-Accept: ");

            key += MAGIC_KEY;
            SHA1 sha1;
            unsigned int message_digest[5];
            sha1.Reset();
            sha1<<key.c_str();
            sha1.Result(message_digest);
            for (int i = 0; i < 5; i++) {
                message_digest[i] = htonl(message_digest[i]);
            }
            key = base64_encode(reinterpret_cast<const unsigned char*>(message_digest),20);
            key += "\r\n";
            strcat(respond, key.c_str());
            strcat(respond, "Upgrade: websocket\r\n\r\n");
            //
            rLen = strlen(respond);

            return 0;
        }

        virtual int decoder( char * msgBuf, int & pos )
        {
            uint8_t fin = 0;
            uint8_t opcode = 0;
            uint8_t mask = 0;
            uint8_t maskingKey[4] = {0};
            uint64_t payloadLength = 0;

            fin = (unsigned char)msgBuf[pos] >> 7;
            opcode = msgBuf[pos] & 0x0f;
            pos++;
            mask = (unsigned char)msgBuf[pos] >> 7;

            //
            payloadLength = msgBuf[pos] & 0x7f;
            pos++;

            if(payloadLength == 126){
                uint16_t length = 0;
                memcpy(&length, msgBuf + pos, 2);
                pos += 2;
                payloadLength = ntohs(length);
            }
            else if(payloadLength == 127)
            {
                uint32_t length = 0;
                memcpy(&length, msgBuf + pos, 4);
                pos += 4;
                payloadLength = ntohl(length);
            }

            //
            if(mask == 1)
            {
                for(int i = 0; i < 4; i++)
                {
                    maskingKey[i] = msgBuf[pos + i];
                }
                //
                pos += 4;

                for(uint i = 0; i < payloadLength; i++)
                {
                    int j = i % 4;
                    msgBuf[pos + i] = msgBuf[pos + i] ^ maskingKey[j];
                }
            }

            printf("fin:%d  mask: %d payloadLength: %d \n", fin, mask, payloadLength);

            return payloadLength;
        }

        virtual int encoder( char * msgBuf, uint32_t len )
        {
            int pos = 0;
            uint8_t  head = 0x81;
            memcpy(msgBuf, &head, 1);
            pos += 1;
            //
            if(len < 126)
            {
                uint8_t dLen = (uint8_t)(len & 0xff);
                memcpy(msgBuf+pos, &dLen, 1);
                pos++;
            }
            else if(len < 0xffff)
            {
                uint8_t dLen = 126;
                memcpy(msgBuf+pos, &dLen, 1);
                pos++;
                //
                uint16_t length = (uint16_t)(len & 0xffff);
                length = htons(length);
                memcpy(msgBuf+pos, &length, 2);
                pos += 2;
            }
            else
            {
                uint8_t dLen = 127;
                memcpy(msgBuf+pos, &dLen, 1);
                pos++;
                //
                uint32_t length = htonl(len);
                memcpy(msgBuf+pos, &length, 4);
                pos += 4;
            }

            return pos;
        }
};


#endif // H5MESSAGERESOLVER_HPP_INCLUDED
